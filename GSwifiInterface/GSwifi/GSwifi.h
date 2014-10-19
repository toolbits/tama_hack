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
 *
 * @section DESCRIPTION
 *
 * GainSpan GS1011, Wi-Fi module
 *
 *   http://www.gainspan.com/modules
 */
/** @file
 * @brief Gainspan wi-fi module library for mbed
 * GS1011MIC, GS1011MIP, GainSpan WiFi Breakout, etc.
 */

#ifndef GSwifi_H
#define GSwifi_H

#include "GSwifi_conf.h"

#include "mbed.h"
#include "RawSerial.h"
#include "CBuffer.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#ifdef CFG_ENABLE_RTOS
#include "rtos.h"
#endif

//Debug is disabled by default
#if defined(DEBUG) and (!defined(TARGET_LPC11U24))
#define DBG(x, ...) std::printf("[DBG]" x "\r\n", ##__VA_ARGS__);
#define WARN(x, ...) std::printf("[WARN]" x "\r\n", ##__VA_ARGS__);
#define ERR(x, ...) std::printf("[ERR]" x "\r\n", ##__VA_ARGS__);
#define INFO(x, ...) std::printf("[INFO]" x "\r\n", ##__VA_ARGS__);
#else
#define DBG(x, ...)
#define WARN(x, ...)
#define ERR(x, ...)
#define INFO(x, ...)
#endif


/** GSwifi class
 */
class GSwifi
{

public:

    /** Wi-Fi mode
     */
    enum WiFiMode {
        WM_INFRASTRUCTURE,
        WM_ADHOCK,
        WM_LIMITEDAP,
    };

    /** Wi-Fi security
     */
    enum Security {
        SEC_AUTO = 0,
        SEC_NONE = 0,
        SEC_OPEN = 1,
        SEC_WEP = 2,
        SEC_WPA_PSK = 4,
        SEC_WPA2_PSK = 8,
        SEC_WPA_ENT = 16,
        SEC_WPA2_ENT = 32,
        SEC_WPS_BUTTON = 64,
        SEC_WPS_PIN,
    };

    /** TCP/IP protocol
     */
    enum Protocol {
        PROTO_UDP = 0,
        PROTO_TCP = 1,
        PROTO_HTTPGET,
        PROTO_HTTPPOST,
        PROTO_HTTPD,
    };

    /** Client/Server
     */
    enum Type {
        TYPE_CLIENT = 0,
        TYPE_SERVER = 1,
    };

    enum Response {
        RES_NULL,
        RES_CONNECT,
        RES_WPAPSK,
        RES_WPS,
        RES_MACADDRESS,
        RES_DHCP,
        RES_DNSLOOKUP,
        RES_HTTP,
        RES_RSSI,
        RES_TIME,
        RES_STATUS,
    };

    enum Mode {
        MODE_COMMAND,
        MODE_CMDRESP,
        MODE_ESCAPE,
        MODE_DATA_RX,
        MODE_DATA_RX_BULK,
        MODE_DATA_RXUDP,
        MODE_DATA_RXUDP_BULK,
        MODE_DATA_RXHTTP,
        MODE_DATA_RAW,
    };

    enum Status {
        STAT_NONE,
        STAT_READY,
        STAT_STANDBY,
        STAT_WAKEUP,
        STAT_DEEPSLEEP,
    };

    // ----- GSwifi.cpp -----
    /** Constructor
     * \param tx mbed pin to use for tx line of Serial interface
     * \param rx mbed pin to use for rx line of Serial interface
     * \param cts mbed pin to use for cts line of Serial interface
     * \param rts mbed pin to use for rts line of Serial interface
     * \param reset reset pin of the wifi module
     * \param alarm alarm pin of the wifi module (default NULL)
     * \param baud baud rate of Serial interface (default 9600)
     */
    GSwifi (PinName tx, PinName rx, PinName cts, PinName rts, PinName reset, PinName alarm = NC, int baud = 9600);

    /** Connect the wifi module to the ssid contained in the constructor.
     * @param sec Security type (NONE, WEP_128 or WPA)
     * @param ssid ssid of the network
     * @param phrase WEP or WPA key
     * @return 0 if connected, -1 otherwise
     */
    int join ();

    /** Connect the wifi module to the adhock in the constructor.
     * @param sec Security type (NONE, WEP_128 or WPA)
     * @param ssid ssid of the network
     * @param phrase WEP or WPA key
     * @return 0 if connected, -1 otherwise
     */
    int adhock ();

    /** Connect the wifi module to the limited AP in the constructor.
     * @param sec Security type (NONE, WEP_128 or WPA)
     * @param ssid ssid of the network
     * @param phrase WEP or WPA key
     * @return 0 if connected, -1 otherwise
     */
    int limitedap ();

    /** Disconnect the wifi module from the access point
     * @returns 0 if successful
     */
    int dissociate ();
    int disconnect () {
        return dissociate();
    }
    
    /** Check if a Wi-Fi link is active
     * @returns true if successful
     */
    bool isAssociated();

    /** polling if not use rtos
     */
    void poll ();

    /** get Wi-Fi modue status
     * @return Status
     */
    Status getStatus ();

    /** set MAC address
     */
    int setMacAddress (const char *mac);
    /** get MAC address
     */
    int getMacAddress (char *mac);

    /** use DHCP
     */
    int setAddress (const char *name = NULL);
    /** use static IP address
     * @param ip my ip address (dhcp start address)
     * @param netmask subnet mask
     * @param gateway default gateway
     * @param dns name server (default NULL)
     * @param name my host name (default NULL)
     */
    int setAddress (const char *ip, const char *netmask, const char *gateway, const char *dns = NULL, const char *name = NULL);
    /** get IP address
     */
    int getAddress (char *ip, char *netmask, char *gateway);

    /** set Wi-Fi security parameter
     * @param sec Security
     * @param ssid SSID
     * @param pass pass phrase
     */
    int setSsid (Security sec, const char *ssid, const char *phrase);

    static GSwifi * getInstance() {
        return _inst;
    };

    // ----- GSwifi_util.cpp -----
    int setRegion (int reg);

    /** get RSSI
     * @return RSSI (dBm)
     */
    int getRssi ();

    /** power save mode
     * @param active rx radio 0:switched off, 1:always on
     * @param save power save 0:disable, 1:enable
     */
    int powerSave (int active, int save);

    /** RF power
     * @param power 0(high)-7(low)
     */
    int setRfPower (int power);

    /** set system time
     * @param time date time (UTC)
     */
    int setTime (time_t time);

    /** get system time
     * @return date time (UTC)
     */
    time_t getTime ();

    /** set NTP server
     * @param host SNTP server
     * @param sec time sync interval, 0:one time
     */
    int ntpdate (char *host, int sec = 0);

    /** GPIO output
     * @param port 10,11,30,31
     * @param out 0:set(high), 1:reset(low)
     */
    int setGpio (int port, int out);

    /** Web server
     */
    int provisioning (char *user, char *pass);

    /** standby mode
     * @param msec wakeup after
     * wakeup after msec or alarm1/2
     * core off, save to RTC ram
     */
    int standby (int msec);

    /** deep sleep mode
     */
    int deepSleep ();

    /** restore standby or deep sleep
     */
    int wakeup ();

    // ----- GSwifi_sock.cpp -----
    /** Resolv hostname
     * @param name hostname
     * @param ip resolved ip address
     */
    int getHostByName(const char * host, char * ip);

    /** TCP/UDP client
     * @return CID, -1:failure
     */
    int open (Protocol proto, const char *ip, int port, int src = 0, void(*func)(int) = NULL);

    /** TCP/UDP server
     * @return CID, -1:failure
     */
    int listen (Protocol proto, int port, void(*func)(int) = NULL);

    /** close client/server
     */
    int close (int cid);

    /** send data tcp(s/c), udp(c)
     */
    int send (int cid, const char *buf, int len);

    /** send data udp(s)
     */
    int sendto (int cid, const char *buf, int len, const char *ip, int port);

    /** recv data tcp(s/c), udp(c)
     * @return length
     */
    int recv (int cid, char *buf, int len);

    /** recv data udp(s)
     * @return length
     */
    int recvfrom (int cid, char *buf, int len, char *ip, int *port);

    /** readable recv data
     */
    int readable (int cid);

    /** tcp/udp connected
     */
    bool isConnected (int cid);

    int accept (int cid);
    int getRemote(int cid, char **ip, int *port);

    // ----- GSwifi_http.cpp -----
    /** http request (GET method)
     */
    int httpGet (const char *host, int port, const char *uri, bool ssl = false, const char *user = NULL, const char *pwd = NULL, void(*func)(int) = NULL);
    int httpGet (const char *host, int port, const char *uri, void(*func)(int)) {
        return httpGet(host, port, uri, false, NULL, NULL, func);
    }
    /** http request (POST method)
     */
    int httpPost (const char *host, int port, const char *uri, const char *body, bool ssl = false, const char *user = NULL, const char *pwd = NULL, void(*func)(int) = NULL);
    int httpPost (const char *host, int port, const char *uri, const char *body, void(*func)(int)) {
        return httpPost(host, port, uri, body, false, NULL, NULL, func);
    }

    int base64encode (const char *input, int length, char *output, int len);
    int urlencode (const char *str, char *buf, int len);
    int urldecode (const char *str, char *buf, int len);

#ifdef CFG_ENABLE_HTTPD
    // ----- GSwifi_httpd.cpp -----
    /** start http server
     * @param port
     */
    int httpd (int port = 80);

    // ----- GSwifi_httpd_util.cpp -----
    /** attach uri to dirctory handler
     */
    void httpdError (int cid, int err);
    /** attach uri to dirctory handler
     */
    int httpdAttach (const char *uri, const char *dir);
    /** attach uri to cgi handler
     */
    int httpdAttach (const char *uri, void (*funcCgi)(int), int type = 0);
    
    const char *httpdGetFilename (int cid);
    const char *httpdGetQuerystring (int cid);
#endif

#ifdef CFG_ENABLE_WEBSOCKET
    // ----- GSwifi_httpd_ws.cpp -----
    /** websocket request (Upgrade method)
     * @return CID, -1:failure
     */
    int wsOpen (const char *host, int port, const char *uri, const char *user = NULL, const char *pwd = NULL);
    /** send websocket data
     */
    int wsSend (int cid, const char *buf, int len, const char *mask = NULL);
#endif

#ifdef CFG_ENABLE_SMTP
    // ----- GSwifi_smtp.cpp -----
    /** send mail (smtp)
     * @param host SMTP server
     * @param port SMTP port (25 or 587 or etc.)
     * @param to To address
     * @param from From address
     * @param subject Subject
     * @param mesg Message
     * @param user username (SMTP Auth)
     * @param pwd password (SMTP Auth)
     * @retval 0 success
     * @retval -1 failure
     */
    int mail (const char *host, int port, const char *to, const char *from, const char *subject, const char *mesg, const char *user = NULL, const char *pwd = NULL);
#endif

    // ----- GSwifi_msg.cpp -----

    // ----- GSwifi_at.cpp -----
    /** Send a command to the wifi module. Check if the module is in command mode. If not enter in command mode
     * @param cmd string to be sent
     * @param res need response
     * @param timeout
     * @returns 0 if successful
     */
    int sendCommand(const char * cmd, Response res = RES_NULL, int timeout = DEFAULT_WAIT_RESP_TIMEOUT);

    /** Send a command to the wifi module. Check if the module is in command mode. If not enter in command mode
     * @param data string to be sent
     * @param res need response
     * @param timeout
     * @param cmd 
     * @returns 0 if successful
     */
    int sendData(const char * data, int len, int timeout = CFG_TIMEOUT, const char * cmd = NULL);


protected:

    static GSwifi * _inst;
#ifdef CFG_ENABLE_RTOS
    Thread *_threadPoll;
    Mutex _mutexUart;
#endif

//    Serial _gs;
    RawSerial _gs;
    int _baud;
    DigitalIn *_cts;
    DigitalOut *_rts;
    int _flow;
#if defined(TARGET_LPC1768) || defined(TARGET_LPC2368) || defined(TARGET_LPC4088) || defined(TARGET_LPC176X) || defined(TARGET_LPC408X)
    LPC_UART1_TypeDef *_uart;
#elif defined(TARGET_LPC11U24) || defined(TARGET_LPC11UXX)
    LPC_USART_Type *_uart;
#elif defined(TARGET_LPC11XX)
    LPC_UART_TypeDef *_uart;
#elif defined(TARGET_LPC81X)
    LPC_UART_TypeDef *_uart;
#endif
    DigitalInOut _reset;
    DigitalInOut *_alarm;

    struct STATE {
        WiFiMode wm;
        Security sec;
        char ssid[35];
        char pass[66];
        char ip[16];
        char netmask[16];
        char gateway[16];
        char nameserver[16];
        char mac[18];
        char resolv[16];
        char name[32];
        int rssi;
        bool dhcp;
        time_t time;

        bool initialized;
        bool associated;
        volatile Mode mode;
        volatile Status status;
        bool escape;
        volatile bool ok, failure;
        volatile Response res;
        int cid;
        int n;
        CircBuffer<char> *buf;
    } _state;

    struct CONNECTION {
        Protocol protocol;
        Type type;
        bool connected;
        char ip[16];
        int port;
        CircBuffer<char> *buf;
        volatile bool received;
        volatile int parent;
        volatile bool accept;
        void(*func)(int);
    } _con[16];

#ifdef CFG_ENABLE_HTTPD
    enum HttpdMode {
        HTTPDMODE_REQUEST,
        HTTPDMODE_REQUEST_STR,
        HTTPDMODE_HEADER,
        HTTPDMODE_BODY,
        HTTPDMODE_ENTER,
        HTTPDMODE_ERROR,
        HTTPDMODE_WEBSOCKET_BEGIN,
        HTTPDMODE_WEBSOCKET,
        HTTPDMODE_WEBSOCKET_MASK,
        HTTPDMODE_WEBSOCKET_BODY,
        HTTPDMODE_WEBSOCKET_ENTER,
    };

    enum HttpdReq {
        REQ_HTTPGET,
        REQ_HTTPPOST,
    };

    struct HTTPD {
        HttpdMode mode;
        HttpdReq req;
        CircBuffer <char>*buf;
        char *uri;
        char *filename;
        char *querystring;
        int enter;
        int length;
        int n;
        int keepalive;
#ifdef CFG_ENABLE_WEBSOCKET
        int websocket;
        char *websocket_key;
        int websocket_opcode;
        int websocket_flg;
        char websocket_mask[4];
        int websocket_payload;
#endif
    } _httpd[16];

    struct HTTPD_HANDLER {
        char *uri;
        char *dir;
        void (*func)(int);
        int ws;
    } _httpd_handler[CFG_HTTPD_HANDLER_NUM];

    int _httpd_cid;
    int _handler_count;
#endif // CFG_ENABLE_HTTPD


    // ----- GSwifi.cpp -----
#ifdef CFG_ENABLE_RTOS
    static void threadPoll (void const *args);
#endif

    // ----- GSwifi_sock.cpp -----
    void initCon (int cid, bool connected);

    // ----- GSwifi_util.cpp -----
    int x2i (char c);
    char i2x (int i);
    int from_hex (int ch);
    int to_hex (int code);

    // ----- GSwifi_http.cpp -----

#ifdef CFG_ENABLE_HTTPD
    // ----- GSwifi_httpd.cpp -----
    void httpdRecvData (int cid, char c);
    int httpdParseRequest (int cid);
    void httpdPoll ();
    int httpdParseHeader (int cid);
    void reqContentLength (int cid, const char *buf);
    void reqConnection (int cid, const char *buf);
    void reqUpgrade (int cid, const char *buf);
    void reqWebSocketVersion (int cid, const char *buf);
    void reqWebSocketKey (int cid, const char *buf);

    // ----- GSwifi_httpd_util.cpp -----
    int httpdFile (int cid, char *dir);
    int httpdGetHandler (const char *uri);
    char *mimetype (char *file);
    int strnicmp (const char *p1, const char *p2, int n);
#endif

#ifdef CFG_ENABLE_WEBSOCKET
    // ----- GSwifi_httpd_ws.cpp -----
    int wsWait (int cid, int code);
#ifdef CFG_ENABLE_HTTPD
    void wsRecvData (int cid, char c);
    int wsParseRequest (int cid);
    int wsAccept (int cid);
#endif
#endif

#ifdef CFG_ENABLE_SMTP
    // ----- GSwifi_smtp.cpp -----
    int smtpWait (int cid, int code);
#endif

    // ----- GSwifi_msg.cpp -----
    void recvData (char c);
    int parseMessage ();
    void msgOk (const char*);
    void msgError (const char*);
    void msgConnect (const char*);
    void msgDisconnect (const char*);
    void msgDisassociated (const char*);
    void msgReset (const char*);
    void msgOutofStandby (const char*);
    void msgOutofDeepsleep (const char*);
    void resNormal (const char*);
    void resConnect (const char*);
    void resWpapsk (const char *buf);
    void resWps (const char*);
    void resMacAddress (const char*);
    void resIp (const char*);
    void resLookup (const char*);
    void resRssi (const char*);
    void resTime (const char*);
    void resChannel (const char*);
    void resStatus (const char*);
    void resHttp (const char *buf);

    // ----- GSwifi_at.cpp -----
    void clearFlags ();
    int cmdAT ();
    int cmdE (bool n);
    int cmdR (bool n);
    int cmdNMAC (const char *s = NULL);
    int cmdWREGDOMAIN (int n = CFG_WREGDOMAIN);
    int cmdWS ();
    int cmdWM (int n);
    int cmdWA (const char *s);
    int cmdWD ();
    int cmdWWPS (bool n, const char *p = NULL);
    int cmdNSTAT ();
    int cmdWSTATUS ();
    int cmdWRSSI ();
    int cmdWAUTH (int n);
    int cmdWWEP (int n, const char *s);
    int cmdWPAPSK (const char *s, const char *p);
    int cmdWRXACTIVE (bool n);
    int cmdWRXPS (bool n);
    int cmdWP (int n);
    int cmdNDHCP (bool n, const char *s = NULL, int m = CFG_TIMEOUT);
    int cmdDHCPSRVR (bool n);
    int cmdNSET (const char *s, const char *t, const char *u);
    int cmdDNS (bool n, const char *s);
    int cmdDNSLOOKUP (const char *s);
    int cmdDNSSET (const char *s);
    int cmdSTORENWCONN ();
    int cmdRESTORENWCONN ();
    int cmdBDATA (bool n);
    int cmdNCTCP (const char *s, int n);
    int cmdNCUDP (const char *s, int n, int m = 0);
    int cmdNSTCP (int n);
    int cmdNSUDP (int n);
    int cmdNCLOSE (int n);
    int cmdNCLOSEALL ();
    int cmdHTTPCONF (int n, const char *s);
    int cmdHTTPCONFDEL (int n);
    int cmdHTTPOPEN (const char *s, int n = 80, bool m = false);
    int cmdHTTPSEND (int n, bool m, const char *s, int t = 0);
    int cmdHTTPCLOSE (int n);
    int cmdPSDPSLEEP (int n = 0);
    int cmdPSSTBY (int n, int m = 0);
    int cmdWEBPROV (const char *s, const char *p);
    int cmdSETTIME (const char *s, const char *t);
    int cmdGETTIME ();
    int cmdNTIMESYNC (bool n, const char *s, int m = 0);
    int cmdDGPIO (int n, int m);

    // ----- GSwifi_hal.cpp -----
    void setReset (bool flg);
    void setAlarm (bool flg);
    void isrUart ();
    int getUart ();
    void putUart (char c);
    void setRts (bool flg);
    int lockUart (int ms);
    void unlockUart ();
    void initUart (PinName cts, PinName rts, PinName alarm, int baud);

};

#endif