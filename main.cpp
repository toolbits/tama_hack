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
#include "GSwifiInterface.h"
#include "I2CSlaveX.h"
#include "picojson.h"
#include "IRXTime.h"
#include "Button.h"
#include "OLED.h"
#include "Speaker.h"
#include "AvailableMemory.h"
#include <algorithm>

//#define __WIFI_HOME
#define __WIFI_IDD

#define SERIAL_BAUD             (115200)
#define WIFI_BAUD               (38400) // ATB=38400, AT&K0, AT&R1, AT&W0
#if defined __WIFI_HOME
#define WIFI_SECURITY           (GSwifi::SEC_WPA_PSK)
#define WIFI_SSID               ("****")
#define WIFI_PHRASE             ("****")
#elif defined __WIFI_IDD
#define WIFI_SECURITY           (GSwifi::SEC_OPEN)
#define WIFI_SSID               ("art.idd_g")
#define WIFI_PHRASE             ("")
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
#define WIFI_UDP_DISCOVERY_PORT (1900)
#define WIFI_TWITTER_HOST       ("****")
#define I2C_CLOCK_CONTROLLER    (75000)
#define I2C_CLOCK_MAINBOARD     (15000)
#define I2C_ADDRESS             (0x58 << 1)
#define KEYHOLD_SHORT           (30)
#define KEYHOLD_LONG            (200)
#define KEYPRESS_TIMEOUT        (1000)
#define LOCAL_TIME_OFFSET       (60 * 60 * 9)

enum CommandEnum {
    COMMAND_NONE                = -1,
    COMMAND_READ_KEY            = 0x00,
    COMMAND_WRITE_LED           = 0x10,
    COMMAND_WRITE_STATE         = 0x20,
    COMMAND_READ_STATE          = 0x80
};
enum KeyEnum {
    KEY_NONE                    = 0x00,
    KEY_POWER                   = 0x01 << 0,
    KEY_ESPRESSO                = 0x01 << 1,
    KEY_CAFFELATTE              = 0x01 << 2,
    KEY_CAPPUCCINO              = 0x01 << 3,
    KEY_BLACKMAG                = 0x01 << 4,
    KEY_BLACK                   = 0x01 << 5
};
typedef char                    KeyType;
enum LEDEnum {
    LED_POWER_GREEN,
    LED_POWER_RED,
    LED_MAINTENANCE,
    LED_CLEANING,
    LED_SUPPLY,
    LED_ESPRESSO,
    LED_CAFELATE,
    LED_CAPPUCCINO,
    LED_BLACKMAG,
    LED_BLACK,
    LED_LIMIT
};
enum ModeEnum {
    MODE_OFF,
    MODE_ON,
    MODE_BLINK_0,
    MODE_BLINKFAST_0,
    MODE_BLINK_1,
    MODE_BLINKFAST_1,
    MODE_LIMIT
};
enum StateEnum {
    STATE_ON                    = 0x00,
    STATE_OFF                   = 0x01,
    STATE_READY                 = 0x09
};

static  void                    onServer                (int id);
static  void                    onClient                (int id);
static  void                    onUdp                   (int id);
static  void                    onI2C                   (void);
static  void                    onTicker                (void);
static  void                    powerOnBarista          (void);
static  void                    powerOffBarista         (void);
static  void                    setupWifi               (void);
static  void                    connectWifi             (void);
static  void                    connectSatellite        (void);
static  void                    connectTwitterStream    ();
static  void                    keepaliveTwitterStream  ();
static  void                    tweet                   (const std::string& text);
static  bool                    checkKey                (KeyType* key, bool* hold);
static  bool                    readKey                 (KeyType* key);
static  void                    pressKey                (KeyType key, bool hold);
static  void                    clearLED                (void);
static  void                    setLEDBarista           (LEDEnum led, ModeEnum mode);
static  void                    setLEDHack              (LEDEnum led, ModeEnum mode);
static  void                    invertLED               (LEDEnum led);
static  void                    updateLED               (void);
static  void                    writeLED                (LEDEnum led, ModeEnum mode);
static  void                    startTicker             (int count);
static  void                    stopTicker              (void);
static  void                    parseQuery              (string const& param, map<string, string>* result);

static  Serial usbram           g_serial(USBTX, USBRX);
static  GSwifiInterface usbram  g_wifi(p13, p14, p12, p11, p7, NC, WIFI_BAUD);
static  bool usbram             g_wifi_ntp;
static  bool usbram             g_wifi_client;
static  bool usbram             g_wifi_flag;
static  string usbram           g_wifi_session;
static  ir::IRXTime usbram      g_wifi_aos;
static  ir::IRXTime usbram      g_wifi_los;
static  int                     g_twitter_stream;
static  I2C usbram              g_controller_i2c(p9, p10);
static  InterruptIn usbram      g_controller_irq(p5);
static  I2CSlaveX usbram        g_mainboard_i2c(p28, p27);
static  DigitalInOut usbram     g_mainboard_irq(p24);
static  bool volatile usbram    g_mainboard_flag;
static  KeyType volatile usbram g_mainboard_key;
static  OLED usbram             g_oled(p29, p30);
static  Button usbram           g_button(p17);
static  Ticker usbram           g_ticker;
static  bool volatile usbram    g_ticker_flag;
static  int volatile usbram     g_ticker_count;
static  int volatile usbram     g_ticker_index;
static  Speaker usbram          g_speaker(p22);
static  DigitalOut usbram       g_sign_wifi(LED1);
static  DigitalOut usbram       g_sign_server(LED2);
static  DigitalOut usbram       g_sign_client(LED3);
static  DigitalOut usbram       g_sign_detect(LED4);
static  ir::IRXTime usbram      g_time;
static  ModeEnum volatile usbram    g_led_mode[2][LED_LIMIT];
static  ModeEnum volatile usbram    g_led_save[LED_LIMIT];
static  StateEnum volatile usbram   g_state;
static  KeyType usbram          g_schedule;

int main(void)
{
    static Timer usbram timer;
    static bool usbram update;
    static KeyType usbram key;
    static bool usbram hold;
    static ir::IRXTime usbram next;
    static ir::IRXTime usbram last;
    
    PHY_PowerDown();
    NVIC_SetPriority(I2C2_IRQn, 0); // I2CSlaveX
    NVIC_SetPriority(EINT3_IRQn, 254); // Button
    NVIC_SetPriority(TIMER3_IRQn, 255); // Ticker
    
    g_serial.baud(SERIAL_BAUD);
    setupWifi();
    g_controller_i2c.frequency(I2C_CLOCK_CONTROLLER);
    g_controller_irq.mode(PullNone);
    g_mainboard_i2c.frequency(I2C_CLOCK_MAINBOARD);
    g_mainboard_i2c.address(I2C_ADDRESS);
    g_mainboard_irq.mode(PullNone);
    g_mainboard_irq.mode(OpenDrain);
    g_mainboard_irq.output();
    g_mainboard_irq = true;
    g_mainboard_flag = false;
    g_oled.setup();
    g_button.setup();
    g_ticker_flag = false;
    g_speaker.setup();
    clearLED();
    g_state = STATE_OFF;
    g_schedule = KEY_NONE;
    setLEDHack(LED_POWER_GREEN, MODE_BLINK_1);
    
    g_serial.printf("Available memory (exact bytes) : %d\n", AvailableMemory(1));
    g_mainboard_i2c.attach(&onI2C);
    
    g_oled.print(0, "Network Barista");
    g_oled.print(1, "Hello...       -");
    g_speaker.beep(1);
    pressKey(KEY_POWER, false);
    g_oled.print(1, "Hello...       \\");
    wait_ms(2000);
    g_oled.print(1, "Hello...       |");
    powerOnBarista();
    g_oled.print(1, "Hello...       /");
    powerOffBarista();
    g_oled.print(1, "Hello...       -");
    update = true;
    last.set(0);
    g_speaker.beep(3);
    g_oled.print(1, "NESCAFE x Tamabi");
    g_serial.printf("Available memory (exact bytes) : %d\n", AvailableMemory(1));
    timer.start();
    while (true) {
        if (g_button.hasFlag() || update) {
            connectWifi();
            g_serial.printf("Available memory (exact bytes) : %d\n", AvailableMemory(1));
            checkKey(&key, &hold);
            update = true;
            g_button.clearFlag();
        }
        if (timer.read_ms() >= WIFI_CLIENT_INTERVAL) {
            update = true;
        }
        if (update) {
            timer.reset();
            set_time(g_wifi.getTime());
            connectSatellite();
            g_serial.printf("Available memory (exact bytes) : %d\n", AvailableMemory(1));
            keepaliveTwitterStream();
            update = false;
        }
        g_time = ir::IRXTime::currentTime();
        if (checkKey(&key, &hold)) {
            if (hold && g_state == STATE_ON && g_led_mode[0][LED_POWER_GREEN] == MODE_ON) {
                std::string text;
                switch (key) {
                    case KEY_ESPRESSO:
                        text += ":coffee: espresso"; break;
                    case KEY_CAFFELATTE:
                        text += ":coffee: caffelatte"; break;
                    case KEY_CAPPUCCINO:
                        text += ":coffee: cappuccino"; break;
                    case KEY_BLACKMAG:
                        text += ":coffee: black coffee (mag)"; break;
                    case KEY_BLACK:
                        text += ":coffee: black coffee"; break;
                }
                if (!text.empty()) {
                    text += " at " + g_time.formatYMD();
                    tweet(text);
                }
            }
            pressKey(key, hold);
        }
        g_wifi.poll();
        if (g_schedule != KEY_NONE) {
            powerOnBarista();
            g_serial.printf("BARISTA: network[%d]\n", g_schedule);
            pressKey(g_schedule, true);
            g_schedule = KEY_NONE;
        }
        if (g_wifi_flag) {
            next = g_wifi_aos + (g_wifi_los - g_wifi_aos) / 2;
            if (g_wifi_aos <= g_time && g_time <= g_wifi_los) {
                startTicker(-1);
                g_sign_detect = true;
                if (next != last) {
                    if (g_time >= next) {
                        powerOnBarista();
                        g_serial.printf("BARISTA: satellite[%s]\n", next.formatYMD().c_str());
                        pressKey(KEY_ESPRESSO, true);
                        last = next;
                    }
                }
            }
            else {
                g_sign_detect = false;
                stopTicker();
            }
        }
        else {
            g_sign_detect = false;
            stopTicker();
        }
        if (g_oled.isEmpty(1)) {
            switch (g_time.asTime_t() / 30 % 3) {
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
                default:
                    // nop
                    break;
            }
        }
    }
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
    static char usbram buffer[256];
    map<string, string> query;
    map<string, string>::const_iterator it;
    picojson::object json;
    picojson::array led;
    int i;
    int wifierr;
    
    startTicker(1);
    g_sign_server = false;
    for (i = 0; i < lengthof(s_header); ++i) {
        if ((wifierr = g_wifi.send(id, s_header[i], strlen(s_header[i]))) < 0) {
            g_serial.printf("Wifi send failed: %d\n", wifierr);
            break;
        }
    }
    if (wifierr >= 0) {
        host = "";
        port = 0;
        if ((wifierr = g_wifi.getRemote(id, &host, &port)) < 0) {
            g_serial.printf("Wifi getRemote failed: %d\n", wifierr);
        }
        error = ERROR_OK;
        sequence = g_wifi.httpdGetQuerystring(id);
        if ((wifierr = g_wifi.urldecode(sequence.c_str(), buffer, sizeof(buffer))) >= 0) {
            sequence = buffer;
            parseQuery(sequence, &query);
            if ((it = query.find("button")) != query.end()) {
                if (g_schedule == KEY_NONE) {
                    if (it->second == "espresso") {
                        g_schedule = KEY_ESPRESSO;
                    }
                    else if (it->second == "cafelate") {
                        g_schedule = KEY_CAFFELATTE;
                    }
                    else if (it->second == "cappuccino") {
                        g_schedule = KEY_CAPPUCCINO;
                    }
                    else if (it->second == "blackmag") {
                        g_schedule = KEY_BLACKMAG;
                    }
                    else if (it->second == "black") {
                        g_schedule = KEY_BLACK;
                    }
                    else {
                        error = ERROR_INVALID_PARAM;
                    }
                }
                else {
                    error = ERROR_INVALID_STATE;
                }
            }
        }
        else {
            g_serial.printf("Wifi urldecode failed: %d\n", wifierr);
        }
        g_serial.printf("HTTPD: %s:%d [%s]\n", host, port, sequence.c_str());
        json["error"] = picojson::value(static_cast<double>(error));
        for (i = 0; i < LED_LIMIT; ++i) {
            led.push_back(picojson::value(static_cast<double>(g_led_mode[0][i])));
        }
        json["led"] = picojson::value(led);
        json["state"] = picojson::value(static_cast<double>(g_state));
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
            g_serial.printf("Wifi send failed: %d\n", wifierr);
        }
    }
    if ((wifierr = g_wifi.close(id)) < 0) {
        g_serial.printf("Wifi close failed: %d\n", wifierr);
    }
    g_sign_server = true;
    return;
}

static void onClient(int id)
{
    static char usbram buffer[256];
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
                                                g_serial.printf("AOS: %s, LOS: %s, Time: %s\n", g_wifi_aos.formatYMD().c_str(), g_wifi_los.formatYMD().c_str(), g_time.formatYMD().c_str());
                                            }
                                            else {
                                                g_serial.printf("JSON format[los] error: %d\n", value);
                                            }
                                        }
                                        else {
                                            g_serial.printf("JSON result[los] error: %s\n", picojson::value(result).serialize().c_str());
                                        }
                                    }
                                    else {
                                        g_serial.printf("JSON format[aos] error: %d\n", value);
                                    }
                                }
                                else {
                                    g_serial.printf("JSON result[aos] error: %s\n", picojson::value(result).serialize().c_str());
                                }
                            }
                            else {
                                g_serial.printf("JSON result[session] error: %s\n", picojson::value(result).serialize().c_str());
                            }
                        }
                        else {
                            g_serial.printf("JSON rpc error: %s\n", picojson::value(root).serialize().c_str());
                        }
                    }
                    else {
                        g_serial.printf("JSON format error: %s\n", json.serialize().c_str());
                    }
                }
                else {
                    g_serial.printf("JSON parse failed: %s\n", error.c_str());
                }
            }
            else {
                g_serial.printf("HTTP status failed: %s\n", status.c_str());
            }
        }
        else {
            g_serial.printf("HTTP response failed: %s\n", content.c_str());
        }
    }
    else {
        g_serial.printf("Wifi recv failed: %d\n", wifierr);
    }
    return;
}

static void onUdp(int id)
{
    static const char request[]  = "Where is the coffee pot?";
    static const char response[] = "I'm a coffee pot.";
    char buf[sizeof(request)-1];
    char ip[17];
    int port;
    if (g_wifi.readable(id))
    {
        if (g_wifi.recvfrom(id, buf, sizeof(buf), ip, &port) == sizeof(buf))
        {
            if (memcmp(request, buf, sizeof(buf)) == 0)
            {
                g_wifi.sendto(id, response, sizeof(response)-1, ip, port);
                //printOLED(1, 0, "I'm a coffee pot");
                //wait_ms(2000);
                //printOLED(1, 0, "NESCAFE x Tamabi");
            }
        }
    }
}

static void onI2C(void)
{
    static CommandEnum s_command(COMMAND_NONE);
    char data[4];
    int size;
    int i;
    
    switch (g_mainboard_i2c.receive()) {
        case I2CSlave::ReadAddressed:
            data[0] = 0x00;
            switch (s_command) {
                case COMMAND_READ_KEY:
                    if (g_mainboard_flag) {
                        data[0] = g_mainboard_key;
                        g_mainboard_irq = true;
                        g_mainboard_flag = false;
                    }
                    break;
                case COMMAND_READ_STATE:
                    data[0] = STATE_READY;
                    break;
                default:
                    // nop
                    break;
            }
            g_mainboard_i2c.write(data, 1);
            g_serial.printf("- %02x\n", data[0]);
            break;
        case I2CSlave::WriteAddressed:
            s_command = COMMAND_NONE;
            if ((size = g_mainboard_i2c.read(data, sizeof(data)) - 1) > 0) {
                s_command = static_cast<CommandEnum>(data[0]);
                switch (s_command) {
                    case COMMAND_READ_KEY:
                    case COMMAND_READ_STATE:
                        // nop
                        break;
                    case COMMAND_WRITE_STATE:
                        if (size > 1) {
                            if ((g_state = static_cast<StateEnum>(data[1])) == STATE_OFF) {
                                for (i = 0; i < LED_LIMIT; ++i) {
                                    setLEDBarista(static_cast<LEDEnum>(i), MODE_OFF);
                                }
                            }
                        }
                        break;
                    default:
                        if (COMMAND_WRITE_LED <= s_command && s_command < COMMAND_WRITE_STATE) {
                            if (size > 1) {
                                setLEDBarista(static_cast<LEDEnum>(s_command - COMMAND_WRITE_LED), static_cast<ModeEnum>(data[1]));
                            }
                        }
                        break;
                }
                g_serial.printf("+ ");
                for (i = 0; i < size; ++i) {
                    g_serial.printf("%02x ", data[i]);
                }
                g_serial.printf("\n");
            }
            break;
        default:
            // nop
            break;
    }
    return;
}

static void onTicker(void)
{
    if (g_ticker_flag) {
        updateLED();
        if (g_ticker_count != 0) {
            if (++g_ticker_index > LED_BLACK - LED_ESPRESSO) {
                if (g_ticker_count > 0) {
                    --g_ticker_count;
                }
                g_ticker_index = 0;
            }
            invertLED(static_cast<LEDEnum>(LED_ESPRESSO + g_ticker_index));
        }
        else {
            g_ticker.detach();
            g_ticker_flag = false;
        }
    }
    return;
}

static void powerOnBarista(void)
{
    if (g_state != STATE_ON) {
        g_serial.printf("BARISTA: power on start...\n");
        pressKey(KEY_POWER, true);
        wait_ms(2000);
        if (g_state != STATE_ON) {
            pressKey(KEY_POWER, true);
            wait_ms(2000);
        }
        if (g_state == STATE_ON) {
            while (g_led_mode[0][LED_POWER_GREEN] != MODE_ON && g_led_mode[0][LED_POWER_RED] == MODE_OFF);
            wait_ms(2000);
        }
        g_serial.printf("BARISTA: power on %s\n", (g_state == STATE_ON) ? ("ok") : ("failed"));
    }
    return;
}

static void powerOffBarista(void)
{
    if (g_state != STATE_OFF) {
        g_serial.printf("BARISTA: power off start...\n");
        pressKey(KEY_POWER, true);
        wait_ms(2000);
        if (g_state != STATE_OFF) {
            pressKey(KEY_POWER, true);
            wait_ms(2000);
        }
        if (g_state == STATE_OFF) {
            while (g_led_mode[0][LED_POWER_GREEN] != MODE_OFF);
            wait_ms(2000);
        }
        g_serial.printf("BARISTA: power off %s\n", (g_state == STATE_OFF) ? ("ok") : ("failed"));
    }
    return;
}

static void setupWifi(void)
{
    int wifierr;
    
    g_serial.printf("Wifi init start...\n");
    if ((wifierr = g_wifi.init()) >= 0) {
#if 0
        if ((wifierr = g_wifi.powerSave(1, 0)) < 0) {
            g_serial.printf("Wifi powerSave failed: %d\n", wifierr);
        }
#endif
    }
    else {
        g_serial.printf("Wifi init failed: %d\n", wifierr);
    }
    return;
}

static void connectWifi(void)
{
    int wifierr;
    
    startTicker(-1);
    setLEDHack(LED_POWER_RED, MODE_BLINK_1);
    g_oled.print(0, "WIFI Negotiating");
    g_oled.print(1, "");
    g_wifi.disconnect();
    g_wifi_flag = false;
    g_wifi_client = false;
    g_wifi_ntp = false;
    g_sign_client = false;
    g_sign_server = false;
    g_sign_wifi = false;
    wait_ms(1000);
    g_oled.print(1, OLED::SCROLL_FIT, "SSID: %s", WIFI_SSID);
    g_serial.printf("Wifi connect start...\n");
    if ((wifierr = g_wifi.connect(WIFI_SECURITY, WIFI_SSID, WIFI_PHRASE)) >= 0) {
        g_sign_wifi = true;
        g_oled.print(1, OLED::SCROLL_FIXED, 10, "%s", g_wifi.getIPAddress());
        g_serial.printf("Client IP: %s\n", g_wifi.getIPAddress());
        g_serial.printf("Gateway IP: %s\n", g_wifi.getGateway());
        g_serial.printf("Sub Net Mask: %s\n", g_wifi.getNetworkMask());
        g_serial.printf("Wifi httpd start...\n");
        g_wifi.listen(GSwifi::PROTO_UDP, WIFI_UDP_DISCOVERY_PORT, onUdp);
        connectTwitterStream();
        if ((wifierr = g_wifi.httpd()) >= 0) {
            if ((wifierr = g_wifi.httpdAttach("/barista.json", &onServer)) >= 0) {
                g_sign_server = true;
                g_serial.printf("Wifi ntpdate start...\n");
                if ((wifierr = g_wifi.ntpdate(WIFI_NTP_HOST)) >= 0) {
                    if ((wifierr = g_wifi.ntpdate(WIFI_NTP_HOST, WIFI_NTP_INTERVAL)) >= 0) {
                        g_wifi_ntp = true;
                        setLEDHack(LED_POWER_RED, MODE_OFF);
                    }
                    else {
                        g_serial.printf("Wifi ntpdate sync failed: %d\n", wifierr);
                    }
                }
                else {
                    g_serial.printf("Wifi ntpdate failed: %d\n", wifierr);
                }
            }
            else {
                g_serial.printf("Wifi httpdAttach failed: %d\n", wifierr);
            }
        }
        else {
            g_serial.printf("Wifi httpd failed: %d\n", wifierr);
        }
    }
    else {
        g_oled.print(1, "");
        g_serial.printf("Wifi connect failed: %d\n", wifierr);
    }
    stopTicker();
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
    json["jsonrpc"] = picojson::value(string("2.0"));
    json["method"] = picojson::value(string("observer.getSatelliteAOSLOS"));
    params["session"] = picojson::value(g_wifi_session);
    json["params"] = picojson::value(params);
    json["id"] = picojson::value(static_cast<double>(0));
    if ((wifierr = g_wifi.httpPost(WIFI_CLIENT_HOST, WIFI_CLIENT_PORT, WIFI_CLIENT_URI, picojson::value(json).serialize().c_str(), &onClient)) < 0) {
        g_serial.printf("Wifi httpPost failed: %d\n", wifierr);
    }
    return;
}

static bool checkKey(KeyType* key, bool* hold)
{
    Timer timer;
    KeyType data;
    bool result(false);
    
    if (readKey(&data)) {
        if (data != KEY_NONE) {
            *key = data;
            *hold = false;
            g_speaker.beep(1);
            timer.start();
            while (true) {
                if (!*hold) {
                    if (timer.read_ms() >= KEYHOLD_LONG) {
                        *hold = true;
                        g_speaker.beep(2);
                    }
                }
                if (readKey(&data)) {
                    if (data != KEY_NONE) {
                        *key |= data;
                    }
                    else {
                        break;
                    }
                }
            }
            result = true;
        }
    }
    return result;
}

static bool readKey(KeyType* key)
{
    char data[2];
    bool result(false);
    
    if (!g_controller_irq) {
        data[0] = COMMAND_READ_KEY;
        if (g_controller_i2c.write(I2C_ADDRESS, data, 1) == 0) {
            if (g_controller_i2c.read(I2C_ADDRESS, data, 1) == 0) {
                *key = data[0];
                wait_ms(KEYHOLD_SHORT);
                result = true;
            }
        }
    }
    return result;
}

static void pressKey(KeyType key, bool hold)
{
    Timer timer;
    
    if (!g_mainboard_flag) {
        g_mainboard_key = key;
        g_mainboard_flag = true;
        g_mainboard_irq = false;
        timer.start();
        while (g_mainboard_flag) {
            if (timer.read_ms() >= KEYPRESS_TIMEOUT) {
                g_mainboard_irq = true;
                g_mainboard_flag = false;
                break;
            }
        }
        timer.reset();
        wait_ms((hold) ? (KEYHOLD_LONG) : (KEYHOLD_SHORT));
        g_mainboard_key = KEY_NONE;
        g_mainboard_flag = true;
        g_mainboard_irq = false;
        timer.start();
        while (g_mainboard_flag) {
            if (timer.read_ms() >= KEYPRESS_TIMEOUT) {
                g_mainboard_irq = true;
                g_mainboard_flag = false;
                break;
            }
        }
    }
    return;
}

static void clearLED(void)
{
    int i;
    
    for (i = 0; i < LED_LIMIT; ++i) {
        g_led_mode[0][i] = MODE_OFF;
        g_led_mode[1][i] = MODE_OFF;
        g_led_save[i] = MODE_LIMIT;
    }
    updateLED();
    return;
}

static void setLEDBarista(LEDEnum led, ModeEnum mode)
{
    if (led < LED_LIMIT) {
        if (led == LED_POWER_GREEN && mode == MODE_BLINK_0) {
            mode = MODE_BLINKFAST_1;
        }
        else if (led == LED_POWER_RED && mode == MODE_ON) {
            mode = MODE_BLINK_0;
        }
        g_led_mode[0][led] = mode;
        updateLED();
    }
    return;
}

static void setLEDHack(LEDEnum led, ModeEnum mode)
{
    if (led < LED_LIMIT) {
        g_led_mode[1][led] = mode;
        updateLED();
    }
    return;
}

static void invertLED(LEDEnum led)
{
    if (led < LED_LIMIT) {
        writeLED(led, (g_led_save[led] != MODE_ON) ? (MODE_ON) : (MODE_OFF));
    }
    return;
}

static void updateLED(void)
{
    ModeEnum mode;
    int i;
    
    for (i = 0; i < LED_LIMIT; ++i) {
        if (g_led_mode[0][i] == MODE_ON || g_led_mode[1][i] == MODE_ON) {
            mode = MODE_ON;
        }
        else {
            mode = max(g_led_mode[0][i], g_led_mode[1][i]);
        }
        writeLED(static_cast<LEDEnum>(i), mode);
    }
    return;
}

static void writeLED(LEDEnum led, ModeEnum mode)
{
    char data[2];
    
    __disable_irq();
    if (mode != g_led_save[led]) {
        data[0] = COMMAND_WRITE_LED + led;
        data[1] = mode;
        g_controller_i2c.write(I2C_ADDRESS, data, sizeof(data));
        g_led_save[led] = mode;
    }
    __enable_irq();
    return;
}

static void startTicker(int count)
{
    if (!g_ticker_flag) {
        if ((g_ticker_count = count) >= 0) {
            ++g_ticker_count;
        }
        g_ticker_index = LED_BLACK - LED_ESPRESSO;
        g_ticker_flag = true;
        g_ticker.attach_us(&onTicker, 75000);
    }
    return;
}

static void stopTicker(void)
{
    if (g_ticker_flag) {
        g_ticker_count = 1;
        while (g_ticker_flag);
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

static void tweet(const std::string& text)
{
    g_wifi.httpPost(WIFI_TWITTER_HOST, 3000, "/BRWXsm5YGoQnJ", text.c_str());
}

static void onTwitterStream(int id)
{
    g_serial.printf("onTwitterStream\n");
    if (g_wifi.readable(id)) {
        char buf[256];
        int len = g_wifi.recv(id, buf, sizeof(buf));
        std::string tweet(buf, len);
        g_serial.printf("read %s\n", tweet.c_str());
        std::transform(tweet.begin(), tweet.end(), tweet.begin(), ::tolower);
        if (tweet.find("please") != std::string::npos && tweet.find("@net_barista") != std::string::npos) {
            powerOnBarista();
            if (tweet.find("espresso") != std::string::npos) {
                pressKey(KEY_ESPRESSO, true);
            }
            else if (tweet.find("caffelatte") != std::string::npos) {
                pressKey(KEY_CAFFELATTE, true);
            }
            else if (tweet.find("cappuccino") != std::string::npos) {
                pressKey(KEY_CAPPUCCINO, true);
            }
            else if (tweet.find("mag coffee") != std::string::npos) {
                pressKey(KEY_BLACKMAG, true);
            }
            else if (tweet.find("coffee") != std::string::npos) {
                pressKey(KEY_BLACK, true);
            }
        }
    }
}

static void connectTwitterStream()
{
    g_twitter_stream = g_wifi.open(GSwifi::PROTO_TCP, WIFI_TWITTER_HOST, 3001, 0, onTwitterStream);
}

static void keepaliveTwitterStream()
{
    g_wifi.send(g_twitter_stream, "p", 1);
    g_serial.printf("Twitter Keepalive\n");
}
