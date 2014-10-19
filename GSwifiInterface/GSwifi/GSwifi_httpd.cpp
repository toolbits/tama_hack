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

#ifdef CFG_ENABLE_HTTPD

int GSwifi::httpd (int port) {
    int i;

    if (!isAssociated() || _state.status != STAT_READY) return -1;

    memset(&_httpd, 0, sizeof(_httpd));
    for (i = 0; i < CFG_MAX_SOCKETS; i ++) {
        _httpd[i].mode = HTTPDMODE_REQUEST;
        _httpd[i].buf = _con[i].buf;
    }
#ifdef CFG_ENABLE_RTOS
#ifdef CFG_EXTENDED_MEMORY2
    int n = CFG_EXTENDED_SIZE2 / CFG_MAX_SOCKETS;
    for (i = 0; i < CFG_MAX_SOCKETS; i ++) {
        _httpd[i].uri = (char*)(CFG_EXTENDED_MEMORY2 + (n * i));
#ifdef CFG_ENABLE_RTOS
        _httpd[i].websocket_key = (char*)malloc(30);
#endif
    }
#else // CFG_EXTENDED_MEMORY
    for (i = 0; i < CFG_MAX_SOCKETS; i ++) {
        _httpd[i].uri = (char*)malloc(CFG_CMD_SIZE);
#ifdef CFG_ENABLE_RTOS
        _httpd[i].websocket_key = (char*)malloc(30);
#endif
    }
#endif // CFG_EXTENDED_MEMORY
#endif
    _handler_count = 0;

    _httpd_cid = listen(PROTO_TCP, port);
    if (_httpd_cid < 0) return -1;

    _con[_httpd_cid].protocol = PROTO_HTTPD;
    return _httpd_cid;
}

void GSwifi::httpdRecvData (int cid, char c) {

    switch (_httpd[cid].mode) {
    case HTTPDMODE_REQUEST:
#ifndef CFG_ENABLE_RTOS
        if (_con[cid].buf == NULL) {
            _con[cid].buf = new CircBuffer<char>(CFG_DATA_SIZE);
        }
#endif
        _httpd[cid].buf = _con[cid].buf;
        if (_httpd[cid].buf != NULL) {
            _httpd[cid].buf->flush();
        }
        _httpd[cid].keepalive = 0;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            if (_httpd[cid].buf != NULL) {
                _httpd[cid].buf->queue(c);
            }
            _httpd[cid].mode = HTTPDMODE_REQUEST_STR;
        }
        break;
    case HTTPDMODE_REQUEST_STR:
        switch (c) {
        case 0:
        case 0x0d: // CR
            break;
        case 0x0a: // LF
            if (httpdParseRequest(cid)) {
                _httpd[cid].mode = HTTPDMODE_REQUEST_STR;
            } else {
                _httpd[cid].mode = HTTPDMODE_HEADER;
            }
            _httpd[cid].enter = 0;
            break;
        default:
            if (_httpd[cid].buf != NULL) {
                _httpd[cid].buf->queue(c);
            }
            break;
        }
        break;
    case HTTPDMODE_HEADER:
        switch (c) {
        case 0:
        case 0x0d: // CR
            break;
        case 0x0a: // LF
//            if (_httpd[cid].buf != NULL && _httpd[cid].buf->available() == 0) {
//                if ((_httpd[cid].enter == 0x0d && c == 0x0a) || (_httpd[cid].enter == 0x0a && c == 0x0a)) {
                if (_httpd[cid].enter == 0x0a && c == 0x0a) {
                    if (_httpd[cid].buf != NULL) {
                        _httpd[cid].buf->flush();
                    }
#ifdef CFG_ENABLE_WEBSOCKET
                    if (_httpd[cid].websocket) {
                        INFO("MODE_WEBSOCKET_BEGIN");
                        _httpd[cid].mode = HTTPDMODE_WEBSOCKET_BEGIN;
                    } else
#endif
                    if (_httpd[cid].length) {
                        INFO("MODE_BODY");
                        _httpd[cid].mode = HTTPDMODE_BODY;
                    } else {
                        INFO("MODE_ENTER");
                        _httpd[cid].mode = HTTPDMODE_ENTER;
                        _con[cid].received = true;
                    }
                }
//                break;
//            }

            _httpd[cid].enter = c;
            httpdParseHeader(cid);
            break;
        default:
            if (_httpd[cid].buf != NULL) {
                _httpd[cid].buf->queue(c);
            }
            _httpd[cid].enter = 0;
            break;
        }
        break;
    case HTTPDMODE_BODY:
        if (_httpd[cid].buf != NULL) {
            _httpd[cid].buf->queue(c);
            if (_httpd[cid].buf->available() >= _httpd[cid].length) {
                _httpd[cid].mode = HTTPDMODE_ENTER;
                _con[cid].received = true;
            }
        }
        break;
#ifdef CFG_ENABLE_WEBSOCKET
    case HTTPDMODE_WEBSOCKET:
    case HTTPDMODE_WEBSOCKET_MASK:
    case HTTPDMODE_WEBSOCKET_BODY:
        wsRecvData(cid, c);
        break;
#endif
    }
}

void GSwifi::httpdPoll () {
    for (int cid = 0; cid < 16; cid ++) {
        if (isConnected(cid) && _con[cid].protocol == PROTO_HTTPD) {

    if (_httpd[cid].mode == HTTPDMODE_ENTER) {
        int i = httpdGetHandler(_httpd[cid].uri);
        if (i >= 0) { 
            if (_httpd_handler[i].dir) {
                // file
                httpdFile(cid, _httpd_handler[i].dir);
            } else
            if (_httpd_handler[i].func && !_httpd_handler[i].ws) {
                // cgi
                _httpd_handler[i].func(cid);
//                _httpd[cid].keepalive = 0;
            } else {
                httpdError(cid, 403);
            }
        } else {
            httpdError(cid, 404);
        }

        if (_httpd[cid].keepalive) {
            DBG("keepalive %d", _httpd[cid].keepalive);
            _httpd[cid].keepalive --;
        } else {
            close(cid);
        }
        _httpd[cid].mode = HTTPDMODE_REQUEST;
#ifdef CFG_ENABLE_WEBSOCKET
    } else
    if (_httpd[cid].mode == HTTPDMODE_WEBSOCKET_BEGIN) {
        wsAccept(cid);
        _httpd[cid].mode = HTTPDMODE_WEBSOCKET;
    } else
    if (_httpd[cid].mode == HTTPDMODE_WEBSOCKET_ENTER) {
        wsParseRequest(cid);
        _httpd[cid].mode = HTTPDMODE_WEBSOCKET;
#endif
    }

        } // PROTO_HTTPD
    } // for
}

int GSwifi::httpdParseRequest (int cid) {
    int i, j, len;
    char buf[CFG_CMD_SIZE];

    if (_httpd[cid].buf == NULL) return 0;
    for (len = 0; len < sizeof(buf); len++) {
        if (_httpd[cid].buf->dequeue(&buf[len]) == false) break;
    }
    buf[len] = 0;

    if (strnicmp(buf, "GET ", 4) == 0) {
        _httpd[cid].req = REQ_HTTPGET;
        j = 4;
    } else
    if (strnicmp(buf, "POST ", 5) == 0) {
        _httpd[cid].req = REQ_HTTPPOST;
        j = 5;
    } else {
        return -1;
    }
#ifndef CFG_ENABLE_RTOS
    if (_httpd[cid].uri == NULL)
        _httpd[cid].uri = (char*)malloc(CFG_CMD_SIZE);
#endif
    for (i = 0; i < len - j; i ++) {
        _httpd[cid].uri[i] = buf[i + j];
        if (buf[i + j] == ' ' || i >= CFG_CMD_SIZE - 1) {
            _httpd[cid].uri[i] = 0;
            INFO("URI %d '%s'", _httpd[cid].req, _httpd[cid].uri);
            _httpd[cid].mode = HTTPDMODE_HEADER;
            if (_httpd[cid].buf != NULL) {
                _httpd[cid].buf->flush();
            }
            _httpd[cid].length = 0;
            _httpd[cid].n = 0;
            _httpd[cid].filename = NULL;
            _httpd[cid].querystring = NULL;
#ifdef CFG_ENABLE_WEBSOCKET
            _httpd[cid].websocket = 0;
#endif
            break;
        }
    }

    i = httpdGetHandler(_httpd[cid].uri);
    if (i >= 0) { 
        _httpd[cid].filename = &_httpd[cid].uri[strlen(_httpd_handler[i].uri)];
        for (i = 0; i < strlen(_httpd[cid].filename); i ++) {
            if (_httpd[cid].filename[i] == '?') {
                _httpd[cid].filename[i] = 0;
                _httpd[cid].querystring = _httpd[cid].filename + i + 1;
                break;
            }
        }
        INFO("FILE '%s' QUERY '%s'", _httpd[cid].filename, _httpd[cid].querystring);
    }
    return 0;
}

#define HEADER_TABLE_NUM 5
int GSwifi::httpdParseHeader (int cid) {
    int i;
    char buf[CFG_CMD_SIZE];
    static const struct HEADER_TABLE {
        const char header[24];
        void (GSwifi::*func)(int id, const char*);
    } header_table[HEADER_TABLE_NUM] = {
      {"Content-Length:",         &GSwifi::reqContentLength},
      {"Connection:",             &GSwifi::reqConnection},
      {"Upgrade: websocket",      &GSwifi::reqUpgrade},
      {"Sec-WebSocket-Version:",  &GSwifi::reqWebSocketVersion},
      {"Sec-WebSocket-Key:",      &GSwifi::reqWebSocketKey},
    };

    if (_httpd[cid].buf == NULL) return 0;
    for (i = 0; i < sizeof(buf); i++) {
        if (_httpd[cid].buf->dequeue(&buf[i]) == false) break;
    }
    buf[i] = 0;

    for (i = 0; i < HEADER_TABLE_NUM; i ++) {
        if (strnicmp(buf, header_table[i].header, strlen(header_table[i].header)) == 0) {
            DBG("parse header %d '%s'\r\n", i, buf);
            if (header_table[i].func != NULL) {
                (this->*(header_table[i].func))(cid, buf);
            }
            return 0;
        }
    }

    return -1;
}

void GSwifi::reqContentLength (int cid, const char *buf) {
    _httpd[cid].length = atoi(&buf[16]);
}

void GSwifi::reqConnection (int cid, const char *buf) {
    if (strnicmp(&buf[12], "Keep-Alive", 10) == 0) {
        if (_httpd[cid].keepalive == 0)
            _httpd[cid].keepalive = CFG_HTTPD_KEEPALIVE;
    } else {
        _httpd[cid].keepalive = 0;
    }
}

#ifdef CFG_ENABLE_WEBSOCKET
void GSwifi::reqUpgrade (int cid, const char *buf) {
    if (! _httpd[cid].websocket) _httpd[cid].websocket = 1;
}

void GSwifi::reqWebSocketVersion (int cid, const char *buf) {
    _httpd[cid].websocket = atoi(&buf[23]);
}

void GSwifi::reqWebSocketKey (int cid, const char *buf) {
#ifndef CFG_ENABLE_RTOS
    if (_httpd[cid].websocket_key == NULL) {
        _httpd[cid].websocket_key = (char*)malloc(30);
    }
#endif
    strncpy(_httpd[cid].websocket_key, &buf[19], 30);
}
#else
void GSwifi::reqUpgrade (int cid, const char *buf) {
}
void GSwifi::reqWebSocketVersion (int cid, const char *buf) {
}
void GSwifi::reqWebSocketKey (int cid, const char *buf) {
}
#endif

#endif
