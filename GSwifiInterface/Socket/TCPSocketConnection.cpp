/* Copyright (C) 2012 mbed.org, MIT License
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
/* Copyright (C) 2013 gsfan, MIT License
 *  port to the GainSpan Wi-FI module GS1011
 */

#include "TCPSocketConnection.h"
#include <algorithm>

TCPSocketConnection::TCPSocketConnection() {}

int TCPSocketConnection::connect(const char* host, const int port)
{  
    if (set_address(host, port) != 0) return -1;

    _server = false;
    _cid = _wifi->open(GSwifi::PROTO_TCP, get_address(), get_port());
    if (_cid < 0) return -1;

    return 0;
}

bool TCPSocketConnection::is_connected(void)
{
    bool _is_connected = _wifi->isConnected(_cid);
    if (!_is_connected) _cid = -1;
    return _is_connected;
}

int TCPSocketConnection::send(char* data, int length)
{
    if (_cid < 0 || !is_connected()) return -1;

    // TCP Client/Server
    return _wifi->send(_cid, data, length);
}

// -1 if unsuccessful, else number of bytes written
int TCPSocketConnection::send_all(char* data, int length)
{
    Timer tmr;
    int idx = 0;

    if (_cid < 0 || !is_connected()) return -1;

    tmr.start();
    // TCP Client/Server
    while ((tmr.read_ms() < _timeout) || _blocking) {

        idx += _wifi->send(_cid, &data[idx], length - idx);
        if (idx < 0) return -1;

        if (idx == length)
            return idx;
    }
    return (idx == 0) ? -1 : idx;
}

// -1 if unsuccessful, else number of bytes received
int TCPSocketConnection::receive(char* data, int length)
{
    Timer tmr;
    int time = -1;

    if (_cid < 0 || !is_connected()) return -1;

    if (_blocking) {
        tmr.start();
        while (time < _timeout + 20) {
            if (_wifi->readable(_cid)) {
                DBG("receive readable");
                break;
            }
            time = tmr.read_ms();
        }
        if (time >= _timeout + 20) {
            DBG("receive timeout");
            return 0;
        }
    }

    int nb_available = _wifi->recv(_cid, data, length);

    return nb_available;
}


// -1 if unsuccessful, else number of bytes received
int TCPSocketConnection::receive_all(char* data, int length)
{
    Timer tmr;
    int idx = 0;
    int time = -1;

    if (_cid < 0 || !is_connected()) return -1;

    tmr.start();
    
    while (time < _timeout || _blocking) {

        idx += _wifi->recv(_cid, &data[idx], length - idx);
        if (idx < 0) return -1;

        if (idx == length)
            break;

        time = tmr.read_ms();
    }

    return (idx == 0) ? -1 : idx;
}

void TCPSocketConnection::acceptCID (int cid) {
    char *ip;
    int port;
    _server = true;
    _cid = cid;
    if (!_wifi->getRemote(_cid, &ip, &port)) {
        set_address(ip, port);
    }
}
