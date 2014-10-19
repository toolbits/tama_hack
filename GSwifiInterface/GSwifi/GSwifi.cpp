/* Copyright (C) 2013 gsfan, MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "mbed.h"
#include "GSwifi.h"
#include <string>
#include <algorithm>

GSwifi * GSwifi::_inst;

GSwifi::GSwifi(PinName tx, PinName rx, PinName cts, PinName rts, PinName reset, PinName alarm, int baud):
    _gs(tx, rx), _reset(reset)
{
    _inst = this;
    memset(&_state, 0, sizeof(_state));
    memset(&_con, 0, sizeof(_con));
    _state.initialized = false;
    _state.status = STAT_READY;
    _state.cid = -1;
#ifdef CFG_EXTENDED_MEMORY1
    int n = CFG_EXTENDED_SIZE1 / (CFG_MAX_SOCKETS + 1);
    _state.buf = new CircBuffer<char>(n - 1, (void*)CFG_EXTENDED_MEMORY1);
#ifdef CFG_ENABLE_RTOS
    for (int i = 0; i < CFG_MAX_SOCKETS; i ++) {
        _con[i].buf = new CircBuffer<char>(n - 1, (void*)(CFG_EXTENDED_MEMORY1 + (n * (i + 1))));
        if (_con[i].buf == NULL)
            error("CircBuffer failed");
    }
    _threadPoll = NULL;
#endif
#else // CFG_EXTENDED_MEMORY
    _state.buf = new CircBuffer<char>(CFG_DATA_SIZE);
#ifdef CFG_ENABLE_RTOS
    for (int i = 0; i < CFG_MAX_SOCKETS; i ++) {
        _con[i].buf = new CircBuffer<char>(CFG_DATA_SIZE);
        if (_con[i].buf == NULL)
            error("CircBuffer failed");
    }
    _threadPoll = NULL;
#endif
#endif // CFG_EXTENDED_MEMORY

    setReset(true);
    initUart(cts, rts, alarm, baud);
    setAlarm(true);
    wait_ms(10);
    setAlarm(false);
    wait_ms(100);
    setReset(false);
    wait_ms(100);
}

int GSwifi::join()
{
    int i;
    bool r = -1;

    _state.wm = WM_INFRASTRUCTURE;
    _state.initialized = true;
    if (!strlen(_state.name)) {
        strncpy(_state.name, CFG_DHCPNAME, sizeof(_state.name));
    }
    clearFlags();
    _state.mode = MODE_COMMAND;
    sendCommand(NULL, RES_NULL, 0);
    if (cmdE(false)) return -1;
    if (_flow) {
        cmdR(true);
    }
    cmdBDATA(true);
    if (cmdNMAC()) return -1;
    cmdWREGDOMAIN(CFG_WREGDOMAIN);
    cmdWM(0); // infrastructure
    wait_ms(100);

    switch (_state.sec) {
    case SEC_NONE:
    case SEC_OPEN:
    case SEC_WEP:
        cmdNDHCP(_state.dhcp, _state.name, DEFAULT_WAIT_RESP_TIMEOUT);
        if (! _state.dhcp) {
            cmdNSET(_state.ip, _state.netmask, _state.ip);
        }
        cmdWAUTH(_state.sec);
        if (_state.sec != SEC_NONE) {
            cmdWWEP(1, _state.pass);
            wait_ms(100);
        }
        for (i = 0; i < CFG_TRYJOIN; i ++) {
            r = cmdWA(_state.ssid);
            if (!r) break;
            DBG("retry\r\n");
            wait_ms(1000);
        }
        break;
    case SEC_WPA_PSK:
    case SEC_WPA2_PSK:
        cmdNDHCP(_state.dhcp, _state.name, DEFAULT_WAIT_RESP_TIMEOUT);
        if (! _state.dhcp) {
            cmdNSET(_state.ip, _state.netmask, _state.ip);
        }
        cmdWAUTH(0);
        cmdWPAPSK(_state.ssid, _state.pass);
        wait_ms(100);
        for (i = 0; i < CFG_TRYJOIN; i ++) {
            r = cmdWA(_state.ssid);
            if (!r) break;
            DBG("retry\r\n");
            wait_ms(1000);

            if (_state.dhcp && i == CFG_TRYJOIN - 1) {
                cmdNDHCP(0, NULL, DEFAULT_WAIT_RESP_TIMEOUT);
                r = cmdWA(_state.ssid);
                if (!r && _state.dhcp) {
                    wait_ms(1000);
                    r = cmdNDHCP(_state.dhcp, NULL, CFG_TIMEOUT);
                }
                break;
            }
        }
        break;
    case SEC_WPA_ENT:
    case SEC_WPA2_ENT:
        cmdWAUTH(0);
        DBG("Can't support security\r\n");
        break;
    case SEC_WPS_BUTTON:
        cmdNDHCP(false);
        if (! _state.dhcp) {
            cmdNSET(_state.ip, _state.netmask, _state.ip);
        }
        cmdWAUTH(0);
        r = cmdWWPS(true);
        if (r) break;
        if (_state.dhcp) {
            r = cmdNDHCP(_state.dhcp, _state.name);
        }
        cmdWSTATUS();
        cmdWPAPSK(_state.ssid, _state.pass);
        break;
    case SEC_WPS_PIN:
        cmdNDHCP(false);
        if (! _state.dhcp) {
            cmdNSET(_state.ip, _state.netmask, _state.ip);
        }
        cmdWAUTH(0);
        r = cmdWWPS(true, _state.pass);
        if (r) break;
        if (_state.dhcp) {
            r = cmdNDHCP(_state.dhcp, _state.name);
        }
        cmdWSTATUS();
        cmdWPAPSK(_state.ssid, _state.pass);
        break;
    default:
        DBG("Can't use security\r\n");
        break;
    }

    if (!r) {
        // connected
        if (! _state.dhcp) {
            cmdDNSSET(_state.nameserver);
        }
        _state.associated = true;
#ifdef CFG_ENABLE_RTOS
        _threadPoll = new Thread(&threadPoll);
//        _threadPoll = new Thread(&threadPoll, NULL, osPriorityLow);
#endif
    }
    return r;
}

int GSwifi::adhock ()
{
    bool r = -1;

    _state.wm = WM_ADHOCK;
    _state.initialized = true;
    clearFlags();
    _state.mode = MODE_COMMAND;
    sendCommand(NULL, RES_NULL, 0);
    if (cmdE(false)) return -1;
    if (_flow) {
        cmdR(true);
    }
    if (cmdNMAC()) return -1;
    cmdBDATA(true);
    cmdWREGDOMAIN(CFG_WREGDOMAIN);
    cmdWM(1); // adhock
    wait_ms(100);

    cmdNDHCP(false);
    if (! _state.dhcp) {
        cmdNSET(_state.ip, _state.netmask, _state.ip);
    }

    switch (_state.sec) {
    case SEC_NONE:
    case SEC_OPEN:
    case SEC_WEP:
        cmdWAUTH(_state.sec);
        if (_state.sec != SEC_NONE) {
            cmdWWEP(1, _state.pass);
            wait_ms(100);
        }
        r = cmdWA(_state.ssid);
        break;
    default:
        DBG("Can't use security\r\n");
        break;
    }

    if (!r) {
        // connected
        _state.associated = true;
    }
    return r;
}

int GSwifi::limitedap ()
{
    bool r = -1;

    _state.wm = WM_LIMITEDAP;
    _state.initialized = true;
    if (!strlen(_state.name)) {
        strncpy(_state.name, CFG_DNSNAME, sizeof(_state.name));
    }
    clearFlags();
    _state.mode = MODE_COMMAND;
    sendCommand(NULL, RES_NULL, 0);
    if (cmdE(false)) return -1;
    if (_flow) {
        cmdR(true);
    }
    if (cmdNMAC()) return -1;
    cmdBDATA(true);
    cmdWREGDOMAIN(CFG_WREGDOMAIN);
    cmdWM(2); // limitedap
    wait_ms(100);

    cmdNDHCP(false);
    if (! _state.dhcp) {
        cmdNSET(_state.ip, _state.netmask, _state.ip);
        cmdDNSSET(_state.ip);
    }
    cmdDHCPSRVR(true);
    cmdDNS(true, _state.name);

    switch (_state.sec) {
    case SEC_NONE:
    case SEC_OPEN:
    case SEC_WEP:
        cmdWAUTH(_state.sec);
        if (_state.sec != SEC_NONE) {
            cmdWWEP(1, _state.pass);
            wait_ms(100);
        }
        r = cmdWA(_state.ssid);
        break;
    default:
        DBG("Can't use security\r\n");
        break;
    }

    if (!r) {
        // connected
        _state.associated = true;
    }
    return r;
}

int GSwifi::dissociate()
{
    int i;

    // if already disconnected, return
    if (!_state.associated)
        return 0;

#ifdef CFG_ENABLE_RTOS
    if (_threadPoll) {
        _threadPoll->terminate();
        delete _threadPoll;
    }
#endif
    for (i = 0; i < 16; i ++) {
        if (_con[i].buf) {
            _con[i].buf->flush();
        }
    }
    cmdNCLOSEALL();
    cmdWD();
    cmdNDHCP(false);
    wait_ms(100);

    _state.associated = false;
    return 0;

}

bool GSwifi::isAssociated()
{
    return _state.associated;
}

void GSwifi::poll () {
#ifndef CFG_ENABLE_RTOS
    static int t = 0;
    static Timer timer;

    if (_state.wm == WM_INFRASTRUCTURE && (! isAssociated()) && _state.ssid) {
      if (t == 0 || timer.read() > CFG_RECONNECT) {
        // Wi-Fi re-associate
        if (_state.initialized) {
            if (cmdWA(_state.ssid)) {
                timer.reset();
                timer.start();
                t = 1;
            } else {
                _state.associated = true;
                timer.stop();
                t = 0;
            }
        } else {
            if (join()) {
                timer.reset();
                timer.start();
                t = 1;
            } else {
                timer.stop();
                t = 0;
            }
        }
      }
    }
#else
    if (_state.wm == WM_INFRASTRUCTURE && (! isAssociated()) && _state.ssid) {
        // Wi-Fi re-associate
        if (_state.initialized) {
            if (!cmdWA(_state.ssid)) {
                _state.associated = true;
            }
        } else {
            join();
        }
    }
#endif

    for (int i = 0; i < 16; i ++) {
        if (isConnected(i) && _con[i].received && _con[i].buf != NULL)
          if (_con[i].func != NULL && !_con[i].buf->isEmpty()) {
            _con[i].func(i);
            if (_con[i].buf->isEmpty()) {
                _con[i].received = false;
            }
        }
    }

#ifdef CFG_ENABLE_HTTPD
    httpdPoll();
#endif
}

#ifdef CFG_ENABLE_RTOS
void GSwifi::threadPoll (void const *args) {
    GSwifi * _wifi = GSwifi::getInstance();
    if (_wifi == NULL)
        error("Socket constructor error: no wifly instance available!\r\n");

    INFO("threadPoll");
    for (;;) {
        Thread::signal_wait(1);
        Thread::wait(1000);
        INFO("disassociated");
        while (!_wifi->isAssociated()){
            _wifi->poll();
            Thread::wait(CFG_RECONNECT * 1000);
        }
    }
}
#endif

GSwifi::Status GSwifi::getStatus () {
    return _state.status;
}

int GSwifi::setMacAddress (const char *mac) {
    if (cmdNMAC(mac)) return -1;
    strncpy(_state.mac, mac, sizeof(_state.mac));
    return 0;
}

int GSwifi::getMacAddress (char *mac) {
    if (cmdNMAC()) return -1;
    strcpy(mac, _state.mac);
    return 0;
}

int GSwifi::setAddress (const char *name) {
    _state.dhcp = true;
    strncpy(_state.name, name, sizeof(_state.name));
    return 0;
}

int GSwifi::setAddress (const char *ip, const char *netmask, const char *gateway, const char *dns, const char *name) {
    _state.dhcp = false;
    strncpy(_state.ip, ip, sizeof(_state.ip));
    strncpy(_state.netmask, netmask, sizeof(_state.netmask));
    strncpy(_state.gateway, gateway, sizeof(_state.gateway));
    if (dns) {
        strncpy(_state.nameserver, dns, sizeof(_state.nameserver));
    }
    if (name) {
        strncpy(_state.name, name, sizeof(_state.name));
    }
    return 0;
}

int GSwifi::getAddress (char *ip, char *netmask, char *gateway) {
    strcpy(ip, _state.ip);
    strcpy(netmask, _state.netmask);
    strcpy(gateway, _state.gateway);
    return 0;
}

int GSwifi::setSsid (Security sec, const char *ssid, const char *phrase) {
    int i;

    _state.sec = sec;

    // change all ' ' in '$' in the ssid and the passphrase
    strncpy(_state.ssid, ssid, sizeof(_state.ssid));
    for (i = 0; i < strlen(_state.ssid); i++) {
        if (_state.ssid[i] == ' ')
            _state.ssid[i] = '$';
    }

    if (sec == SEC_WEP) {
        if (strlen(phrase) == 5 || strlen(phrase) == 13) {
            for (i = 0; i < strlen(phrase); i++) {
                _state.pass[i * 2] = '0' + ((phrase[i] >> 4) & 0x0f);
                _state.pass[i * 2 + 1] = '0' + (phrase[i + 1] & 0x0f);
            }
        } else {
            strncpy(_state.pass, phrase, strlen(phrase));
        }
    } else {
        strncpy(_state.pass, phrase, sizeof(_state.pass));
    }
    for (i = 0; i < strlen(_state.pass); i++) {
        if (_state.pass[i] == ' ')
            _state.pass[i] = '$';
    }
    return 0;
}

