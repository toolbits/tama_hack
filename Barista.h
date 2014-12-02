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
**      Barista.h
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

#ifndef __BARISTA_H
#define __BARISTA_H

enum CommandEnum {
    COMMAND_NONE                = -1,
    COMMAND_READ_KEY            = 0x00,
    COMMAND_WRITE_LED           = 0x10,
    COMMAND_WRITE_STATE         = 0x20,
    COMMAND_READ_STATE          = 0x80
};
enum StateEnum {
    STATE_ON                    = 0x00,
    STATE_OFF                   = 0x01,
    STATE_READY                 = 0x09
};
enum LEDEnum {
    LED_POWER_GREEN,
    LED_POWER_RED,
    LED_MAINTENANCE,
    LED_CLEANING,
    LED_SUPPLY,
    LED_ESPRESSO,
    LED_CAFFELATTE,
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
enum KeyEnum {
    KEY_NONE                    = 0x00,
    KEY_POWER                   = 0x01 << 0,
    KEY_ESPRESSO                = 0x01 << 1,
    KEY_CAFFELATTE              = 0x01 << 2,
    KEY_CAPPUCCINO              = 0x01 << 3,
    KEY_BLACKMAG                = 0x01 << 4,
    KEY_BLACK                   = 0x01 << 5
};
typedef unsigned char           KeyType;

#endif
