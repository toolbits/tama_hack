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

void GSwifi::clearFlags () {
    _state.ok = false;
    _state.failure = false;
    _state.res = RES_NULL;
    _state.n = 0;
}

int GSwifi::sendCommand(const char * cmd, Response res, int timeout) {
    int i;
    Timer t;

    if (lockUart(timeout)) return -1;

    clearFlags();
    _state.res = res;
    for (i = 0; i < strlen(cmd); i ++) {
        putUart(cmd[i]);
    }
    putUart('\r');
    putUart('\n');
    unlockUart();
    INFO("command: '%s'\r\n", cmd);

    if (timeout) {
        t.start();
        for (;;) {
            if (_state.ok && _state.res == RES_NULL) break;
            if (_state.failure || t.read_ms() > timeout) {
                WARN("failure or timeout\r\n");
                _state.res = RES_NULL;
                return -1;
            }
        }
        t.stop();
    }
    INFO("ok\r\n");
    _state.res = RES_NULL;

    return 0;
}

int GSwifi::sendData(const char * data, int len, int timeout, const char * cmd) {
    int i;
    Timer t;

    if (lockUart(timeout)) return -1;

    clearFlags();
    for (i = 0; i < strlen(cmd); i ++) {
        putUart(cmd[i]);
    }
    for (i = 0; i < len; i ++) {
        putUart(data[i]);
    }
    unlockUart();
    INFO("data: '%s' %d\r\n", cmd, len);

    if (timeout) {
        t.start();
        for (;;) {
            if (_state.ok) break;
            if (_state.failure || t.read_ms() > timeout) {
                WARN("failure or timeout\r\n");
                return -1;
            }
        }
        t.stop();
    }

    return i;
}


int GSwifi::cmdAT () {
    return sendCommand("AT");
}

int GSwifi::cmdE (bool n) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "ATE%d", n ? 1 : 0);
    return sendCommand(cmd);
}

int GSwifi::cmdR (bool n) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT&R%d", n ? 1 : 0);
    return sendCommand(cmd);
}

int GSwifi::cmdNMAC (const char *s) {
    int r;
    char cmd[CFG_CMD_SIZE];
    const char xmac[] = "00:1D:C9:01:99:99";
    if (s) {
        sprintf(cmd, "AT+NMAC2=%s", s);
        r = sendCommand(cmd);
    } else {
        sprintf(cmd, "AT+NMAC=?");
        r = sendCommand(cmd, RES_MACADDRESS);
        if (!r && strncmp(_state.mac, xmac, 17) == 0) {
            r = -1;
        }
    }
    return r;
}

int GSwifi::cmdWREGDOMAIN (int n) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+WREGDOMAIN=%d", n);
    return sendCommand(cmd);
}

int GSwifi::cmdWS () {
    return sendCommand("AT+WS");
}

int GSwifi::cmdWM (int n) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+WM=%d", n);
    return sendCommand(cmd);
}

int GSwifi::cmdWA (const char *s) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+WA=%s", s);
    return sendCommand(cmd, RES_DHCP, CFG_TIMEOUT);
}

int GSwifi::cmdWD () {
    return sendCommand("AT+WD");
}

int GSwifi::cmdWWPS (bool n, const char *p) {
    char cmd[CFG_CMD_SIZE];
    if (p) {
        sprintf(cmd, "AT+WWPS=2,%s", p);
    } else {
        sprintf(cmd, "AT+WWPS=%d", n ? 1 : 0);
    }
    return sendCommand(cmd, RES_WPS, CFG_TIMEOUT);
}

int GSwifi::cmdNSTAT () {
    return sendCommand("AT+NSTAT=?");
}

int GSwifi::cmdWSTATUS () {
    return sendCommand("AT+WSTATUS", RES_STATUS);
}

int GSwifi::cmdWRSSI () {
    return sendCommand("AT+WRSSI=?", RES_RSSI);
}

int GSwifi::cmdWAUTH (int n) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+WAUTH=%d", n);
    return sendCommand(cmd);
}

int GSwifi::cmdWWEP (int n, const char *s) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+WWEP%d=%s", n, s);
    return sendCommand(cmd);
}

int GSwifi::cmdWPAPSK (const char *s, const char *p) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+WPAPSK=%s,%s", s, p);
    return sendCommand(cmd, RES_WPAPSK, CFG_TIMEOUT);
}

int GSwifi::cmdWRXACTIVE (bool n) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+WRXACTIVE=%d", n ? 1 : 0);
    return sendCommand(cmd);
}

int GSwifi::cmdWRXPS (bool n) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+WRXPS=%d", n ? 1 : 0);
    return sendCommand(cmd);
}

int GSwifi::cmdWP (int n) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+WP=%d", n);
    return sendCommand(cmd);
}

int GSwifi::cmdNDHCP (bool n, const char *s, int m) {
    char cmd[CFG_CMD_SIZE];
    if (n) {
        if (s) {
            sprintf(cmd, "AT+NDHCP=1,%s", s);
        } else {
            sprintf(cmd, "AT+NDHCP=1");
        }
        return sendCommand(cmd, RES_DHCP, m);
    } else {
        return sendCommand("AT+NDHCP=0");
    }
}

int GSwifi::cmdDHCPSRVR (bool n) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+DHCPSRVR=%d", n ? 1 : 0);
    return sendCommand(cmd);
}

int GSwifi::cmdNSET (const char *s, const char *t, const char *u) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+NSET=%s,%s,%s", s, t, u);
    return sendCommand(cmd);
}

int GSwifi::cmdDNS (bool n, const char *s) {
    char cmd[CFG_CMD_SIZE];
    if (n) {
        if (s) {
            sprintf(cmd, "AT+DNS=1,%s", s);
        } else {
            sprintf(cmd, "AT+DNS=1," CFG_DNSNAME);
        }
    } else {
        sprintf(cmd, "AT+DNS=0");
    }
    return sendCommand(cmd);
}

int GSwifi::cmdDNSLOOKUP (const char *s) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+DNSLOOKUP=%s", s);
    return sendCommand(cmd, RES_DNSLOOKUP, CFG_TIMEOUT);
}

int GSwifi::cmdDNSSET (const char *s) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+DNSSET=%s", s);
    return sendCommand(cmd);
}

int GSwifi::cmdSTORENWCONN () {
    return sendCommand("AT+STORENWCONN");
}

int GSwifi::cmdRESTORENWCONN () {
    return sendCommand("AT+RESTORENWCONN");
}

int GSwifi::cmdBDATA (bool n) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+BDATA=%d", n ? 1 : 0);
    return sendCommand(cmd);
}

int GSwifi::cmdNCTCP (const char *s, int n) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+NCTCP=%s,%d", s, n);
    return sendCommand(cmd, RES_CONNECT, CFG_TIMEOUT);
}

int GSwifi::cmdNCUDP (const char *s, int n, int m) {
    char cmd[CFG_CMD_SIZE];
    if (m) {
        sprintf(cmd, "AT+NCUDP=%s,%d,%d", s, n, m);
    } else {
        sprintf(cmd, "AT+NCUDP=%s,%d", s, n);
    }
    return sendCommand(cmd, RES_CONNECT, CFG_TIMEOUT);
}

int GSwifi::cmdNSTCP (int n) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+NSTCP=%d", n);
    return sendCommand(cmd, RES_CONNECT);
}

int GSwifi::cmdNSUDP (int n) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+NSUDP=%d", n);
    return sendCommand(cmd, RES_CONNECT);
}

int GSwifi::cmdNCLOSE (int n) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+NCLOSE=%X", n);
    return sendCommand(cmd, RES_NULL, CFG_TIMEOUT);
}

int GSwifi::cmdNCLOSEALL () {
    return sendCommand("AT+NCLOSEALL", RES_NULL, CFG_TIMEOUT);
}

int GSwifi::cmdHTTPCONF (int n, const char *s) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+HTTPCONF=%d,%s", n, s);
    return sendCommand(cmd);
}

int GSwifi::cmdHTTPCONFDEL (int n) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+HTTPCONFDEL=%d", n);
    return sendCommand(cmd);
}

int GSwifi::cmdHTTPOPEN (const char *s, int n, bool m) {
    char cmd[CFG_CMD_SIZE];
    if (m) {
        sprintf(cmd, "AT+HTTPOPEN=%s,%d,1", s, n);
    } else {
        sprintf(cmd, "AT+HTTPOPEN=%s,%d", s, n);
    }
    return sendCommand(cmd, RES_HTTP, CFG_TIMEOUT);
}

int GSwifi::cmdHTTPSEND (int n, bool m, const char *s, int t) {
    char cmd[CFG_CMD_SIZE];
    if (m) {
        sprintf(cmd, "AT+HTTPSEND=%X,3,%d,%s,%d", n, CFG_TIMEOUT / 1000, s, t);
    } else {
        sprintf(cmd, "AT+HTTPSEND=%X,1,%d,%s", n, CFG_TIMEOUT / 1000, s);
    }
    return sendCommand(cmd);
}

int GSwifi::cmdHTTPCLOSE (int n) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+HTTPCLOSE=%X", n);
    return sendCommand(cmd, RES_NULL, CFG_TIMEOUT);
}

int GSwifi::cmdPSDPSLEEP (int n) {
    char cmd[CFG_CMD_SIZE];
    if (n) {
        sprintf(cmd, "AT+PSDPSLEEP=%d", n);
    } else {
        sprintf(cmd, "AT+PSDPSLEEP");
    }
    return sendCommand(cmd, RES_NULL, 0);
}

int GSwifi::cmdPSSTBY (int n, int m) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+PSSTBY=%d,0,%d,%d", n, m, m);
    return sendCommand(cmd, RES_NULL, 0);
}

int GSwifi::cmdWEBPROV (const char *s, const char *p) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+WEBPROV=%s,%s", s, p);
    return sendCommand(cmd);
}

int GSwifi::cmdSETTIME (const char *s, const char *t) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+SETTIME=%s,%s", s, t);
    return sendCommand(cmd);
}

int GSwifi::cmdGETTIME () {
    return sendCommand("AT+GETTIME=?", RES_TIME);
}

int GSwifi::cmdNTIMESYNC (bool n, const char *s, int m) {
    char cmd[CFG_CMD_SIZE];
    if (n) {
        if (m) {
            sprintf(cmd, "AT+NTIMESYNC=1,%s,%d,1,%d", s, CFG_TIMEOUT / 1000, m);
        } else {
            sprintf(cmd, "AT+NTIMESYNC=1,%s,%d,0", s, CFG_TIMEOUT / 1000);
        }
    } else {
        sprintf(cmd, "AT+NTIMESYNC=0");
    }
    return sendCommand(cmd, RES_NULL, CFG_TIMEOUT);
}

int GSwifi::cmdDGPIO (int n, int m) {
    char cmd[CFG_CMD_SIZE];
    sprintf(cmd, "AT+DGPIO=%d,%d", n, m);
    return sendCommand(cmd);
}
