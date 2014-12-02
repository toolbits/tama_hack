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
**      Mainboard.cpp
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

#include "Mainboard.h"

#define I2C_CLOCK               (15000)
#define I2C_ADDRESS             (0x58 << 1)
#define STATE_CHANGE_WAIT       (2000)
#define STATE_MARGIN_WAIT       (1000)
#define KEYHOLD_SHORT           (25)
#define KEYHOLD_LONG            (200)
#define KEYPRESS_TIMEOUT        (1000)

/*public */ModeEnum Mainboard::getLED(LEDEnum led) const
{
    ModeEnum result(MODE_OFF);
    
    if (_valid) {
        if (led < LED_LIMIT) {
            result = _led[led];
        }
    }
    return result;
}

/*public */bool Mainboard::isOn(void) const
{
    bool result(false);
    
    if (_valid) {
        if (_state == STATE_ON) {
            result = true;
        }
    }
    return result;
}

/*public */bool Mainboard::isReady(void) const
{
    bool result(false);
    
    if (_valid) {
        if (_state == STATE_ON) {
            if (_led[LED_POWER_GREEN] == MODE_ON &&
                _led[LED_POWER_RED  ] == MODE_OFF &&
                _led[LED_MAINTENANCE] == MODE_OFF &&
                _led[LED_CLEANING   ] == MODE_OFF &&
                _led[LED_SUPPLY     ] == MODE_OFF &&
                _led[LED_ESPRESSO   ] == MODE_ON &&
                _led[LED_CAFFELATTE ] == MODE_ON &&
                _led[LED_CAPPUCCINO ] == MODE_ON &&
                _led[LED_BLACKMAG   ] == MODE_ON &&
                _led[LED_BLACK      ] == MODE_ON) {
                result = true;
            }
        }
    }
    return result;
}

/*public */bool Mainboard::isErrorMaintenance(void) const
{
    bool result(false);
    
    if (_valid) {
        if (_state == STATE_ON) {
            if (_led[LED_MAINTENANCE] != MODE_OFF) {
                result = true;
            }
        }
    }
    return result;
}

/*public */bool Mainboard::isErrorCleaning(void) const
{
    bool result(false);
    
    if (_valid) {
        if (_state == STATE_ON) {
            if (_led[LED_CLEANING] != MODE_OFF) {
                result = true;
            }
        }
    }
    return result;
}

/*public */bool Mainboard::isErrorSupply(void) const
{
    bool result(false);
    
    if (_valid) {
        if (_state == STATE_ON) {
            if (_led[LED_SUPPLY] != MODE_OFF) {
                result = true;
            }
        }
    }
    return result;
}

/*public */void Mainboard::setup(void(*callback)(LEDEnum, ModeEnum))
{
    int i;
    
    cleanup();
    _i2c.frequency(I2C_CLOCK);
    _i2c.address(I2C_ADDRESS);
    _irq.mode(PullNone);
    _irq.mode(OpenDrain);
    _irq.output();
    _irq = true;
    _command = COMMAND_NONE;
    _state = STATE_OFF;
    for (i = 0; i < LED_LIMIT; ++i) {
        _led[i] = MODE_OFF;
    }
    _flag = false;
    _callback = callback;
    _valid = true;
    _i2c.attach(this, &Mainboard::onI2C);
    return;
}

/*public */void Mainboard::cleanup(void)
{
    if (_valid) {
        _i2c.attach(NULL);
        _irq = true;
    }
    _valid = false;
    return;
}

/*public */ErrorEnum Mainboard::synchronize(void)
{
    ErrorEnum error(ERROR_OK);
    
    if (_valid) {
        if ((error = pressKey(KEY_POWER, false)) == ERROR_OK) {
            clearWDT();
            wait_ms(STATE_CHANGE_WAIT);
            if ((error = pressKey(KEY_POWER, false)) == ERROR_OK) {
                clearWDT();
                wait_ms(STATE_CHANGE_WAIT);
                if ((error = powerOn()) == ERROR_OK) {
                    error = powerOff();
                }
            }
        }
    }
    else {
        error = ERROR_INVALID_STATE;
    }
    return error;
}

/*public */ErrorEnum Mainboard::powerOn(void)
{
    ErrorEnum error(ERROR_OK);
    
    if (_valid) {
        log("Mainboard: power on...");
        if (_state != STATE_ON) {
            if ((error = pressKey(KEY_POWER, true)) == ERROR_OK) {
                clearWDT();
                wait_ms(STATE_CHANGE_WAIT);
                if (_state != STATE_ON) {
                    if ((error = pressKey(KEY_POWER, true)) == ERROR_OK) {
                        clearWDT();
                        wait_ms(STATE_CHANGE_WAIT);
                        if (_state != STATE_ON) {
                            error = ERROR_FAILED;
                        }
                    }
                }
                if (error == ERROR_OK) {
                    while (_led[LED_POWER_GREEN] != MODE_ON && _led[LED_POWER_RED] == MODE_OFF) {
                        clearWDT();
                    }
                    clearWDT();
                    wait_ms(STATE_MARGIN_WAIT);
                }
            }
        }
        log("%s\n", (_state == STATE_ON) ? ("ok") : ("failed"));
    }
    else {
        error = ERROR_INVALID_STATE;
    }
    return error;
}

/*public */ErrorEnum Mainboard::powerOff(void)
{
    ErrorEnum error(ERROR_OK);
    
    if (_valid) {
        log("Mainboard: power off...");
        if (_state != STATE_OFF) {
            if ((error = pressKey(KEY_POWER, true)) == ERROR_OK) {
                clearWDT();
                wait_ms(STATE_CHANGE_WAIT);
                if (_state != STATE_OFF) {
                    if ((error = pressKey(KEY_POWER, true)) == ERROR_OK) {
                        clearWDT();
                        wait_ms(STATE_CHANGE_WAIT);
                        if (_state != STATE_OFF) {
                            error = ERROR_FAILED;
                        }
                    }
                }
                if (error == ERROR_OK) {
                    while (_led[LED_POWER_GREEN] != MODE_OFF || _led[LED_POWER_RED] != MODE_OFF) {
                        clearWDT();
                    }
                    clearWDT();
                    wait_ms(STATE_MARGIN_WAIT);
                }
            }
        }
        log("%s\n", (_state == STATE_OFF) ? ("ok") : ("failed"));
    }
    else {
        error = ERROR_INVALID_STATE;
    }
    return error;
}

/*public */ErrorEnum Mainboard::pressKey(KeyType key, bool hold)
{
    Timer timer;
    ErrorEnum error(ERROR_OK);
    
    if (_valid) {
        if (!_flag) {
            _key = key;
            _flag = true;
            _irq = false;
            timer.start();
            while (_flag) {
                clearWDT();
                if (timer.read_ms() >= KEYPRESS_TIMEOUT) {
                    _irq = true;
                    _flag = false;
                    error = ERROR_TIMEOUT;
                    break;
                }
            }
            if (error == ERROR_OK) {
                timer.reset();
                clearWDT();
                wait_ms((hold) ? (KEYHOLD_LONG) : (KEYHOLD_SHORT));
                _key = KEY_NONE;
                _flag = true;
                _irq = false;
                timer.start();
                while (_flag) {
                    clearWDT();
                    if (timer.read_ms() >= KEYPRESS_TIMEOUT) {
                        _irq = true;
                        _flag = false;
                        error = ERROR_TIMEOUT;
                        break;
                    }
                }
            }
        }
        else {
            error = ERROR_INVALID_STATE;
        }
    }
    else {
        error = ERROR_INVALID_STATE;
    }
    return error;
}

/*private */void Mainboard::onI2C(void)
{
    static char data[4];
    int size;
    int i;
    
    switch (_i2c.receive()) {
        case I2CSlave::ReadAddressed:
            data[0] = 0x00;
            switch (_command) {
                case COMMAND_READ_KEY:
                    if (_flag) {
                        data[0] = _key;
                    }
                    break;
                case COMMAND_READ_STATE:
                    data[0] = STATE_READY;
                    break;
                default:
                    // nop
                    break;
            }
            _i2c.write(data, 1);
            switch (_command) {
                case COMMAND_READ_KEY:
                    if (_flag) {
                        wait_ms(1);
                        _irq = true;
                        _flag = false;
                    }
                    break;
                default:
                    // nop
                    break;
            }
#if 0
            log("- %02x\n", data[0]);
#endif
            break;
        case I2CSlave::WriteAddressed:
            _command = COMMAND_NONE;
            if ((size = _i2c.read(data, sizeof(data)) - 1) > 0) {
                _command = static_cast<CommandEnum>(data[0]);
                switch (_command) {
                    case COMMAND_READ_KEY:
                    case COMMAND_READ_STATE:
                        // nop
                        break;
                    case COMMAND_WRITE_STATE:
                        if (size > 1) {
                            if ((_state = static_cast<StateEnum>(data[1])) == STATE_OFF) {
                                for (i = 0; i < LED_LIMIT; ++i) {
                                    _led[i] = MODE_OFF;
                                    if (_callback != NULL) {
                                        (*_callback)(static_cast<LEDEnum>(i), _led[i]);
                                    }
                                }
                            }
                        }
                        break;
                    default:
                        if (COMMAND_WRITE_LED <= _command && _command < COMMAND_WRITE_STATE) {
                            i = _command - COMMAND_WRITE_LED;
                            if (0 <= i && i < LED_LIMIT) {
                                if (size > 1) {
                                    _led[i] = static_cast<ModeEnum>(data[1]);
                                    if (_callback != NULL) {
                                        (*_callback)(static_cast<LEDEnum>(i), _led[i]);
                                    }
                                }
                            }
                        }
                        break;
                }
#if 0
                log("+ ");
                for (i = 0; i < size; ++i) {
                    log("%02x ", data[i]);
                }
                log("\n");
#endif
            }
            break;
        default:
            // nop
            break;
    }
    return;
}
