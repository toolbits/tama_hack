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

int GSwifi::x2i (char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    } else
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return 0;
}

char GSwifi::i2x (int i) {
    if (i >= 0 && i <= 9) {
        return i + '0';
    } else
    if (i >= 10 && i <= 15) {
        return i - 10 + 'A';
    }
    return 0;
}

int GSwifi::from_hex (int ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

int GSwifi::to_hex (int code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}


int GSwifi::setRegion (int reg) {
    return cmdWREGDOMAIN(reg);
}

int GSwifi::getRssi () {
    cmdWRSSI();
    return _state.rssi;
}

int GSwifi::powerSave (int active, int save) {
    cmdWRXACTIVE(active);
    return cmdWRXPS(save);
}

int GSwifi::setRfPower (int power) {
    if (power < 0 || power > 7) return false;
    return cmdWP(power);
}

int GSwifi::setTime (time_t time) {
    char dmy[16], hms[16];
    struct tm *t = localtime(&time);

    sprintf(dmy, "%d/%d/%d", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900);
    sprintf(hms, "%d:%d:%d", t->tm_hour, t->tm_min, t->tm_sec);
    return cmdSETTIME(dmy, hms);
}

time_t GSwifi::getTime () {
    cmdGETTIME();
    return _state.time;
}

int GSwifi::ntpdate (char *host, int sec) {
    char ip[17];

    if (!isAssociated() || _state.status != STAT_READY) return -1;

    if (getHostByName(host, ip)) return -1;
    return cmdNTIMESYNC(true, ip, sec);
}

int GSwifi::setGpio (int port, int out) {
    return cmdDGPIO(port, out);
}

int GSwifi::provisioning (char *user, char *pass) {

    if (!isAssociated() || _state.status != STAT_READY) return -1;

    return cmdWEBPROV(user, pass);
}

int GSwifi::standby (int msec) {
    switch (_state.status) {
    case STAT_READY:
        cmdNCLOSEALL();
        for (int i = 0; i < 16; i ++) {
            _con[i].connected = false;
        }
        cmdSTORENWCONN();
//        cmdWRXACTIVE(false);
        break;
    case STAT_WAKEUP:
        if (cmdE(false)) return -1;
        if (_flow) {
            cmdR(true);
        }
        break;
    default:
        return -1;
    }

    _state.status = STAT_STANDBY;
    return cmdPSSTBY(msec, 0);
}


int GSwifi::deepSleep () {
    if (_state.status != STAT_READY) return -1;

    _state.status = STAT_DEEPSLEEP;
    return cmdPSDPSLEEP(); // go deep sleep
}

int GSwifi::wakeup () {
    int r;

    if (_state.status == STAT_STANDBY) {
        Timer timeout;
        setAlarm(true);
        timeout.start();
        while (timeout.read() < CFG_TIMEOUT) {
            if (_state.status == STAT_WAKEUP) break;
        }
        timeout.stop();
        setAlarm(false);
    }

    switch (_state.status) {
    case STAT_WAKEUP:
        if (cmdE(false)) return -1;
        if (_flow) {
            cmdR(true);
        }
        cmdBDATA(true);
        r = cmdRESTORENWCONN();
        if (!r) {
            wait_ms(100);
//            cmdWRXACTIVE(true);
            _state.status = STAT_READY;
        }
        return r;
    case STAT_DEEPSLEEP:
        r = cmdAT();
        if (!r) {
            _state.status = STAT_READY;
        }
        return r;
    default:
        break;
    }
    return -1;
}
