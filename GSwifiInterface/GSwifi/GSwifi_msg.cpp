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

#include "GSwifi.h"

#ifdef CFG_ENABLE_RTOS
#undef DBG
#define DBG(x, ...)
#endif

void GSwifi::recvData (char c) {
    static int cid, sub, len, count;

#ifdef DEBUG_DUMP
    if (c < 0x20 || c >= 0x7f) {
        std::printf("_%02x", c);
    } else {
        std::printf("_%c", c);
    }
#endif
    switch (_state.mode) {
    case MODE_COMMAND:
        switch (c) {
        case 0:
        case 0x0a: // LF
        case 0x0d: // CR
            break;
        case 0x1b: // ESC
            _state.buf->flush();
            _state.mode = MODE_ESCAPE;
            break;
        default:
            _state.buf->flush();
            _state.buf->queue(c);
            _state.mode = MODE_CMDRESP;
            break;
        }
        break;
    
    case MODE_CMDRESP:
        switch (c) {
        case 0:
            break;
        case 0x0a: // LF
        case 0x0d: // CR
//            _state.buf->queue(c);
            if (_flow == 2) setRts(false);
            _state.mode = MODE_COMMAND;
            parseMessage();
            if (_flow == 2) setRts(true);
            break;
        case 0x1b: // ESC
            _state.mode = MODE_ESCAPE;
            break;
        default:
            _state.buf->queue(c);
            break;
        }
        break;

    case MODE_ESCAPE:
        sub = 0;
        switch (c) {
        case 'H':
            _state.mode = MODE_DATA_RXHTTP;
            break;
        case 'u':
            _state.mode = MODE_DATA_RXUDP;
            break;
        case 'y':
            _state.mode = MODE_DATA_RXUDP_BULK;
            break;
        case 'Z':
            _state.mode = MODE_DATA_RX_BULK;
            break;
        case 'S':
            _state.mode = MODE_DATA_RX;
            break;
        case ':':
            _state.mode = MODE_DATA_RAW;
            break;
        case 'O':
            _state.mode = MODE_COMMAND;
            _state.ok = true;
            return;
        case 'F':
            _state.mode = MODE_COMMAND;
            _state.failure = true;
            return;
        default:
            _state.mode = MODE_COMMAND;
            return;
        }
        break;
    
    case MODE_DATA_RX:
    case MODE_DATA_RXUDP:
        switch (sub) {
        case 0:
            // cid
            cid = x2i(c);
            sub ++;
            count = 0;
            if (_state.mode == MODE_DATA_RX) {
                sub = 3;
            }
            break;
        case 1:
            // ip
            if ((c >= '0' && c <= '9') || c == '.') {
                _con[cid].ip[count] = c;
                count ++;
            } else {
                _con[cid].ip[count] = 0;
                _con[cid].port = 0;
                sub ++;
            }
            break;
        case 2:
            // port
            if (c >= '0' && c <= '9') {
                _con[cid].port = (_con[cid].port * 10) + (c - '0'); 
            } else {
                sub ++;
                count = 0;
            }
            break;            
        default:
            // data
            if (_state.escape) {
                if (c == 'E') {
                    DBG("recv ascii %d %d/a\r\n", cid, count);
                    _con[cid].received = true;
                    _state.mode = MODE_COMMAND;
                } else {
                    if (_con[cid].buf != NULL) {
                        _con[cid].buf->queue(0x1b);
                        _con[cid].buf->queue(c);
                    }
                    count += 2;
                }
                _state.escape = false;
            } else
            if (c == 0x1b) {
                _state.escape = true;
            } else {
                if (_con[cid].buf != NULL) {
                    _con[cid].buf->queue(c);
                    if (_con[cid].func != NULL && _con[cid].buf->available() > CFG_DATA_SIZE - 16) {
                        setRts(false);
                        _con[cid].received = true;
                        WARN("buf full");
                    }
                }
                count ++;
            }
            break;
        }
        break;
    
    case MODE_DATA_RX_BULK:
    case MODE_DATA_RXUDP_BULK:
    case MODE_DATA_RXHTTP:
        switch (sub) {
        case 0:
            // cid
            cid = x2i(c);
            sub ++;
            len = 0;
            count = 0;
            if (_state.mode != MODE_DATA_RXUDP_BULK) {
                sub = 3;
            }
            break;
        case 1:
            // ip
            if ((c >= '0' && c <= '9') || c == '.') {
                _con[cid].ip[count] = c;
                count ++;
            } else {
                _con[cid].ip[count] = 0;
                _con[cid].port = 0;
                sub ++;
            }
            break;
        case 2:
            // port
            if (c >= '0' && c <= '9') {
                _con[cid].port = (_con[cid].port * 10) + (c - '0'); 
            } else {
                sub ++;
                count = 0;
            }
            break;
        case 3:
            // length
            len = (len * 10) + (c - '0');
            count ++;
            if (count >= 4) {
                sub ++;
                count = 0;
            }
            break;
        default:
            // data
#ifdef CFG_ENABLE_HTTPD
            if (_con[cid].protocol == PROTO_HTTPD) {
                httpdRecvData(cid, c);
            } else
#endif
            if (_con[cid].buf != NULL) {
                _con[cid].buf->queue(c);
                if (_con[cid].func != NULL && _con[cid].buf->available() > CFG_DATA_SIZE - 16) {
                    setRts(false);
                    _con[cid].received = true;
                    WARN("buf full");
                }
            }
            count ++;
            if (count >= len) {
                DBG("recv bulk %d %d/%d\r\n", cid, count, len);
                _con[cid].received = true;
                _state.mode = MODE_COMMAND;
            }
            break;
        }
        break;
    }
}

#define MSG_TABLE_NUM 15
#define RES_TABLE_NUM 11
int GSwifi::parseMessage () {
    int i;
    char buf[256];
    static const struct MSG_TABLE {
        const char msg[24];
        void (GSwifi::*func)(const char*);
    } msg_table[MSG_TABLE_NUM] = {
      {"OK",                      &GSwifi::msgOk},
      {"ERROR",                   &GSwifi::msgError},
      {"INVALID INPUT",           &GSwifi::msgError},
      {"CONNECT ",                &GSwifi::msgConnect},
      {"DISCONNECT ",             &GSwifi::msgDisconnect},
      {"DISASSOCIATED",           &GSwifi::msgDisassociated},
      {"Disassociated",           &GSwifi::msgDisassociated},
      {"Disassociation Event",    &GSwifi::msgDisassociated},
      {"Serial2WiFi APP",         &GSwifi::msgReset},
      {"UnExpected Warm Boot",    &GSwifi::msgReset},
      {"APP Reset-APP SW Reset",  &GSwifi::msgReset},
      {"APP Reset-Wlan Except",   &GSwifi::msgReset},
      {"Out of StandBy-Timer",    &GSwifi::msgOutofStandby},
      {"Out of StandBy-Alarm",    &GSwifi::msgOutofStandby},
      {"Out of Deep Sleep",       &GSwifi::msgOutofDeepsleep},
    };
    static const struct RES_TABLE {
        const Response res;
        void (GSwifi::*func)(const char*);
    } res_table[RES_TABLE_NUM] = {
      {RES_NULL,        NULL},
      {RES_CONNECT,     &GSwifi::resConnect},
      {RES_WPAPSK,      &GSwifi::resWpapsk},
      {RES_WPS,         &GSwifi::resWps},
      {RES_MACADDRESS,  &GSwifi::resMacAddress},
      {RES_DHCP,        &GSwifi::resIp},
      {RES_DNSLOOKUP,   &GSwifi::resLookup},
      {RES_HTTP,        &GSwifi::resHttp},
      {RES_RSSI,        &GSwifi::resRssi},
      {RES_TIME,        &GSwifi::resTime},
      {RES_STATUS,      &GSwifi::resStatus},
    };

    for (i = 0; i < sizeof(buf); i++) {
        if (_state.buf->dequeue(&buf[i]) == false) break;
    }
    buf[i] = 0;

    if (_state.res != RES_NULL) {
      for (i = 0; i < RES_TABLE_NUM; i ++) {
        if (res_table[i].res == _state.res) {
            DBG("parse res %d '%s'\r\n", i, buf);
            if (res_table[i].func != NULL) {
                (this->*(res_table[i].func))(buf);
            }
        }
      }
    }

    for (i = 0; i < MSG_TABLE_NUM; i ++) {
        if (strncmp(buf, msg_table[i].msg, strlen(msg_table[i].msg)) == 0) {
            DBG("parse msg %d '%s'\r\n", i, buf);
            if (msg_table[i].func != NULL) {
                (this->*(msg_table[i].func))(buf);
            }
            return 0;
        }
    }

    return -1;
}

void GSwifi::msgOk (const char *buf) {
    _state.ok = true;
    if (_state.status == STAT_DEEPSLEEP) {
        _state.status = STAT_READY;
    }
}

void GSwifi::msgError (const char *buf) {
    _state.failure = true;
}

void GSwifi::msgConnect (const char *buf) {
    int i, count;
    int cid, acid;

    if (buf[8] < '0' || buf[8] > 'F' || buf[9] != ' ') return;

    cid = x2i(buf[8]);
    acid = x2i(buf[10]);
    DBG("forked %d -> %d\r\n", cid, acid);
    // ip
    count = 0;
    for (i = 12; i < strlen(buf); i ++) {
        if ((buf[i] >= '0' && buf[i] <= '9') || buf[i] == '.') {
            _con[acid].ip[count] = buf[i];
            count ++;
        } else {
            _con[acid].ip[count] = 0;
            break;
        }
    }
    // port
    _con[acid].port = 0;
    count = 0;
    for (; i < strlen(buf); i ++) {
        if (buf[i] >= '0' && buf[i] <= '9') {
            _con[acid].port = (_con[acid].port * 10) + (buf[i] - '0'); 
        } else {
            break;
        }
    }

    // initialize
    initCon(acid, true);
    _con[acid].protocol = _con[cid].protocol;
    _con[acid].type = _con[cid].type;
    _con[acid].parent = cid;
    _con[acid].func = _con[cid].func;
    _con[acid].accept = true;

#ifdef CFG_ENABLE_HTTPD
    if (_con[acid].protocol == PROTO_HTTPD) {
        _httpd[acid].mode = HTTPDMODE_REQUEST;
#ifdef CFG_ENABLE_WEBSOCKET
        _httpd[acid].websocket = 0;
#endif
    }
#endif
}

void GSwifi::msgDisconnect (const char *buf) {
    int cid;

    if (buf[11] < '0' || buf[11] > 'F') return;

    cid = x2i(buf[11]);
    DBG("disconnect %d\r\n", cid);
    _con[cid].connected = false;
}

void GSwifi::msgDisassociated (const char *buf) {
    int i;
    DBG("disassociate\r\n");
    _state.associated = false;
    for (i = 0; i < 16; i ++) {
        _con[i].connected = false;
    }
#ifdef CFG_ENABLE_RTOS
    if (_threadPoll)
        _threadPoll->signal_set(1);
#endif
}

void GSwifi::msgReset (const char *buf) {
    DBG("reset\r\n");
    _state.initialized = false;
    _state.mode = MODE_COMMAND;
    _state.status = STAT_READY;
    msgDisassociated(NULL);
    clearFlags();
#ifdef CFG_ENABLE_RTOS
    if (_threadPoll) {
        _threadPoll->terminate();
        delete _threadPoll;
    }
#endif
}

void GSwifi::msgOutofStandby (const char *buf) {
    DBG("OutofStandby\r\n");
    _state.status = STAT_WAKEUP;
}

void GSwifi::msgOutofDeepsleep (const char *buf) {
    DBG("OutofDeepsleep\r\n");
    _state.status = STAT_READY;
}

void GSwifi::resConnect (const char *buf) {
    int cid;

    // udp/tcp listen socket
    if (strncmp(buf, "CONNECT ", 8) == 0 && buf[9] == 0) {
        cid = x2i(buf[8]);
        DBG("connect %d\r\n", cid);
        // initialize
        initCon(cid, true);
        _state.cid = cid;
        _state.res = RES_NULL;
    }
}

void GSwifi::resWpapsk (const char *buf) {
    if (strncmp(buf, "Computing PSK from SSID and PassPhrase", 38) == 0) {
        _state.res = RES_NULL;
        DBG("wpapsk\r\n");
    }
}

void GSwifi::resWps (const char *buf) {
    if (_state.n == 0 && strncmp(buf, "SSID", 4) == 0) {
        strncpy(_state.ssid, &buf[5], sizeof(_state.ssid));
        _state.n ++;
    } else
    if (_state.n == 1 && strncmp(buf, "CHANNEL", 7) == 0) {
        _state.n ++;
    } else
    if (_state.n == 2 && strncmp(buf, "PASSPHRASE", 10) == 0) {
        strncpy(_state.pass, &buf[11], sizeof(_state.pass));
        _state.n ++;
        _state.res = RES_NULL;
        DBG("wps %s %s\r\n", _state.ssid, _state.pass);
    }
}

void GSwifi::resMacAddress (const char *buf) {
    if (buf[2] == ':' && buf[5] == ':') {
        strncpy(_state.mac, buf, sizeof(_state.mac));
        _state.mac[17] = 0;
        _state.res = RES_NULL;
        DBG("mac %s\r\n", _state.mac);
    }
}

void GSwifi::resIp (const char *buf) {
    const char *tmp, *tmp2;

    if (_state.n == 0 && strstr(buf, "SubNet") && strstr(buf, "Gateway")) {
        _state.n ++;
    } else
    if (_state.n == 1) {
        tmp = buf + 1;
        tmp2 = strstr(tmp, ":");
        strncpy(_state.ip, tmp, tmp2 - tmp);
        tmp = tmp2 + 2;
        tmp2 = strstr(tmp, ":");
        strncpy(_state.netmask, tmp, tmp2 - tmp);
        tmp = tmp2 + 2;
        strncpy(_state.gateway, tmp, sizeof(_state.gateway));
        _state.n ++;
        _state.res = RES_NULL;
        DBG("ip: %s\r\nnetmask: %s\r\ngateway: %s", _state.ip, _state.netmask, _state.gateway);
    }
}

void GSwifi::resLookup (const char *buf) {
    if (strncmp(buf, "IP:", 3) == 0) {
        strncpy(_state.resolv, &buf[3], sizeof(_state.resolv));
        _state.res = RES_NULL;
        DBG("resolv: %s\r\n", _state.resolv);
    }
}

void GSwifi::resRssi (const char *buf) {
    if (buf[0] == '-' || (buf[0] >= '0' && buf[0] <= '9')) {
        _state.rssi = atoi(buf);
        _state.res = RES_NULL;
        DBG("rssi: %d\r\n", _state.rssi);
    }
}

void GSwifi::resTime (const char *buf) {
    int year, month, day, hour, min, sec;
    struct tm t;
    if (buf[0] >= '0' && buf[0] <= '9') {
        sscanf(buf, "%d/%d/%d,%d:%d:%d", &day, &month, &year, &hour, &min, &sec);
        t.tm_sec = sec;
        t.tm_min = min;
        t.tm_hour = hour;
        t.tm_mday = day;
        t.tm_mon = month - 1;
        t.tm_year = year - 1900;   
        _state.time = mktime(&t);            
        _state.res = RES_NULL;
    }
}

void GSwifi::resChannel (const char *buf) {
}

void GSwifi::resStatus (const char *buf) {
    if (_state.n == 0 && strncmp(buf, "NOT ASSOCIATED", 14) == 0) {
        msgDisassociated(NULL);
        _state.res = RES_NULL;
    }

    if (_state.n == 0 && strncmp(buf, "MODE:", 5) == 0) {
        _state.n ++;
    } else
    if (_state.n == 1 && strncmp(buf, "BSSID:", 6) == 0) {
        const char *tmp = strstr(buf, "SECURITY:") + 2;
        if (strncmp(tmp, "WEP (OPEN)", 10) == NULL) {
            _state.sec = SEC_OPEN;
        } else
        if (strncmp(tmp, "WEP (SHARED)", 12) == NULL) {
            _state.sec = SEC_WEP;
        } else
        if (strncmp(tmp, "WPA-PERSONAL", 12) == NULL) {
            _state.sec = SEC_WPA_PSK;
        } else
        if (strncmp(tmp, "WPA2-PERSONAL", 13) == NULL) {
            _state.sec = SEC_WPA2_PSK;
        }
        _state.res = RES_NULL;
    }
}

void GSwifi::resHttp (const char *buf) {
    int cid;

    // http client socket
    if (buf[0] >= '0' && buf[0] <= 'F' && buf[1] == 0) {
        cid = x2i(buf[0]);
        DBG("connect %d\r\n", cid);
        // initialize
        initCon(cid, true);
        _state.cid = cid;
        _state.res = RES_NULL;
    }
}
