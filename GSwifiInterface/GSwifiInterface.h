/* GSwifiInterface.h */
/* EthernetInterface.h */
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
 
#ifndef GSWIFIINTERFACE_H_
#define GSWIFIINTERFACE_H_

#include "GSwifi.h"

 /** Interface using GSwifi to connect to an IP-based network
 *
 */
class GSwifiInterface: public GSwifi {
public:

    /**
    * Constructor
    *
    * \param tx mbed pin to use for tx line of Serial interface
    * \param rx mbed pin to use for rx line of Serial interface
    * \param cts mbed pin to use for cts line of Serial interface
    * \param rts mbed pin to use for rts line of Serial interface
    * \param reset reset pin of the wifi module
    * \param alarm alarm pin of the wifi module (default: NC)
    * \param baud baud rate of Serial interface (default: 9600)
    */
  GSwifiInterface(PinName tx, PinName rx, PinName cts, PinName rts, PinName reset, PinName alarm = NC, int baud = 9600);

  /** Initialize the interface with DHCP.
  * Initialize the interface and configure it to use DHCP (no connection at this point).
  * \return 0 on success, a negative number on failure
  */
  int init(const char* name = NULL); //With DHCP

  /** Initialize the interface with a static IP address.
  * Initialize the interface and configure it with the following static configuration (no connection at this point).
  * \param ip the IP address to use
  * \param mask the IP address mask
  * \param gateway the gateway to use
  * \return 0 on success, a negative number on failure
  */
  int init(const char* ip, const char* mask, const char* gateway, const char* dns = NULL, const char* name = NULL);

  /** Connect
  * Bring the interface up, start DHCP if needed.
  * \param sec the Wi-Fi security type
  * \param ssid the Wi-Fi SSID
  * \param phrase the Wi-Fi passphrase or security key
  * \param mode the Wi-Fi mode
  * \return 0 on success, a negative number on failure
  */
  int connect(Security sec, const char* ssid, const char* phrase, WiFiMode mode = WM_INFRASTRUCTURE);
  
  /** Disconnect
  * Bring the interface down
  * \return 0 on success, a negative number on failure
  */
  int disconnect();
  
  /** Get the MAC address of your Ethernet interface
   * \return a pointer to a string containing the MAC address
   */
  char* getMACAddress();
  
  /** Get the IP address of your Ethernet interface
   * \return a pointer to a string containing the IP address
   */
  char* getIPAddress();

  /** Get the Gateway address of your Ethernet interface
   * \return a pointer to a string containing the Gateway address
   */
  char* getGateway();
 
  /** Get the Network mask of your Ethernet interface
   * \return a pointer to a string containing the Network mask
   */
  char* getNetworkMask();

};

#include "TCPSocketConnection.h"
#include "TCPSocketServer.h"

#include "Endpoint.h"
#include "UDPSocket.h"

#endif /* GSWIFIINTERFACE_H_ */

