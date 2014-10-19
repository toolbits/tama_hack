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

#ifdef CFG_ENABLE_WEBSOCKET

#include "sha1.h"

int GSwifi::wsOpen (const char *host, int port, const char *uri, const char *user, const char *pwd) {
    int cid;
    char cmd[CFG_CMD_SIZE], tmp[CFG_CMD_SIZE];
    char ip[17];

    if (!isAssociated() || _state.status != STAT_READY) return -1;

    if (getHostByName(host, ip)) return -1;
    if (! port) {
        port = 80;
    }

    cid = open(PROTO_TCP, ip, port);
    if (cid < 0) return -1;
    DBG("ws cid %d\r\n", cid);

    // request
    snprintf(cmd, sizeof(cmd), "GET %s HTTP/1.1\r\n", uri);
    send(cid, cmd, strlen(cmd));
    if (host) {
        snprintf(cmd, sizeof(cmd), "Host: %s\r\n", host);
        send(cid, cmd, strlen(cmd));
    }
    if (user && pwd) {
        snprintf(cmd, sizeof(cmd), "%s:%s", user, pwd);
        base64encode(cmd, strlen(cmd), tmp, sizeof(tmp));
        snprintf(cmd, sizeof(cmd), "Authorization: Basic %s\r\n", tmp);
        send(cid, cmd, strlen(cmd));
    }
    strcpy(cmd, "Upgrade: websocket\r\n");
    send(cid, cmd, strlen(cmd));
    strcpy(cmd, "Connection: Upgrade\r\n");
    send(cid, cmd, strlen(cmd));
    getMacAddress(cmd);
    memcpy(&cmd[6], host, 10);
    base64encode(cmd, 16, tmp, sizeof(tmp));
    snprintf(cmd, sizeof(cmd), "Sec-WebSocket-Key: %s\r\n", tmp);
    send(cid, cmd, strlen(cmd));
//    strcpy(cmd, "Sec-WebSocket-Protocol: chat\r\n");
//    send(cid, cmd, strlen(cmd));
    strcpy(cmd, "Sec-WebSocket-Version: 13\r\n\r\n");
    send(cid, cmd, strlen(cmd));

    if (wsWait(cid, 101)) {
        close(cid);
        return -1;
    }
    wsWait(cid, 0);
    return cid;
}

int GSwifi::wsWait (int cid, int code) {
    Timer timeout;
    int i, n, len;
    char buf[CFG_DATA_SIZE], data[CFG_CMD_SIZE];

    if (code == 0) {
        // dummy read
        timeout.start();
        while (timeout.read_ms() < CFG_TIMEOUT) {
            wait_ms(10);
            if (!readable(cid)) break;
            n = recv(cid, buf, sizeof(buf));
            if (n <= 0) break;
        }
        timeout.stop();
        return 0;
    }

    // wait responce
    len = 0;
    timeout.start();
    while (timeout.read_ms() < CFG_TIMEOUT) {
        wait_ms(10);
        n = recv(cid, buf, sizeof(buf));
        for (i = 0; i < n; i ++) {
            if (buf[i] == '\r') continue;
            if (buf[i] == '\n') {
                if (len == 0) continue;
                goto next;
            } else
            if (len < sizeof(data) - 1) {
                data[len] = buf[i];
                len ++;
            }
        }
    }
next:
    data[len] = 0;
    timeout.stop();
    DBG("ws: %s\r\n", data);
 
    // check return code
    if (strncmp(data, "HTTP/1.1 ", 9) != 0) return -1;
    i = atoi(&data[9]);
    DBG("ws status %d\r\n", i);
    return i == code ? 0 : -1;
}

int GSwifi::wsSend (int cid, const char *buf, int len, const char *mask) {
    int i = 0, r;
    char tmp[10];
 
    tmp[i++] = 0x81; // single, text frame
    if (len < 126) {
        tmp[i++] = (mask == NULL ? 0 : 0x80) | len;
    } else {
        tmp[i++] = (mask == NULL ? 0 : 0x80) | 126;
        tmp[i++] = (len >> 8) & 0xff;
        tmp[i++] = len & 0xff;
    }
    if (mask) {
        memcpy(&tmp[i], mask, 4);
        i += 4;
    }
    r = send(cid, tmp, i);
 
    if (r > 0) {
        if (mask) {
            char tmp2[len];
            for (i = 0; i < len; i ++) {
                tmp2[i] = buf[i] ^ mask[i & 0x03];
            }
            r = send(cid, tmp2, len);
        } else {
            r = send(cid, buf, len);
        }
    }
    return r;
}

#ifdef CFG_ENABLE_HTTPD

void GSwifi::wsRecvData (int cid, char c) {

    switch (_httpd[cid].mode) {
    case HTTPDMODE_WEBSOCKET:
        if (_httpd[cid].n == 0) {
            // flag
            _httpd[cid].websocket_opcode = c & 0x0f;
            _httpd[cid].websocket_flg = c << 8;
            _httpd[cid].n ++;
        } else
        if (_httpd[cid].n == 1) {
            // length 7bit
            _httpd[cid].websocket_flg |= c;
            _httpd[cid].length = c & 0x7f;
            _httpd[cid].n ++;
            if (_httpd[cid].length < 126) {
                _httpd[cid].n = 0;
                if (_httpd[cid].length) {
                    if (_httpd[cid].websocket_flg & 0x0080) {
                        _httpd[cid].mode = HTTPDMODE_WEBSOCKET_MASK;
                    } else {
                        _httpd[cid].mode = HTTPDMODE_WEBSOCKET_BODY;
                    }
                } else {
                    _httpd[cid].mode = HTTPDMODE_WEBSOCKET_ENTER;
                }
                DBG("ws length %d\r\n", _httpd[cid].length);
            }
        } else {
            // length 16bit,64bit
            if (_httpd[cid].n == 2) {
                _httpd[cid].length = c;
                _httpd[cid].n ++;
            } else
            if (_httpd[cid].n < 9 && (_httpd[cid].websocket_flg & 0x7f) == 127) {
                // 64bit
                _httpd[cid].length = (_httpd[cid].length << 8) | c;
                _httpd[cid].n ++;
            } else {
                // end
                _httpd[cid].length = (_httpd[cid].length << 8) | c;
                _httpd[cid].n = 0;
                if (_httpd[cid].websocket_flg & 0x0080) {
                    _httpd[cid].mode = HTTPDMODE_WEBSOCKET_MASK;
                } else {
                    _httpd[cid].mode = HTTPDMODE_WEBSOCKET_BODY;
                }
                DBG("ws length2 %d\r\n", _httpd[cid].length);
            }
        }
        break;
    case HTTPDMODE_WEBSOCKET_MASK:
        // masking key
        _httpd[cid].websocket_mask[_httpd[cid].n] = c;
        _httpd[cid].n ++;
        if (_httpd[cid].n >= 4) {
            _httpd[cid].n = 0;
            _httpd[cid].mode = HTTPDMODE_WEBSOCKET_BODY;
            DBG("ws mask\r\n");
        }
        break;
    case HTTPDMODE_WEBSOCKET_BODY:
        // payload
        if (_httpd[cid].websocket_flg & 0x0080) {
            // un-mask
            _httpd[cid].buf->queue(c ^ _httpd[cid].websocket_mask[_httpd[cid].n & 0x03]); 
        } else {
            _httpd[cid].buf->queue(c); 
        }
        _httpd[cid].n ++;
        if (_httpd[cid].n >= _httpd[cid].length) {
            _httpd[cid].mode = HTTPDMODE_WEBSOCKET_ENTER;
            _con[cid].received = true;
        }
        break;
    }
}

int GSwifi::wsParseRequest (int cid) {
    int i;

    DBG("ws opcode %d\r\n", _httpd[cid].websocket_opcode);
    switch (_httpd[cid].websocket_opcode) {
    case 0x00: // continuation
        break;
    case 0x01: // text
    case 0x02: // binary
        i = httpdGetHandler(_httpd[cid].uri);
        if (i >= 0) {
            if (_httpd_handler[i].func && _httpd_handler[i].ws) {
                // cgi
                _httpd_handler[i].func(cid);
            }
        }
        break;
    case 0x08: // close
        close(cid);
        break;
    case 0x09: // ping
        {
        char pong[_httpd[cid].n + 2];
        pong[0] = 0x8a;
        pong[1] = 0x04;
        for (i = 0; i < _httpd[cid].length; i ++) {
            if (_httpd[cid].buf->dequeue(&pong[i + 2]) == false) break;
        }
        send(cid, pong, _httpd[cid].length + 2);
        }
        break;
    case 0x0a: // pong
        break;
    default:
        break;
    }
    _httpd[cid].n = 0;
    _httpd[cid].length = 0;
    return 0;
}

int GSwifi::wsAccept (int cid) {
    char buf[CFG_CMD_SIZE], buf2[CFG_CMD_SIZE];
    
    DBG("websocket accept: %d\r\n", cid);

    strcpy(buf, "HTTP/1.1 101 Switching Protocols\r\n");
    send(cid, buf, strlen(buf));
    strcpy(buf, "Upgrade: websocket\r\n");
    send(cid, buf, strlen(buf));
    strcpy(buf, "Connection: Upgrade\r\n");
    send(cid, buf, strlen(buf));

    strcpy(buf, "Sec-WebSocket-Accept: ");
    send(cid, buf, strlen(buf));
    strcpy(buf, _httpd[cid].websocket_key);
    strcat(buf, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    sha1(buf, strlen(buf), buf2);
    base64encode(buf2, 20, buf, sizeof(buf));
    send(cid, buf, strlen(buf));
    strcpy(buf, "\r\n\r\n");
    send(cid, buf, strlen(buf));
    return 0;
}

#endif
#endif
