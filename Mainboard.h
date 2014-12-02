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
**      Mainboard.h
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

#ifndef __MAINBOARD_H
#define __MAINBOARD_H

#include "type.h"
#include "I2CSlaveX.h"

class Mainboard {
    private:
                I2CSlaveX               _i2c;
                DigitalInOut            _irq;
                CommandEnum volatile    _command;
                StateEnum volatile      _state;
                ModeEnum volatile       _led[LED_LIMIT];
                bool volatile           _flag;
                KeyType volatile        _key;
                void                    (*_callback)        (LEDEnum, ModeEnum);
                bool                    _valid;
    
    public:
        explicit                        Mainboard           (PinName sda, PinName scl, PinName irq);
                                        ~Mainboard          (void);
                ModeEnum                getLED              (LEDEnum led) const;
                bool                    isOn                (void) const;
                bool                    isReady             (void) const;
                bool                    isErrorMaintenance  (void) const;
                bool                    isErrorCleaning     (void) const;
                bool                    isErrorSupply       (void) const;
                void                    setup               (void(*callback)(LEDEnum, ModeEnum) = NULL);
                void                    cleanup             (void);
                ErrorEnum               synchronize         (void);
                ErrorEnum               powerOn             (void);
                ErrorEnum               powerOff            (void);
                ErrorEnum               pressKey            (KeyType key, bool hold);
    private:
                void                    onI2C               (void);
    private:
                                        Mainboard           (Mainboard const&);
                Mainboard&              operator=           (Mainboard const&);
};

/*public */inline Mainboard::Mainboard(PinName sda, PinName scl, PinName irq) : _i2c(sda, scl), _irq(irq), _valid(false)
{
}

/*public */inline Mainboard::~Mainboard(void)
{
    cleanup();
}

#endif
