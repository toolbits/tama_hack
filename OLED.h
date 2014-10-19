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
**      OLED.h
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

#ifndef __OLED_H
#define __OLED_H

#include "type.h"
#include "SoftwareI2C.h"

class OLED {
    public:
        enum ScrollEnum {
            SCROLL_FIXED,
            SCROLL_FIT,
            SCROLL_OVER
        };
    
    private:
        struct ItemRec {
            ScrollEnum          scroll;
            string              data;
            short               limit;
            short               timeup;
            short volatile      count;
            short volatile      offset;
            short volatile      current;
        };
    
    private:
                SoftwareI2C     _i2c;
                Ticker          _ticker;
                deque<ItemRec>  _item[2];
                bool            _valid;
    
    public:
        explicit                OLED            (PinName sda, PinName scl);
                                ~OLED           (void);
                int             getSize         (int line) const;
                bool            isEmpty         (int line) const;
                void            setup           (void);
                void            cleanup         (void);
                void            print           (int line, ScrollEnum scroll, int second, int immediate, char const* format, va_list ap);
                void            print           (int line, ScrollEnum scroll, int second, int immediate, char const* format, ...);
                void            print           (int line, ScrollEnum scroll, int second, char const* format, ...);
                void            print           (int line, ScrollEnum scroll, char const* format, ...);
                void            print           (int line, char const* format, ...);
    private:
                void            onTicker        (void);
                void            attach          (void);
                void            detach          (void);
                void            writeString     (char const* param, int length = -1);
                void            writeCommand    (unsigned char param);
                void            writeData       (unsigned char param);
    private:
                                OLED            (OLED const&);
                OLED&           operator=       (OLED const&);
};

/*public */inline OLED::OLED(PinName sda, PinName scl) : _i2c(sda, scl), _valid(false)
{
}

/*public */inline OLED::~OLED(void)
{
    cleanup();
}

#endif
