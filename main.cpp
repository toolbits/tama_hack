/*
**      Barista Hack
**
**      Original Copyright (C) 2014 - 2014 HORIGUCHI Junshi.
**                                          http://iridium.jp/
**                                          zap00365@nifty.com
**      Portions Copyright (C) <year> <author>
**                                          <website>
**                                          <e-mail>
**      Version     mbed LPC1768
**      Website     http://iridium.jp/
**      E-mail      zap00365@nifty.com
**
**      This source code is for mbed Online IDE.
**      mbed Online IDE 2014/10/09
**
**      main.cpp
**
**      ------------------------------------------------------------------------
**
**      GNU GENERAL PUBLIC LICENSE (GPLv3)
**
**      This program is free software: you can redistribute it and/or modify
**      it under the terms of the GNU General Public License as published by
**      the Free Software Foundation, either version 3 of the License,
**      or (at your option) any later version.
**      This program is distributed in the hope that it will be useful,
**      but WITHOUT ANY WARRANTY; without even the implied warranty of
**      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
**      See the GNU General Public License for more details.
**      You should have received a copy of the GNU General Public License
**      along with this program. If not, see <http://www.gnu.org/licenses/>.
**
**      このプログラムはフリーソフトウェアです。あなたはこれをフリーソフトウェア財団によって発行された
**      GNU 一般公衆利用許諾書（バージョン 3 か、それ以降のバージョンのうちどれか）が定める条件の下で
**      再頒布または改変することができます。このプログラムは有用であることを願って頒布されますが、
**      *全くの無保証* です。商業可能性の保証や特定目的への適合性は、言外に示されたものも含め全く存在しません。
**      詳しくは GNU 一般公衆利用許諾書をご覧ください。
**      あなたはこのプログラムと共に GNU 一般公衆利用許諾書のコピーを一部受け取っているはずです。
**      もし受け取っていなければ <http://www.gnu.org/licenses/> をご覧ください。
*/

#include "EthernetPowerControl.h"
#include "Button.h"
#include "OLED.h"
#include "Speaker.h"
#include "Controller.h"
#include "Mainboard.h"
#include "GSwifiInterface.h"
#include "IRXTime.h"
#include "picojson.h"

#define __WIFI_HOME
//#define __WIFI_IDD

#define WATCHDOG_TIMEOUT        (60)
#define SERIAL_BAUD             (115200)
#define WIFI_BAUD               (115200) // ATB=115200, AT&K0, AT&R1, AT&W0
//#define WIFI_BAUD               (38400) // ATB=115200, AT&K0, AT&R1, AT&W0
#if defined __WIFI_HOME
#define WIFI_SECURITY           (GSwifi::SEC_WPA_PSK)
#define WIFI_SSID               ("ssid")
#define WIFI_PHRASE             ("password")
#elif defined __WIFI_IDD
#define WIFI_SECURITY           (GSwifi::SEC_WEP)
#define WIFI_SSID               ("barista-net")
#define WIFI_PHRASE             ("c0ffeecafe2014c0ffeec0ffee")
#else
#define WIFI_SECURITY           (GSwifi::SEC_WPS_BUTTON)
#define WIFI_SSID               ("")
#define WIFI_PHRASE             ("")
#endif
#define WIFI_NTP_HOST           ("ntp.nict.jp")
#define WIFI_NTP_INTERVAL       (3600)
#define WIFI_CLIENT_HOST        ("artsat.idd.tamabi.ac.jp")
#define WIFI_CLIENT_PORT        (16782)
#define WIFI_CLIENT_URI         ("/rpc.json")
#define WIFI_CLIENT_INTERVAL    (60000)
#define WIFI_DISCOVERY_PORT     (1900)
#define WIFI_TWITTER_HOST       ("***.***.***.***")
#define LOCAL_TIME_OFFSET       (60 * 60 * 9)

static  void                    onController            (int count);
static  void                    onMainboard             (LEDEnum led, ModeEnum mode);
static  void                    onServer                (int id);
static  void                    onClient                (int id);
static  void                    onUDP                   (int id);
static  void                    onTwitterStream         (int id);
static  void                    setupMbed               (void);
static  void                    setupWifi               (void);
static  void                    connectWifi             (void);
static  void                    connectSatellite        (void);
static  void                    connectTwitterStream    (void);
static  void                    keepaliveTwitterStream  (void);
static  void                    tweet                   (KeyType key);
static  void                    parseQuery              (string const& param, map<string, string>* result);
static  void                    setupLED                (void);
static  void                    refreshLED              (void);
static  void                    setLED                  (LEDEnum led, ModeEnum mode);
static  void                    mergeLED                (LEDEnum led);

static  Button                  g_button(p17);
static  OLED                    g_oled(p29, p30);
static  Speaker                 g_speaker(p22);
static  Controller              g_controller(p9, p10, p5);
static  Mainboard               g_mainboard(p28, p27, p24);
static  ModeEnum                g_led[LED_LIMIT];
static  GSwifiInterface         g_wifi(p13, p14, p12, P0_22, p7, NC, WIFI_BAUD);
//static  GSwifiInterface         g_wifi(p13, p14, p12, p11, p7, NC, WIFI_BAUD);
static  DigitalIn               g_wifi_hack(p11, PullNone);
static  bool                    g_wifi_valid;
static  bool                    g_wifi_ntp;
static  bool                    g_wifi_client;
static  bool                    g_wifi_flag;
static  string                  g_wifi_session;
static  ir::IRXTime             g_wifi_aos;
static  ir::IRXTime             g_wifi_los;
static  int                     g_twitter_stream;
static  ir::IRXTime             g_time;
static  bool                    g_warning;
static  DigitalOut              g_sign_wifi(LED1);
static  DigitalOut              g_sign_server(LED2);
static  DigitalOut              g_sign_client(LED3);
static  DigitalOut              g_sign_detect(LED4);

int main(void)
{
    Timer timer;
    bool update;
    KeyType key;
    bool hold;
    bool ready;
    ir::IRXTime next;
    ir::IRXTime last;
    
    setupMbed();
    setupLED();
    g_time = ir::IRXTime::currentTime();
    g_warning = false;
    
    g_button.setup();
    g_oled.setup();
    g_speaker.setup();
    g_controller.setup();
    g_mainboard.setup(&onMainboard);
    setupWifi();
    
    refreshLED();
    g_oled.print(0, "Network Barista");
    g_oled.print(1, OLED::SCROLL_OVER, "Initializing...");
    g_speaker.beep(1);
    setLED(LED_POWER_GREEN, MODE_BLINK_1);
    if (g_mainboard.synchronize() != ERROR_OK) {
        g_oled.print(1, "##No Mainboard##");
        clearWDT();
        wait_ms(3000);
    }
    g_speaker.beep(2);
    update = true;
    last.set(0);
    timer.start();
    while (true) {
        clearWDT();
        if (g_button.hasFlag() || update) {
            update = true;
            connectWifi();
            g_speaker.beep(3);
            g_controller.checkKey(&key, &hold);
            g_button.clearFlag();
        }
        if (timer.read_ms() >= WIFI_CLIENT_INTERVAL) {
            update = true;
        }
        if (update) {
            timer.reset();
            set_time(g_wifi.getTime());
            connectSatellite();
            //keepaliveTwitterStream();
            update = false;
        }
        g_time = ir::IRXTime::currentTime();
        g_wifi.poll();
        if (g_controller.checkKey(&key, &hold, &onController) == ERROR_OK) {
            if (g_mainboard.isOn()) {
                ready = g_mainboard.isReady();
                if (g_mainboard.pressKey(key, hold) == ERROR_OK) {
                    if (ready && key != KEY_POWER && hold) {
                        tweet(key);
                    }
                }
            }
            else {
                switch (key) {
                    case KEY_POWER:
                        g_mainboard.pressKey(key, hold);
                        break;
                    default:
                        // nop
                        break;
                }
            }
        }
        if (g_wifi_flag) {
            next = g_wifi_aos + (g_wifi_los - g_wifi_aos) / 2;
            if (g_wifi_aos <= g_time && g_time <= g_wifi_los) {
                g_controller.startRotate();
                g_sign_detect = true;
                if (!g_warning) {
                    g_speaker.beep(8);
                    g_warning = true;
                }
                if (next != last) {
                    if (g_time >= next) {
                        g_speaker.beep(4);
                        log("BARISTA: satellite[%s]\n", next.formatYMD().c_str());
                        if (g_mainboard.powerOn() == ERROR_OK) {
                            if (g_mainboard.pressKey(KEY_ESPRESSO, true) == ERROR_OK) {
                                last = next;
                            }
                        }
                        else {
                            last = next;
                        }
                    }
                }
            }
            else {
                g_warning = false;
                g_sign_detect = false;
                g_controller.stopRotate();
            }
        }
        else {
            g_warning = false;
            g_sign_detect = false;
            g_controller.stopRotate();
        }
        if (g_oled.isEmpty(1)) {
            switch (g_time.asTime_t() / 20 % 4) {
                case 0:
                    g_oled.print(0, "Network Barista");
                    g_oled.print(1, OLED::SCROLL_OVER, 1, "NESCAFE x Tamabi");
                    break;
                case 1:
                    g_oled.print(0, "Network Barista");
                    g_oled.print(1, OLED::SCROLL_OVER, 1, "%s", (g_time + ir::IRXTimeDiff(LOCAL_TIME_OFFSET)).formatYMD().c_str());
                    break;
                case 2:
                    g_oled.print(0, "Satellite Coffee");
                    if (g_wifi_flag) {
                        g_oled.print(1, OLED::SCROLL_OVER, 1, "%s", (next + ir::IRXTimeDiff(LOCAL_TIME_OFFSET)).formatYMD().c_str());
                    }
                    else {
                        g_oled.print(1, OLED::SCROLL_OVER, 1, "offline");
                    }
                    break;
                case 3:
                    g_oled.print(0, "Network Barista");
                    g_oled.print(1, OLED::SCROLL_OVER, 1, "%s", g_wifi.getIPAddress());
                    break;
                default:
                    // nop
                    break;
            }
        }
        refreshLED();
    }
}

static void onController(int count)
{
    g_speaker.beep(count);
    return;
}

static void onMainboard(LEDEnum led, ModeEnum mode)
{
    mergeLED(led);
    return;
}

static void onServer(int id)
{
    static char const* const s_header[] = {
        "HTTP/1.1 200 OK\r\n",
        "Content-type: application/json\r\n",
        "Cache-Control: no-cache\r\n",
        "\r\n"
    };
    char* host;
    int port;
    ErrorEnum error;
    string sequence;
    static char buffer[256];
    map<string, string> query;
    map<string, string>::const_iterator it;
    picojson::object json;
    picojson::array led;
    int i;
    int wifierr;
    
    g_sign_server = false;
    for (i = 0; i < lengthof(s_header); ++i) {
        if ((wifierr = g_wifi.send(id, s_header[i], strlen(s_header[i]))) < 0) {
            log("Wifi send failed: %d\n", wifierr);
            break;
        }
    }
    if (wifierr >= 0) {
        host = "";
        port = 0;
        if ((wifierr = g_wifi.getRemote(id, &host, &port)) < 0) {
            log("Wifi getRemote failed: %d\n", wifierr);
        }
        error = ERROR_OK;
        sequence = g_wifi.httpdGetQuerystring(id);
        if ((wifierr = g_wifi.urldecode(sequence.c_str(), buffer, sizeof(buffer))) >= 0) {
            sequence = buffer;
            parseQuery(sequence, &query);
            if ((it = query.find("message")) != query.end()) {
                g_oled.print(1, OLED::SCROLL_OVER, 1, false, "%.32s", it->second.c_str());
            }
        }
        else {
            error = ERROR_INVALID_PARAM;
            log("Wifi urldecode failed: %d\n", wifierr);
        }
        log("HTTPD: %s:%d [%s]\n", host, port, sequence.c_str());
        json["error"] = picojson::value(static_cast<double>(error));
        for (i = 0; i < LED_LIMIT; ++i) {
            led.push_back(picojson::value(static_cast<double>(g_mainboard.getLED(static_cast<LEDEnum>(i)))));
        }
        json["led"] = picojson::value(led);
        json["state"] = picojson::value(string((g_mainboard.isOn()) ? ("on") : ("off")));
        json["time"] = picojson::value(static_cast<double>(g_time.asTime_t()));
        if (g_wifi_flag) {
            json["aos"] = picojson::value(static_cast<double>(g_wifi_aos.asTime_t()));
            json["los"] = picojson::value(static_cast<double>(g_wifi_los.asTime_t()));
        }
        else {
            json["aos"] = picojson::value(static_cast<double>(0));
            json["los"] = picojson::value(static_cast<double>(0));
        }
        json["ntp"] = picojson::value(g_wifi_ntp);
        json["client"] = picojson::value(g_wifi_client);
        sequence = picojson::value(json).serialize();
        if ((wifierr = g_wifi.send(id, sequence.c_str(), sequence.length())) < 0) {
            log("Wifi send failed: %d\n", wifierr);
        }
    }
    if ((wifierr = g_wifi.close(id)) < 0) {
        log("Wifi close failed [%d]: %d\n", id, wifierr);
    }
    g_sign_server = true;
    return;
}

static void onClient(int id)
{
    static char buffer[256];
    int value;
    string content;
    string status;
    picojson::value json;
    string error;
    int wifierr;
    
    if ((wifierr = g_wifi.recv(id, buffer, sizeof(buffer))) >= 0) {
        content = buffer;
        if ((value = content.find("\r\n")) != string::npos) {
            if ((status = content.substr(0, value)) == "200 OK") {
                content.erase(0, value + 2);
                picojson::parse(json, content.c_str(), content.c_str() + content.length(), &error);
                if (error.empty()) {
                    if (json.is<picojson::object>()) {
                        picojson::object& root(json.get<picojson::object>());
                        if (root["result"].is<picojson::object>()) {
                            picojson::object& result(root["result"].get<picojson::object>());
                            if (result["session"].is<string>()) {
                                g_wifi_session = result["session"].get<string>();
                                g_wifi_flag = false;
                                if (result["aos"].is<string>()) {
                                    if ((value = g_wifi_aos.parse("%YYYY/%MM/%DD %hh:%mm:%ss UTC", result["aos"].get<string>())) == ir::IRXTime::ERROR_OK) {
                                        if (result["los"].is<string>()) {
                                            if ((value = g_wifi_los.parse("%YYYY/%MM/%DD %hh:%mm:%ss UTC", result["los"].get<string>())) == ir::IRXTime::ERROR_OK) {
                                                g_wifi_client = true;
                                                g_wifi_flag = true;
                                                g_sign_client = true;
                                                log("AOS: %s, LOS: %s, Time: %s\n", g_wifi_aos.formatYMD().c_str(), g_wifi_los.formatYMD().c_str(), g_time.formatYMD().c_str());
                                            }
                                            else {
                                                log("JSON format[los] error: %d\n", value);
                                            }
                                        }
                                        else {
                                            log("JSON result[los] error: %s\n", picojson::value(result).serialize().c_str());
                                        }
                                    }
                                    else {
                                        log("JSON format[aos] error: %d\n", value);
                                    }
                                }
                                else {
                                    log("JSON result[aos] error: %s\n", picojson::value(result).serialize().c_str());
                                }
                            }
                            else {
                                log("JSON result[session] error: %s\n", picojson::value(result).serialize().c_str());
                            }
                        }
                        else {
                            log("JSON rpc error: %s\n", picojson::value(root).serialize().c_str());
                        }
                    }
                    else {
                        log("JSON format error: %s\n", json.serialize().c_str());
                    }
                }
                else {
                    log("JSON parse failed: %s\n", error.c_str());
                }
            }
            else {
                log("HTTP status failed: %s\n", status.c_str());
            }
        }
        else {
            log("HTTP response failed: %s\n", content.c_str());
        }
    }
    else {
        log("Wifi recv failed: %d\n", wifierr);
    }
    return;
}

static void onUDP(int id)
{
    static string const ping_request  = "COFFEE-ping";
    static string const ping_response = "COFFEE+ping";
    static string const stat_request  = "COFFEE-stat";
    static string const stat_response = "COFFEE+stat";
    static string const push_request  = "COFFEE-push";
    static string const oled_request  = "COFFEE-oled";
    
    static char buf[128];
    static char ip[17];
    
    int port;
    if (g_wifi.readable(id))
    {
        int len;
        if ((len = g_wifi.recvfrom(id, buf, sizeof(buf), ip, &port)) > 0)
        {
            log("UDP received from:%s\n", ip);
            string msg(buf, len);
            if (msg == ping_request)
            {
                g_wifi.sendto(id, ping_response.c_str(), ping_response.size(), ip, port);
            }
            else if (msg == stat_request)
            {
                char status[] = {
                    !g_mainboard.isOn(),
                    g_controller.getLED(LED_POWER_GREEN),
                    g_controller.getLED(LED_POWER_RED),
                    g_controller.getLED(LED_MAINTENANCE),
                    g_controller.getLED(LED_CLEANING),
                    g_controller.getLED(LED_SUPPLY),
                    g_controller.getLED(LED_ESPRESSO),
                    g_controller.getLED(LED_CAFFELATTE),
                    g_controller.getLED(LED_CAPPUCCINO),
                    g_controller.getLED(LED_BLACKMAG),
                    g_controller.getLED(LED_BLACK)
                };
                string res = stat_response + string(status, sizeof(status));
                g_wifi.sendto(id, res.c_str(), res.size(), ip, port);
            }
            else if (msg.find(push_request) == 0 && msg.size() == push_request.size() + 2)
            {
                KeyEnum key = (KeyEnum)msg[push_request.size()+0];
                bool longpress = (bool)msg[push_request.size()+1];
                g_mainboard.pressKey(key, longpress);
                g_speaker.beep(1);
                if (longpress) {
                    g_speaker.beep(2);
                }
            }
            else if (msg.find(oled_request) == 0)
            {
                string text = msg.substr(oled_request.size());
                g_oled.print(1, OLED::SCROLL_OVER, 1, text.c_str());
            }
        }
    }
    return;
}

static void onTwitterStream(int id)
{
    log("onTwitterStream\n");
    if (g_wifi.readable(id)) {
        static char buf[256];
        int len = g_wifi.recv(id, buf, sizeof(buf));
        string tweet(buf, len);
        log("read %s\n", tweet.c_str());
        std::transform(tweet.begin(), tweet.end(), tweet.begin(), ::tolower);
        if (tweet.find("please") != string::npos && tweet.find("@net_barista") != string::npos) {
            g_mainboard.powerOn();
            if (tweet.find("espresso") != string::npos) {
                g_mainboard.pressKey(KEY_ESPRESSO, true);
            }
            else if (tweet.find("caffelatte") != string::npos) {
                g_mainboard.pressKey(KEY_CAFFELATTE, true);
            }
            else if (tweet.find("cappuccino") != string::npos) {
                g_mainboard.pressKey(KEY_CAPPUCCINO, true);
            }
            else if (tweet.find("mag coffee") != string::npos) {
                g_mainboard.pressKey(KEY_BLACKMAG, true);
            }
            else if (tweet.find("coffee") != string::npos) {
                g_mainboard.pressKey(KEY_BLACK, true);
            }
        }
    }
    return;
}

static void setupMbed(void)
{
    PHY_PowerDown();
    NVIC_SetPriority(UART1_IRQn, 0); // GSwifiInterface
    NVIC_SetPriority(I2C2_IRQn, 1); // I2CSlaveX
    NVIC_SetPriority(EINT3_IRQn, 254); // Button
    NVIC_SetPriority(TIMER3_IRQn, 255); // Ticker
    setupWDT(WATCHDOG_TIMEOUT);
    setupLog(SERIAL_BAUD);
    return;
}

static void setupWifi(void)
{
    int wifierr;
    
    g_wifi_valid = false;
    log("Wifi init start...\n");
    if ((wifierr = g_wifi.init()) >= 0) {
        g_wifi_valid = true;
    }
    else {
        log("Wifi init failed: %d\n", wifierr);
    }
    return;
}

static void connectWifi(void)
{
    int wifierr;
    
    g_controller.startRotate();
    setLED(LED_POWER_RED, MODE_BLINK_1);
    g_oled.print(0, "WIFI Negotiating");
    g_oled.print(1, "");
    g_wifi.disconnect();
    g_wifi_flag = false;
    g_wifi_client = false;
    g_wifi_ntp = false;
    g_sign_client = false;
    g_sign_server = false;
    g_sign_wifi = false;
    clearWDT();
    wait_ms(1000);
    g_oled.print(1, OLED::SCROLL_FIT, "SSID: %s", WIFI_SSID);
    log("Wifi connect start...\n");
    if ((wifierr = g_wifi.connect(WIFI_SECURITY, WIFI_SSID, WIFI_PHRASE)) >= 0) {
        g_sign_wifi = true;
        g_oled.print(1, OLED::SCROLL_FIXED, 10, "%s", g_wifi.getIPAddress());
        log("Client IP: %s\n", g_wifi.getIPAddress());
        log("Gateway IP: %s\n", g_wifi.getGateway());
        log("Sub Net Mask: %s\n", g_wifi.getNetworkMask());
        log("Wifi httpd start...\n");
        wifierr = g_wifi.listen(GSwifi::PROTO_UDP, WIFI_DISCOVERY_PORT, &onUDP);
        //connectTwitterStream();
        g_wifi.poll();
        if ((wifierr = g_wifi.httpd()) >= 0) {
            if ((wifierr = g_wifi.httpdAttach("/barista.json", &onServer)) >= 0) {
                g_sign_server = true;
                log("Wifi ntpdate start...\n");
                if ((wifierr = g_wifi.ntpdate(WIFI_NTP_HOST)) >= 0) {
                    if ((wifierr = g_wifi.ntpdate(WIFI_NTP_HOST, WIFI_NTP_INTERVAL)) >= 0) {
                        g_wifi_ntp = true;
                        setLED(LED_POWER_RED, MODE_OFF);
                    }
                    else {
                        log("Wifi ntpdate sync failed: %d\n", wifierr);
                    }
                }
                else {
                    log("Wifi ntpdate failed: %d\n", wifierr);
                }
            }
            else {
                log("Wifi httpdAttach failed: %d\n", wifierr);
            }
        }
        else {
            log("Wifi httpd failed: %d\n", wifierr);
        }
    }
    else {
        g_oled.print(1, "");
        log("Wifi connect failed: %d\n", wifierr);
    }
    g_controller.stopRotate();
    g_oled.print(0, "Network Barista");
    return;
}

static void connectSatellite(void)
{
    picojson::object json;
    picojson::object params;
    int wifierr;
    
    g_wifi_client = false;
    g_sign_client = false;
    if (g_wifi_valid) {
        json["jsonrpc"] = picojson::value(string("2.0"));
        json["method"] = picojson::value(string("observer.getSpacecraftAOSLOS"));
        params["session"] = picojson::value(g_wifi_session);
        json["params"] = picojson::value(params);
        json["id"] = picojson::value(static_cast<double>(0));
        log("Wifi httpPost start...\n");
        if ((wifierr = g_wifi.httpPost(WIFI_CLIENT_HOST, WIFI_CLIENT_PORT, WIFI_CLIENT_URI, picojson::value(json).serialize().c_str(), &onClient)) < 0) {
            log("Wifi httpPost failed: %d\n", wifierr);
        }
    }
    return;
}

static void connectTwitterStream(void)
{
    g_twitter_stream = g_wifi.open(GSwifi::PROTO_TCP, WIFI_TWITTER_HOST, 3001, 0, &onTwitterStream);
    return;
}

static void keepaliveTwitterStream(void)
{
    g_wifi.send(g_twitter_stream, "p", 1);
    return;
}

static void tweet(KeyType key)
{
    string text;
    
    switch (key) {
        case KEY_ESPRESSO:
            text += ":coffee: espresso";
            break;
        case KEY_CAFFELATTE:
            text += ":coffee: caffelatte";
            break;
        case KEY_CAPPUCCINO:
            text += ":coffee: cappuccino";
            break;
        case KEY_BLACKMAG:
            text += ":coffee: black coffee (mag)";
            break;
        case KEY_BLACK:
            text += ":coffee: black coffee";
            break;
        default:
            // nop
            break;
    }
    if (!text.empty()) {
        text += " at " + g_time.formatYMD();
        g_wifi.httpPost(WIFI_TWITTER_HOST, 3000, "/BRWXsm5YGoQnJ", text.c_str());
    }
    return;
}

static void parseQuery(string const& param, map<string, string>* result)
{
    string pair;
    string key;
    int index;
    int amp;
    int equal;
    
    result->clear();
    index = 0;
    do {
        if ((amp = param.find('&', index)) != string::npos) {
            pair = param.substr(index, amp - index);
        }
        else {
            pair = param.substr(index);
        }
        if ((equal = pair.find('=')) != string::npos) {
            key = pair.substr(0, equal);
            if (!key.empty()) {
                (*result)[key] = pair.substr(equal + 1);
            }
        }
        index = amp + 1;
    } while (amp != string::npos);
    return;
}

static void setupLED(void)
{
    int i;
    
    for (i = 0; i < LED_LIMIT; ++i) {
        g_led[i] = MODE_OFF;
    }
    return;
}

static void refreshLED(void)
{
    int i;
    
    for (i = 0; i < LED_LIMIT; ++i) {
        mergeLED(static_cast<LEDEnum>(i));
    }
    return;
}

static void setLED(LEDEnum led, ModeEnum mode)
{
    if (led < LED_LIMIT) {
        g_led[led] = mode;
        mergeLED(led);
    }
    return;
}

static void mergeLED(LEDEnum led)
{
    ModeEnum barista;
    ModeEnum hack;
    
    if (led < LED_LIMIT) {
        barista = g_mainboard.getLED(led);
        if (led == LED_POWER_GREEN && barista == MODE_BLINK_0) {
            barista = MODE_BLINKFAST_1;
        }
        else if (led == LED_POWER_RED && barista == MODE_ON) {
            barista = MODE_BLINK_0;
        }
        hack = g_led[led];
        g_controller.setLED(led, (barista == MODE_ON || hack == MODE_ON) ? (MODE_ON) : (max(barista, hack)));
    }
    return;
}
