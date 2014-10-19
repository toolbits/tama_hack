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

#ifdef CFG_ENABLE_SMTP

int GSwifi::mail (const char *host, int port, const char *to, const char *from, const char *subject, const char *mesg, const char *user, const char *pwd) {
    int ret = -1;
    int cid;
    char cmd[CFG_DATA_SIZE];
    char ip[17];

    if (!isAssociated() || _state.status != STAT_READY) return -1;

    if (getHostByName(host, ip)) return -1;
    if (! port) {
        port = 25;
    }

    cid = open(PROTO_TCP, ip, port);
    if (cid < 0) return -1;
    DBG("cid %d\r\n", cid);

    if (smtpWait(cid ,220)) goto exit;

    // send request
    snprintf(cmd, sizeof(cmd), "EHLO %s\r\n", _state.name);
    send(cid, cmd, strlen(cmd));
    wait_ms(100);
    if (smtpWait(cid ,250)) goto quit;
    smtpWait(cid ,0);

    if (user && pwd) {
        // smtp auth
        int len;
        snprintf(cmd, sizeof(cmd), "%s%c%s%c%s", user, 0, user, 0, pwd);
        len = strlen(user) * 2 + strlen(pwd) + 2;
        char tmp[len + (len / 2)];
        base64encode(cmd, len, tmp, sizeof(tmp));
        snprintf(cmd, sizeof(cmd), "AUTH PLAIN %s\r\n", tmp);
        send(cid, cmd, strlen(cmd));
        if (smtpWait(cid ,235)) goto quit;
    }
 
    snprintf(cmd, sizeof(cmd), "MAIL FROM: %s\r\n", from);
    send(cid, cmd, strlen(cmd));
    if (smtpWait(cid ,250)) goto quit;
 
    snprintf(cmd, sizeof(cmd), "RCPT TO: %s\r\n", to);
    send(cid, cmd, strlen(cmd));
    if (smtpWait(cid ,250)) goto quit;
 
    strcpy(cmd, "DATA\r\n");
    send(cid, cmd, strlen(cmd));
    if (smtpWait(cid ,354)) goto quit;
 
    // mail data
    snprintf(cmd, sizeof(cmd), "From: %s\r\n", from);
    send(cid, cmd, strlen(cmd));
    snprintf(cmd, sizeof(cmd), "To: %s\r\n", to);
    send(cid, cmd, strlen(cmd));
    snprintf(cmd, sizeof(cmd), "Subject: %s\r\n\r\n", subject);
    send(cid, cmd, strlen(cmd));

    send(cid, mesg, strlen(mesg));
    strcpy(cmd, "\r\n.\r\n");
    send(cid, cmd, strlen(cmd));
    if (smtpWait(cid ,250)) goto quit;
    ret = 0;

    INFO("Mail, from: %s, to: %s %d\r\n", from, to, strlen(mesg));

quit:
    strcpy(cmd, "QUIT\r\n");
    send(cid, cmd, strlen(cmd));
    smtpWait(cid ,221);
exit:
    close(cid);
    return ret;
}

int GSwifi::smtpWait (int cid, int code) {
    Timer timeout;
    int i, n, len = 0;
    char buf[CFG_CMD_SIZE], data[CFG_CMD_SIZE];

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
    DBG("smtp: %s\r\n", data);
    timeout.stop();
 
    // check return code
    i = atoi(data);
    DBG("smtp status %d\r\n", i);
    return i == code ? 0 : -1;
}

#endif
