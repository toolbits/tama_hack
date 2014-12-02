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
**      Controller.cpp
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

#include "Controller.h"

#define I2C_CLOCK               (75000)
#define I2C_ADDRESS             (0x58 << 1)
#define KEYHOLD_SHORT           (50)
#define KEYHOLD_LONG            (200)
#define TICKER_INTERVAL         (75000)

/*public */void Controller::setLED(LEDEnum led, ModeEnum mode)
{
    if (_valid) {
        if (led < LED_LIMIT) {
            if (mode != _led[led]) {
                if (!_guard) {
                    _guard = true;
                    writeLED(led, mode);
                    _led[led] = mode;
                    _guard = false;
                }
            }
        }
    }
    return;
}

/*public */ModeEnum Controller::getLED(LEDEnum led) const
{
    ModeEnum result(MODE_OFF);
    
    if (_valid) {
        if (led < LED_LIMIT) {
            result = _led[led];
        }
    }
    return result;
}

/*public */void Controller::setup(void)
{
    int i;
    
    cleanup();
    _i2c.frequency(I2C_CLOCK);
    _irq.mode(PullNone);
    for (i = 0; i < LED_LIMIT; ++i) {
        writeLED(static_cast<LEDEnum>(i), MODE_OFF);
        _led[i] = MODE_OFF;
    }
    _flag = false;
    _guard = false;
    _valid = true;
    return;
}

/*public */void Controller::cleanup(void)
{
    int i;
    
    if (_valid) {
        stopRotate();
        _guard = true;
        for (i = 0; i < LED_LIMIT; ++i) {
            writeLED(static_cast<LEDEnum>(i), MODE_OFF);
        }
    }
    _valid = false;
    return;
}

/*public */ErrorEnum Controller::checkKey(KeyType* key, bool* hold, void(*callback)(int))
{
    Timer timer;
    KeyType data;
    ErrorEnum error(ERROR_OK);
    
    if (_valid) {
        if (key != NULL && hold != NULL) {
            if ((error = readKey(&data)) == ERROR_OK) {
                if (data != KEY_NONE) {
                    *key = data;
                    *hold = false;
                    if (callback != NULL) {
                        (*callback)(1);
                    }
                    timer.start();
                    while (true) {
                        clearWDT();
                        if (!*hold) {
                            if (timer.read_ms() >= KEYHOLD_LONG) {
                                *hold = true;
                                if (callback != NULL) {
                                    (*callback)(2);
                                }
                            }
                        }
                        if ((error = readKey(&data)) == ERROR_OK) {
                            if (data != KEY_NONE) {
                                *key |= data;
                            }
                            else {
                                break;
                            }
                        }
                        else if (error != ERROR_NO_RESULT) {
                            break;
                        }
                    }
                }
                else {
                    error = ERROR_NO_RESULT;
                }
            }
        }
        else {
            error = ERROR_INVALID_PARAM;
        }
    }
    else {
        error = ERROR_INVALID_STATE;
    }
    return error;
}

/*public */void Controller::startRotate(int count)
{
    if (_valid) {
        if (!_flag) {
            if ((_count = count) >= 0) {
                ++_count;
            }
            _index = LED_BLACK;
            _flag = true;
            _ticker.attach_us(this, &Controller::onTicker, TICKER_INTERVAL);
        }
    }
    return;
}

/*public */void Controller::stopRotate(void)
{
    if (_valid) {
        if (_flag) {
            _count = 1;
            while (_flag) {
                clearWDT();
            }
        }
    }
    return;
}

/*private */void Controller::onTicker(void)
{
    if (!_guard) {
        _guard = true;
        writeLED(static_cast<LEDEnum>(_index), _led[_index]);
        if (_count != 0) {
            if (++_index > LED_BLACK) {
                if (_count > 0) {
                    --_count;
                }
                _index = LED_ESPRESSO;
            }
            writeLED(static_cast<LEDEnum>(_index), (_led[_index] != MODE_ON) ? (MODE_ON) : (MODE_OFF));
        }
        else {
            _ticker.detach();
            _flag = false;
        }
        _guard = false;
    }
    return;
}

/*private */void Controller::writeLED(LEDEnum led, ModeEnum mode)
{
    static char data[2];
    
    data[0] = COMMAND_WRITE_LED + led;
    data[1] = mode;
    _i2c.write(I2C_ADDRESS, data, sizeof(data));
    return;
}

/*private */ErrorEnum Controller::readKey(KeyType* key)
{
    static char data[2];
    ErrorEnum error(ERROR_OK);
    
    if (!_irq) {
        data[0] = COMMAND_READ_KEY;
        if (_i2c.write(I2C_ADDRESS, data, 1) == 0) {
            if (_i2c.read(I2C_ADDRESS, data, 1) == 0) {
                *key = data[0];
                clearWDT();
                wait_ms(KEYHOLD_SHORT);
            }
            else {
                error = ERROR_FAILED;
            }
        }
        else {
            error = ERROR_FAILED;
        }
    }
    else {
        error = ERROR_NO_RESULT;
    }
    return error;
}
