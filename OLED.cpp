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
**      OLED.cpp
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

#include "OLED.h"

#define I2C_CLOCK               (100000)
#define I2C_ADDRESS             (0x3C << 1)
#define TICKER_INTERVAL         (250000)
#define LINE_WIDTH              (16)
#define FIT_WAIT                (SECOND_TO_STEP(4))
#define SECOND_TO_STEP(param)   ((param) * 1000000 / TICKER_INTERVAL)

enum OLEDEnum {
    OLED_CLEAR_DISPLAY          = 0x01,
    OLED_RETURN_HOME            = 0x02,
    OLED_DISPLAY_ON             = 0x0C,
    OLED_DISPLAY_OFF            = 0x08,
    OLED_FUNCTION_NORMAL        = 0x28,
    OLED_FUNCTION_REGISTER_RE   = 0x2A,
    OLED_FUNCTION_SD_NORMAL     = 0x78,
    OLED_FUNCTION_SD_EXTENSION  = 0x79,
    OLED_FUNCTION_SELECTION_B   = 0x72,
    OLED_FUNCTION_CONTRAST      = 0x81,
    OLED_SELECTION_B_ROM_A      = 0x00,
    OLED_SELECTION_B_ROM_B      = 0x04,
    OLED_SELECTION_B_ROM_C      = 0x08,
    OLED_CONTRAST_MAXIMUM       = 0xFF,
    OLED_CONTRAST_NORMAL        = 0x7F,
    OLED_CONTRAST_MINIMUM       = 0x00,
    OLED_DDRAM_ADDRESS          = 0x80
};

/*public */int OLED::getSize(int line) const
{
    int result(0);
    
    if (_valid) {
        if (0 <= line && line < lengthof(_item)) {
            result = _item[line].size();
        }
    }
    return result;
}

/*public */bool OLED::isEmpty(int line) const
{
    bool result(true);
    
    if (_valid) {
        if (0 <= line && line < lengthof(_item)) {
            result = _item[line].empty();
        }
    }
    return result;
}

/*public */void OLED::setup(void)
{
    cleanup();
    _i2c.setFrequency(I2C_CLOCK);
    wait_ms(100);
    writeCommand(OLED_DISPLAY_OFF);
    writeCommand(OLED_FUNCTION_REGISTER_RE);
    writeCommand(OLED_FUNCTION_SELECTION_B);
    writeData(OLED_SELECTION_B_ROM_B);
    writeCommand(OLED_FUNCTION_SD_EXTENSION);
    writeCommand(OLED_FUNCTION_CONTRAST);
    writeCommand(OLED_CONTRAST_MAXIMUM);
    writeCommand(OLED_FUNCTION_SD_NORMAL);
    writeCommand(OLED_FUNCTION_NORMAL);
    writeCommand(OLED_CLEAR_DISPLAY);
    wait_ms(5);
    writeCommand(OLED_RETURN_HOME);
    wait_ms(5);
    writeCommand(OLED_DISPLAY_ON);
    attach();
    _valid = true;
    return;
}

/*public */void OLED::cleanup(void)
{
    int i;
    
    if (_valid) {
        detach();
        writeCommand(OLED_DISPLAY_OFF);
        for (i = 0; i < lengthof(_item); ++i) {
            _item[i].clear();
        }
    }
    _valid = false;
    return;
}

/*public */void OLED::print(int line, ScrollEnum scroll, int second, int immediate, char const* format, va_list ap)
{
    char data[256];
    ItemRec item;
    
    if (_valid) {
        if (0 <= line && line < lengthof(_item)) {
            if (vsnprintf(data, sizeof(data), format, ap) >= 0) {
                item.scroll = scroll;
                item.data = data;
                item.limit = item.data.length();
                switch (item.scroll) {
                    case SCROLL_FIT:
                        if (item.limit > LINE_WIDTH) {
                            item.limit -= LINE_WIDTH;
                            item.limit += FIT_WAIT - 1;
                        }
                        else {
                            item.limit = 0;
                        }
                        item.limit += FIT_WAIT - 1;
                        break;
                    case SCROLL_OVER:
                        item.limit += LINE_WIDTH;
                        break;
                    case SCROLL_FIXED:
                    default:
                        item.limit = 0;
                        break;
                }
                ++item.limit;
                if (second < 0) {
                    item.timeup = -1;
                }
                else if (second > 0) {
                    item.timeup = ((SECOND_TO_STEP(second) - 1) / item.limit + 1) * item.limit;
                }
                else {
                    item.timeup = 0;
                }
                item.count = 0;
                item.offset = 0;
                item.current = -1;
                detach();
                if (immediate) {
                    if (!_item[line].empty()) {
                        if (_item[line].front().timeup < 0) {
                            _item[line].pop_front();
                        }
                    }
                    _item[line].push_front(item);
                }
                else {
                    _item[line].push_back(item);
                }
                attach();
            }
        }
    }
    return;
}

/*public */void OLED::print(int line, ScrollEnum scroll, int second, int immediate, char const* format, ...)
{
    va_list ap;
    
    va_start(ap, format);
    print(line, scroll, second, immediate, format, ap);
    va_end(ap);
    return;
}

/*public */void OLED::print(int line, ScrollEnum scroll, int second, char const* format, ...)
{
    va_list ap;
    
    va_start(ap, format);
    print(line, scroll, second, true, format, ap);
    va_end(ap);
    return;
}

/*public */void OLED::print(int line, ScrollEnum scroll, char const* format, ...)
{
    va_list ap;
    
    va_start(ap, format);
    print(line, scroll, -1, true, format, ap);
    va_end(ap);
    return;
}

/*public */void OLED::print(int line, char const* format, ...)
{
    va_list ap;
    
    va_start(ap, format);
    print(line, SCROLL_FIXED, -1, true, format, ap);
    va_end(ap);
    return;
}

/*private */void OLED::onTicker(void)
{
    bool update;
    char data[LINE_WIDTH];
    int value;
    int i;
    
    for (i = 0; i < lengthof(_item); ++i) {
        update = false;
        while (!_item[i].empty()) {
            ItemRec& item(_item[i].front());
            if (item.timeup >= 0) {
                if (item.count >= item.timeup) {
                    _item[i].pop_front();
                    memset(data, ' ', sizeof(data));
                    update = true;
                    continue;
                }
            }
            else if (item.count >= item.limit) {
                item.count = 0;
            }
            ++item.count;
            switch (item.scroll) {
                case SCROLL_FIT:
                    if (item.offset > static_cast<int>(item.data.length()) - LINE_WIDTH) {
                        item.offset = 0;
                    }
                    if (item.offset != item.current || update) {
                        memset(data, ' ', sizeof(data));
                        item.data.copy(data, sizeof(data), item.offset);
                        update = true;
                        item.current = item.offset;
                    }
                    value = (item.count - 1) % item.limit + 1;
                    if (!((1 <= value && value < FIT_WAIT) || (item.limit - FIT_WAIT < value && value <= item.limit - 1))) {
                        ++item.offset;
                    }
                    break;
                case SCROLL_OVER:
                    if (item.offset > static_cast<int>(item.data.length()) + LINE_WIDTH) {
                        item.offset = 0;
                    }
                    if (item.offset != item.current || update) {
                        value = max(LINE_WIDTH - item.offset, 0);
                        memset(data, ' ', sizeof(data));
                        item.data.copy(&data[value], sizeof(data) - value, max(item.offset - LINE_WIDTH, 0));
                        update = true;
                        item.current = item.offset;
                    }
                    ++item.offset;
                    break;
                case SCROLL_FIXED:
                default:
                    if (item.offset != item.current || update) {
                        memset(data, ' ', sizeof(data));
                        item.data.copy(data, sizeof(data), item.offset);
                        update = true;
                        item.current = item.offset;
                    }
                    break;
            }
            break;
        }
        if (update) {
            writeCommand(OLED_DDRAM_ADDRESS | (i * 0x20));
            writeString(data, sizeof(data));
        }
    }
    return;
}

/*private */void OLED::attach(void)
{
    _ticker.attach_us(this, &OLED::onTicker, TICKER_INTERVAL);
    return;
}

/*private */void OLED::detach(void)
{
    _ticker.detach();
    return;
}

/*private */void OLED::writeString(char const* param, int length)
{
    if (length < 0) {
        for (; *param != '\0'; ++param) {
            writeData(*param);
        }
    }
    else {
        for (; length > 0; --length, ++param) {
            writeData(*param);
        }
    }
    return;
}

/*private */void OLED::writeCommand(unsigned char param)
{
    unsigned char data[2];
    
    data[0] = 0x00;
    data[1] = param;
    _i2c.write(I2C_ADDRESS, data, sizeof(data));
    wait_us(100);
    return;
}

/*private */void OLED::writeData(unsigned char param)
{
    unsigned char data[2];
    
    data[0] = 0x40;
    data[1] = param;
    _i2c.write(I2C_ADDRESS, data, sizeof(data));
    wait_us(100);
    return;
}
