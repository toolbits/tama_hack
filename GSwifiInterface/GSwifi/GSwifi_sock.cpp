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

int GSwifi::getHostByName(const char * host, char * ip)
{
    int i, flg = 0;

    if (!isAssociated() || _state.status != STAT_READY) return -1;

    for (i = 0; i < strlen(host); i ++) {
        if ((host[i] < '0' || host[i] > '9') && host[i] != '.') {
            flg = 1;
            break;
        }
    }
    if (!flg) {
        strncpy(ip, host, 16);
        return 0;
    }

    if (cmdDNSLOOKUP(host)) {
        // retry
        wait_ms(1000);
        if (cmdDNSLOOKUP(host)) return -1;
    }
    strncpy(ip, _state.resolv, 16);
    return 0;
}

int GSwifi::open (Protocol proto, const char *ip, int port, int src, void(*func)(int)) {
    int cid;

    if (!isAssociated() || _state.status != STAT_READY) return -1;

    _state.cid = -1;
    if (proto == PROTO_TCP) {
        if (cmdNCTCP(ip, port)) return -1;
    } else {
        if (cmdNCUDP(ip, port, src)) return -1;
    }
    if (_state.cid < 0) return -1;
    cid = _state.cid;
    _con[cid].protocol = proto;
    _con[cid].type = TYPE_CLIENT;
    _con[cid].func = func;
    return cid;
}

int GSwifi::listen (Protocol proto, int port, void(*func)(int)) {
    int cid;

    if (!isAssociated() || _state.status != STAT_READY) return -1;

    _state.cid = -1;
    if (proto == PROTO_TCP) {
        if (cmdNSTCP(port)) return -1;
    } else {
        if (cmdNSUDP(port)) return -1;
    }
    if (_state.cid < 0) return -1;
    cid = _state.cid;
    _con[cid].protocol = proto;
    _con[cid].type = TYPE_SERVER;
    _con[cid].func = func;
    return cid;
}

int GSwifi::close (int cid) {

    if (!isConnected(cid)) return -1;

    _con[cid].connected = false;
    return cmdNCLOSE(cid);
}

int GSwifi::send (int cid, const char *buf, int len) {
    char cmd[CFG_CMD_SIZE];

    if (!isConnected(cid)) return -1;

    if ((_con[cid].protocol == PROTO_TCP) ||
      (_con[cid].protocol == PROTO_UDP && _con[cid].type == TYPE_CLIENT) ||
      (_con[cid].protocol == PROTO_HTTPD)) {
        if (len > CFG_DATA_SIZE) len = CFG_DATA_SIZE;
        sprintf(cmd, "\x1bZ%X%04d", cid, len);
        return sendData(buf, len, CFG_TIMEOUT, cmd);
    } else {
        return -1;
    }
}

int GSwifi::sendto (int cid, const char *buf, int len, const char *ip, int port) {
    char cmd[CFG_CMD_SIZE];

    if (!isConnected(cid)) return -1;

    if ((_con[cid].protocol == PROTO_UDP && _con[cid].type == TYPE_SERVER)) {
        if (len > CFG_DATA_SIZE) len = CFG_DATA_SIZE;
        sprintf(cmd, "\x1bY%X%s:%d:%04d", cid, ip, port, len);
        return sendData(buf, len, CFG_TIMEOUT, cmd);
    } else {
        return -1;
    }
}

int GSwifi::recv (int cid, char *buf, int len) {
    int i;

    if (!isConnected(cid)) return -1;

    if (_con[cid].buf == NULL) return 0;
    while (!_con[cid].received && _state.mode != MODE_COMMAND);
    _con[cid].received = false;
    for (i = 0; i < len; i ++) {
        if (_con[cid].buf->dequeue(&buf[i]) == false) break;
    }
    setRts(true);
    return i;
}

int GSwifi::recvfrom (int cid, char *buf, int len, char *ip, int *port) {
    int i;

    if (!isConnected(cid)) return -1;

    if (_con[cid].buf == NULL) return 0;
    while (!_con[cid].received && _state.mode != MODE_COMMAND);
    _con[cid].received = false;
    for (i = 0; i < len; i ++) {
        if (_con[cid].buf->dequeue(&buf[i]) == false) break;
    }
    strncpy(ip, _con[cid].ip, 16);
    *port = _con[cid].port;
    setRts(true);
    return i;
}

int GSwifi::readable (int cid) {
    if (!isConnected(cid)) return -1;

    if (_con[cid].buf == NULL) return -1;
    return _con[cid].buf->available();
}

bool GSwifi::isConnected (int cid) {
    if (cid < 0 || cid >= 16) return false;

    return _con[cid].connected;
}

int GSwifi::accept (int cid) {
    int i;

    if (!isConnected(cid)) return -1;

    for (i = 0; i < 16; i ++) {
        if (_con[i].connected && _con[i].accept && _con[i].parent == cid) {
            _con[i].accept = false;
            return i;
        }
    }
    return -1;
}

int GSwifi::getRemote(int cid, char **ip, int *port) {

    if (!isConnected(cid)) return -1;

    *ip = _con[cid].ip;
    *port = _con[cid].port;
    return 0;
}

void GSwifi::initCon (int cid, bool connected) {
    _con[cid].parent = -1;
    _con[cid].func = NULL;
    _con[cid].accept = false;
#ifndef CFG_ENABLE_RTOS
    if (_con[cid].buf == NULL) {
        _con[cid].buf = new CircBuffer<char>(CFG_DATA_SIZE);
        if (_con[cid].buf == NULL) error("Can't allocate memory");
    }
#endif
    if (_con[cid].buf != NULL) {
        _con[cid].buf->flush();
    }
    _con[cid].connected = connected;
}
