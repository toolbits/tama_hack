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

int GSwifi::httpGet (const char *host, int port, const char *uri, bool ssl, const char *user, const char *pwd, void(*func)(int)) {
    char ip[17];
    int cid;

    if (!isAssociated() || _state.status != STAT_READY) return -1;

    if (getHostByName(host, ip)) return -1;
    if (! port) {
        if (ssl) {
            port = 443;
        } else {
            port = 80;
        }
    }

    if (cmdHTTPCONF(3, "close")) return -1; // Connection
    cmdHTTPCONFDEL(5);
    cmdHTTPCONFDEL(7);
    cmdHTTPCONF(11, host); // Host
    if (user && pwd) {
        char tmp[CFG_CMD_SIZE], tmp2[CFG_CMD_SIZE];
        snprintf(tmp, sizeof(tmp), "%s:%s", user, pwd);
        base64encode(tmp, strlen(tmp), tmp2, sizeof(tmp2));
        sprintf(tmp, "Basic %s", tmp2);
        cmdHTTPCONF(2, tmp); // Authorization
    } else {
        cmdHTTPCONFDEL(2);
    }

    _state.cid = -1;
    if (cmdHTTPOPEN(ip, port, ssl)) return -1;
    if (_state.cid < 0) return -1;
    cid = _state.cid;
    _con[cid].protocol = PROTO_HTTPGET;
    _con[cid].type = TYPE_CLIENT;
    _con[cid].func = func;

    cmdHTTPSEND(cid, false, uri); // GET
    return cid;
}

int GSwifi::httpPost (const char *host, int port, const char *uri, const char *body, bool ssl, const char *user, const char *pwd, void(*func)(int)) {
    char cmd[CFG_CMD_SIZE];
    char ip[17];
    int cid, len;

    if (!isAssociated() || _state.status != STAT_READY) return -1;

    if (getHostByName(host, ip)) return -1;
    if (! port) {
        if (ssl) {
            port = 443;
        } else {
            port = 80;
        }
    }
    len = strlen(body);

    if (cmdHTTPCONF(3, "close")) return -1; // Connection
    sprintf(cmd, "%d", len);
    cmdHTTPCONF(5, cmd); // Content-Length
    cmdHTTPCONF(7, "application/x-www-form-urlencoded"); // Content-type
    cmdHTTPCONF(11, host); // Host
    if (user && pwd) {
        char tmp[CFG_CMD_SIZE], tmp2[CFG_CMD_SIZE];
        snprintf(tmp, sizeof(tmp), "%s:%s", user, pwd);
        base64encode(tmp, strlen(tmp), tmp2, sizeof(tmp2));
        sprintf(tmp, "Basic %s", tmp2);
        cmdHTTPCONF(2, tmp); // Authorization
    } else {
        cmdHTTPCONFDEL(2);
    }

    _state.cid = -1;
    if (cmdHTTPOPEN(ip, port, ssl)) return -1;
    if (_state.cid < 0) return -1;
    cid = _state.cid;
    _con[cid].protocol = PROTO_HTTPPOST;
    _con[cid].type = TYPE_CLIENT;
    _con[cid].func = func;

    cmdHTTPSEND(cid, true, uri, len); // POST
    sprintf(cmd, "\x1bH%X", cid);
    sendData(body, len, DEFAULT_WAIT_RESP_TIMEOUT, cmd);
    return cid;
}


/* base64encode code from 
 * Copyright (c) 2010 Donatien Garnier (donatiengar [at] gmail [dot] com)
 */
int GSwifi::base64encode (const char *input, int length, char *output, int len) {
    static const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned int c, c1, c2, c3;

    if (len < ((((length-1)/3)+1)<<2)) return -1;
    for(unsigned int i = 0, j = 0; i<length; i+=3,j+=4) {
        c1 = ((((unsigned char)*((unsigned char *)&input[i]))));
        c2 = (length>i+1)?((((unsigned char)*((unsigned char *)&input[i+1])))):0;
        c3 = (length>i+2)?((((unsigned char)*((unsigned char *)&input[i+2])))):0;

        c = ((c1 & 0xFC) >> 2);
        output[j+0] = base64[c];
        c = ((c1 & 0x03) << 4) | ((c2 & 0xF0) >> 4);
        output[j+1] = base64[c];
        c = ((c2 & 0x0F) << 2) | ((c3 & 0xC0) >> 6);
        output[j+2] = (length>i+1)?base64[c]:'=';
        c = (c3 & 0x3F);
        output[j+3] = (length>i+2)?base64[c]:'=';
    }
    output[(((length-1)/3)+1)<<2] = '\0';
    return 0;
}


/* urlencode code from 
 * Copyright (c) 2010 Donatien Garnier (donatiengar [at] gmail [dot] com)
 */
int GSwifi::urlencode (const char *str, char *buf, int len) {
//  char *pstr = str, *buf = (char*)malloc(strlen(str) * 3 + 1), *pbuf = buf;
    const char *pstr = str;
    char *pbuf = buf;

    if (len < (strlen(str) * 3 + 1)) return -1;
    while (*pstr) {
        if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~') 
            *pbuf++ = *pstr;
        else if (*pstr == ' ') 
            *pbuf++ = '+';
        else 
            *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
        pstr++;
    }
    *pbuf = '\0';
    return 0;
}

/* urldecode code from 
 * Copyright (c) 2010 Donatien Garnier (donatiengar [at] gmail [dot] com)
 */
int GSwifi::urldecode (const char *str, char *buf, int len) {
//  char *pstr = str, *buf = (char*)malloc(strlen(str) + 1), *pbuf = buf;
    const char *pstr = str;
    char *pbuf = buf;

    if (len < ((int)strlen(str) / 3 - 1)) return -1;
    while (*pstr) {
        if (*pstr == '%') {
            if (pstr[1] && pstr[2]) {
                *pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
                pstr += 2;
            }
        } else if (*pstr == '+') { 
            *pbuf++ = ' ';
        } else {
            *pbuf++ = *pstr;
        }
        pstr++;
    }
    *pbuf = '\0';
    return 0;
}
