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
 */

#include "GSwifi.h"

void GSwifi::setReset (bool flg) {
    if (flg) {
        // low
        _reset.output();
        _reset = 0;
    } else {
        // high z
        _reset.input();
        _reset.mode(PullNone);
    }
}

void GSwifi::setAlarm (bool flg) {
    if (_alarm == NULL) return;

    if (flg) {
        // low
        _alarm->output(); // low
        _alarm->write(0);
    } else {
        // high
        _alarm->input(); // high
        _alarm->mode(PullUp);
    }
}

void GSwifi::isrUart () {
    recvData(getUart());
}

int GSwifi::getUart () {
#ifdef CFG_UART_DIRECT
    return _uart->RBR;
#else
    return _gs.getc();
#endif
}

void GSwifi::putUart (char c) {
#ifdef CFG_UART_DIRECT
    while(!(_uart->LSR & (1<<5)));
    _uart->THR = c;
#else
    _gs.putc(c);
#endif
}

void GSwifi::setRts (bool flg) {
    if (flg) {
        // low
        if (_flow == 1) {
#if defined(TARGET_LPC1768) || defined(TARGET_LPC2368) || defined(TARGET_LPC11U24) || defined(TARGET_LPC4088) || defined(TARGET_LPC176X) || defined(TARGET_LPC11UXX) || defined(TARGET_LPC408X)
            _uart->MCR |= (1<<6); // RTSEN
#endif
        } else
        if (_flow == 2) {
            if (_rts) {
                _rts->write(0); // low
            }
        }
    } else {
        // high
        if (_flow == 1) {
#if defined(TARGET_LPC1768) || defined(TARGET_LPC2368) || defined(TARGET_LPC11U24) || defined(TARGET_LPC4088) || defined(TARGET_LPC176X) || defined(TARGET_LPC11UXX) || defined(TARGET_LPC408X)
            _uart->MCR &= ~(1<<6); // RTS off
            _uart->MCR &= ~(1<<1);
#endif
        } else
        if (_flow == 2) {
            if (_rts) {
                _rts->write(1); // high
            }
        }
    }
}

int GSwifi::lockUart (int ms) {
    Timer t;

    if (_state.mode != MODE_COMMAND) {
        t.start();
        while (_state.mode != MODE_COMMAND) {
            if (t.read_ms() >= ms) {
                WARN("lock timeout (%d)\r\n", _state.mode);
                return -1;
            }
        }
    }

#ifdef CFG_ENABLE_RTOS
    if (_mutexUart.lock(ms) != osOK) return -1;
#endif

    if (_flow == 1) {
#if defined(TARGET_LPC1768) || defined(TARGET_LPC2368) || defined(TARGET_LPC11U24) || defined(TARGET_LPC4088) || defined(TARGET_LPC176X) || defined(TARGET_LPC11UXX) || defined(TARGET_LPC408X)
        if (!(_uart->MSR & (1<<4))) {
            // CTS check
            t.start();
            while (!(_uart->MSR & (1<<4))) {
                if (t.read_ms() >= ms) {
                    DBG("cts timeout\r\n");
                    return -1;
                }
            }
        }
#endif
    } else
    if (_flow == 2) {
        if (_cts && _cts->read()) {
            // CTS check
            t.start();
            while (_cts->read()) {
                if (t.read_ms() >= ms) {
                    DBG("cts timeout\r\n");
                    return -1;
                }
            }
        }
    }

    setRts(false);
    return 0;
}

void GSwifi::unlockUart () {
    setRts(true);
#ifdef CFG_ENABLE_RTOS
    _mutexUart.unlock();
#endif
}

void GSwifi::initUart (PinName cts, PinName rts, PinName alarm, int baud) {

    _baud = baud;
    if (_baud) _gs.baud(_baud);
    _gs.attach(this, &GSwifi::isrUart, Serial::RxIrq);

    _cts = NULL;
    _rts = NULL;
    _flow = 0;
#if defined(TARGET_LPC1768) || defined(TARGET_LPC2368) || defined(TARGET_LPC176X)
    _uart = LPC_UART1;
    if (cts == p12) { // CTS input (P0_17)
        _uart->MCR |= (1<<7); // CTSEN
        LPC_PINCON->PINSEL1 &= ~(3 << 2);
        LPC_PINCON->PINSEL1 |= (1 << 2); // UART CTS
    } else
    if (cts != NC) {
        _cts = new DigitalIn(cts);
    }
    if (rts == P0_22) { // RTS output (P0_22)
        _uart->MCR |= (1<<6); // RTSEN
        LPC_PINCON->PINSEL1 &= ~(3 << 12);
        LPC_PINCON->PINSEL1 |= (1 << 12); // UART RTS
        _flow = 1;
    } else
    if (rts != NC) {
        _rts = new DigitalOut(rts);
        _flow = 2;
    }
#elif defined(TARGET_LPC11U24) || defined(TARGET_LPC11UXX)
    _uart = LPC_USART;
    if (cts == p21) { // CTS input (P0_7)
        _uart->MCR |= (1<<7); // CTSEN
        LPC_IOCON->PIO0_7 &= ~0x07;
        LPC_IOCON->PIO0_7 |= 0x01; // UART CTS
    } else
    if (cts != NC) {
        _cts = new DigitalIn(cts);
    }
    if (rts == p22) { // RTS output (P0_17)
        _uart->MCR |= (1<<6); // RTSEN
        LPC_IOCON->PIO0_17 &= ~0x07;
        LPC_IOCON->PIO0_17 |= 0x01; // UART RTS
        _flow = 1;
    } else
    if (rts != NC) {
        _rts = new DigitalOut(rts);
        _flow = 2;
    }
#elif defined(TARGET_LPC4088) || defined(TARGET_LPC408X)
    _uart = (LPC_UART1_TypeDef*)LPC_UART2;
    if (cts != NC) {
        _cts = new DigitalIn(cts);
    }
    if (rts != NC) {
        _rts = new DigitalOut(rts);
        _flow = 2;
    }
#else
    if (cts != NC) {
        _cts = new DigitalIn(cts);
    }
    if (rts != NC) {
        _rts = new DigitalOut(rts);
        _flow = 2;
    }
#endif

    if (alarm != NC) {
        _alarm = new DigitalInOut(alarm);
    } else {
        _alarm = NULL;
    }
}
