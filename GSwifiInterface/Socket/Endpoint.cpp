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
#include "Socket/Socket.h"
#include "Socket/Endpoint.h"
#include <cstring>

using std::memset;

Endpoint::Endpoint()  {
    _ewifi = GSwifi::getInstance();
    if (_ewifi == NULL)
        error("Endpoint constructor error: no wifly instance available!\r\n");
    reset_address();
}
Endpoint::~Endpoint() {}

void Endpoint::reset_address(void) {
    _ipAddress[0] = '\0';
    _port = 0;
}

int Endpoint::set_address(const char* host, const int port) {
    //Resolve DNS address or populate hard-coded IP address
    _port = port;
    return _ewifi->getHostByName(host, _ipAddress);
}

char* Endpoint::get_address() {
    return _ipAddress;
}

int   Endpoint::get_port() {
    return _port;
}
