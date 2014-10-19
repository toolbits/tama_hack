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

int GSwifi::httpdFile (int cid, char *dir) {
    FILE *fp;
    int i, len;
    char buf[256];
    char file[80];

    INFO("httpdFile %d %s", cid, dir);

    strcpy(file, dir);
    strncat(file, _httpd[cid].filename, sizeof(file) - strlen(file) - 1);
    if (file[strlen(file) - 1] == '/') {
        strncat(file, "index.html", sizeof(file) - strlen(file) - 1);
    }
    DBG("file: %s\r\n", file);

    fp = fopen(file, "r");
    if (fp) {
        strcpy(buf, "HTTP/1.1 200 OK\r\n");
        send(cid, buf, strlen(buf));
        {
            // file size
            i = ftell(fp);
            fseek(fp, 0, SEEK_END);
            len = ftell(fp);
            fseek(fp, i, SEEK_SET);
        }

        strcpy(buf, "Server: GSwifi httpd\r\n");
        send(cid, buf, strlen(buf));
        if (_httpd[cid].keepalive) {
            strcpy(buf, "Connection: Keep-Alive\r\n");
        } else {
            strcpy(buf, "Connection: close\r\n");
        }
        send(cid, buf, strlen(buf));
        sprintf(buf, "Content-Type: %s\r\n", mimetype(file));
        send(cid, buf, strlen(buf));
        sprintf(buf, "Content-Length: %d\r\n\r\n", len);
        send(cid, buf, strlen(buf));

        for (;;) {
            i = fread(buf, sizeof(char), sizeof(buf), fp);
            if (i <= 0) break;
            send(cid, buf, i);
#if defined(TARGET_LPC1768) || defined(TARGET_LPC2368)
            if (feof(fp)) break;
#endif
        }
        fclose(fp);
        return 0;
    }

    httpdError(cid, 404);
    return -1;
}

void GSwifi::httpdError (int cid, int err) {
    char buf[CFG_CMD_SIZE], msg[30];
    
    switch (err) {
    case 400:
        strcpy(msg, "Bad Request");
        break;
    case 403:
        strcpy(msg, "Forbidden");
        break;
    case 404:
        strcpy(msg, "Not Found");
        break;
    case 500:
    default:
        strcpy(msg, "Internal Server Error");
        break;
    }
    DBG("httpd error: %d %d %s\r\n", cid, err, msg);
    
    sprintf(buf, "HTTP/1.1 %d %s\r\n", err, msg);
    send(cid, buf, strlen(buf));
    strcpy(buf, "Content-Type: text/html\r\n\r\n");
    send(cid, buf, strlen(buf));

    sprintf(buf, "<html><head><title>%d %s</title></head>\r\n", err, msg);
    send(cid, buf, strlen(buf));
    sprintf(buf, "<body><h1>%s</h1></body></html>\r\n", msg);
    send(cid, buf, strlen(buf));
    wait_ms(100);
    close(cid);
//    WARN("%d.%d.%d.%d ", _httpd[cid].host.getIp()[0], _httpd[cid].host.getIp()[1], _httpd[cid].host.getIp()[2], _httpd[cid].host.getIp()[3]);
//    WARN("%s %s %d %d -\r\n", _httpd[cid].type == GSPROT_HTTPGET ? "GET" : "POST", _httpd[cid].uri, _httpd[cid].length, err);
}

int GSwifi::httpdGetHandler (const char *uri) {
    int i;
 
    for (i = 0; i < _handler_count; i ++) {
        if (strncmp(uri, _httpd_handler[i].uri, strlen(_httpd_handler[i].uri)) == NULL) {
            // found
            return i;
        }
    }
    return -1;
}

int GSwifi::httpdAttach (const char *uri, const char *dir) {
    if (_handler_count < CFG_HTTPD_HANDLER_NUM) {
        _httpd_handler[_handler_count].uri = (char*)malloc(strlen(uri) + 1);
        strcpy(_httpd_handler[_handler_count].uri, uri);
        _httpd_handler[_handler_count].dir = (char*)malloc(strlen(dir) + 1);
        strcpy(_httpd_handler[_handler_count].dir, dir);
        _httpd_handler[_handler_count].func = NULL;
        _httpd_handler[_handler_count].ws = 0;
        DBG("httpdAttach %s %s\r\n", _httpd_handler[_handler_count].uri, _httpd_handler[_handler_count].dir);
        _handler_count ++;
        return 0;
    } else {
        return -1;
    }
}

int GSwifi::httpdAttach (const char *uri, void (*funcCgi)(int), int ws) {
    if (_handler_count < CFG_HTTPD_HANDLER_NUM) {
        _httpd_handler[_handler_count].uri = (char*)malloc(strlen(uri) + 1);
        strcpy(_httpd_handler[_handler_count].uri, uri);
        _httpd_handler[_handler_count].dir = NULL;
        _httpd_handler[_handler_count].func = funcCgi;
        _httpd_handler[_handler_count].ws = ws;
        DBG("httpdAttach %s %08x\r\n", _httpd_handler[_handler_count].uri, _httpd_handler[_handler_count].func);
        _handler_count ++;
        return 0;
    } else {
        return -1;
    }
}

const char *GSwifi::httpdGetFilename (int cid) {
    return _httpd[cid].filename;
}

const char *GSwifi::httpdGetQuerystring (int cid) {
    return _httpd[cid].querystring;
}

#define MIMETABLE_NUM 9
char *GSwifi::mimetype (char *file) {
    static const struct MIMETABLE {
        const char ext[5];
        const char type[24];
    } mimetable[MIMETABLE_NUM] = {
        {"txt", "text/plain"},  // default
        {"html", "text/html"},
        {"htm", "text/html"},
        {"css", "text/css"},
        {"js", "application/javascript"},
        {"jpg", "image/jpeg"},
        {"png", "image/png"},
        {"gif", "image/gif"},
        {"ico", "image/x-icon"},
    };
    int i, j;

    for (i = 0; i < MIMETABLE_NUM; i ++) {
        j = strlen(mimetable[i].ext);
        if (file[strlen(file) - j - 1] == '.' &&
          strnicmp(&file[strlen(file) - j], mimetable[i].ext, j) == NULL) {
            return (char*)mimetable[i].type;
        }
    }
    return (char*)mimetable[0].type;
}

int GSwifi::strnicmp (const char *p1, const char *p2, int n) {
    int i, r = -1;
    char c1, c2;
    
    for (i = 0; i < n; i ++) {
        c1 = (p1[i] >= 'a' && p1[i] <= 'z') ? p1[i] - ('a' - 'A'): p1[i];
        c2 = (p2[i] >= 'a' && p2[i] <= 'z') ? p2[i] - ('a' - 'A'): p2[i];
        r = c1 - c2;
        if (r) break;
    }
    return r;
}
#endif
