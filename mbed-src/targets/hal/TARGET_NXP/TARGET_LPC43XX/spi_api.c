/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Ported to NXP LPC43XX by Micromint USA <support@micromint.com>
 */
#include "mbed_assert.h"
#include <math.h>

#include "spi_api.h"
#include "cmsis.h"
#include "pinmap.h"
#include "mbed_error.h"

// SCU mode for SPI pins
#define SCU_PINIO_SPI       SCU_PINIO_FAST

static const PinMap PinMap_SPI_SCLK[] = {
    {P1_19, SPI_1, (SCU_PINIO_SPI | 1)},
    {P3_0,  SPI_0, (SCU_PINIO_SPI | 4)},
    {P3_3,  SPI_0, (SCU_PINIO_SPI | 2)},
    {PF_0,  SPI_0, (SCU_PINIO_SPI | 0)},
    {PF_4,  SPI_1, (SCU_PINIO_SPI | 0)},
    {NC,    NC,    0}
};

static const PinMap PinMap_SPI_MOSI[] = {
    {P0_1,  SPI_1, (SCU_PINIO_SPI | 1)},
    {P1_2,  SPI_0, (SCU_PINIO_SPI | 5)},
    {P1_4,  SPI_1, (SCU_PINIO_SPI | 5)},
    {P3_7,  SPI_0, (SCU_PINIO_SPI | 5)},
    {P3_8,  SPI_0, (SCU_PINIO_SPI | 2)},
    {P9_2,  SPI_0, (SCU_PINIO_SPI | 7)},
    {PF_3,  SPI_0, (SCU_PINIO_SPI | 2)},
    {PF_7,  SPI_1, (SCU_PINIO_SPI | 2)},
    {NC,    NC,    0}
};

static const PinMap PinMap_SPI_MISO[] = {
    {P0_0,  SPI_1, (SCU_PINIO_SPI | 1)},
    {P1_1,  SPI_0, (SCU_PINIO_SPI | 5)},
    {P1_3,  SPI_1, (SCU_PINIO_SPI | 5)},
    {P3_6,  SPI_0, (SCU_PINIO_SPI | 5)},
    {P3_7,  SPI_0, (SCU_PINIO_SPI | 2)},
    {P9_1,  SPI_0, (SCU_PINIO_SPI | 7)},
    {PF_2,  SPI_0, (SCU_PINIO_SPI | 2)},
    {PF_6,  SPI_1, (SCU_PINIO_SPI | 2)},
    {NC,    NC,    0}
};

static const PinMap PinMap_SPI_SSEL[] = {
    {P1_0,  SPI_0, (SCU_PINIO_SPI | 5)},
    {P1_5,  SPI_1, (SCU_PINIO_SPI | 5)},
    {P1_20, SPI_1, (SCU_PINIO_SPI | 2)},
    {P3_6,  SPI_0, (SCU_PINIO_SPI | 2)},
    {P3_8,  SPI_0, (SCU_PINIO_SPI | 5)},
    {P9_0,  SPI_0, (SCU_PINIO_SPI | 7)},
    {PF_1,  SPI_0, (SCU_PINIO_SPI | 2)},
    {PF_5,  SPI_1, (SCU_PINIO_SPI | 2)},
    {NC,    NC,    0}
};

static inline int ssp_disable(spi_t *obj);
static inline int ssp_enable(spi_t *obj);

void spi_init(spi_t *obj, PinName mosi, PinName miso, PinName sclk, PinName ssel) {
    // determine the SPI to use
    SPIName spi_mosi = (SPIName)pinmap_peripheral(mosi, PinMap_SPI_MOSI);
    SPIName spi_miso = (SPIName)pinmap_peripheral(miso, PinMap_SPI_MISO);
    SPIName spi_sclk = (SPIName)pinmap_peripheral(sclk, PinMap_SPI_SCLK);
    SPIName spi_ssel = (SPIName)pinmap_peripheral(ssel, PinMap_SPI_SSEL);
    SPIName spi_data = (SPIName)pinmap_merge(spi_mosi, spi_miso);
    SPIName spi_cntl = (SPIName)pinmap_merge(spi_sclk, spi_ssel);

    obj->spi = (LPC_SSP_T*)pinmap_merge(spi_data, spi_cntl);
    MBED_ASSERT((int)obj->spi != NC);
    
    // enable clocking
    switch ((int)obj->spi) {
        case SPI_0: LPC_CGU->BASE_CLK[CLK_BASE_SSP0] = (1 << 11) | (CLKIN_MAINPLL << 24); break;
        case SPI_1: LPC_CGU->BASE_CLK[CLK_BASE_SSP1] = (1 << 11) | (CLKIN_MAINPLL << 24); break;
    }

    // set default format and frequency
    if (ssel == NC) {
        spi_format(obj, 8, 0, 0);  // 8 bits, mode 0, master
    } else {
        spi_format(obj, 8, 0, 1);  // 8 bits, mode 0, slave
    }
    spi_frequency(obj, 1000000);
    
    // enable the ssp channel
    ssp_enable(obj);
    
    // pin out the spi pins
    pinmap_pinout(mosi, PinMap_SPI_MOSI);
    pinmap_pinout(miso, PinMap_SPI_MISO);
    pinmap_pinout(sclk, PinMap_SPI_SCLK);
    if (ssel != NC) {
        pinmap_pinout(ssel, PinMap_SPI_SSEL);
    }
}

void spi_free(spi_t *obj) {}

void spi_format(spi_t *obj, int bits, int mode, int slave) {
    MBED_ASSERT(((bits >= 4) && (bits <= 16)) || ((mode >= 0) && (mode <= 3)));
    ssp_disable(obj);
    
    int polarity = (mode & 0x2) ? 1 : 0;
    int phase = (mode & 0x1) ? 1 : 0;
    
    // set it up
    int DSS = bits - 1;            // DSS (data select size)
    int SPO = (polarity) ? 1 : 0;  // SPO - clock out polarity
    int SPH = (phase) ? 1 : 0;     // SPH - clock out phase
    
    int FRF = 0;                   // FRF (frame format) = SPI
    uint32_t tmp = obj->spi->CR0;
    tmp &= ~(0xFFFF);
    tmp |= DSS << 0
        | FRF << 4
        | SPO << 6
        | SPH << 7;
    obj->spi->CR0 = tmp;
    
    tmp = obj->spi->CR1;
    tmp &= ~(0xD);
    tmp |= 0 << 0                   // LBM - loop back mode - off
        | ((slave) ? 1 : 0) << 2   // MS - master slave mode, 1 = slave
        | 0 << 3;                  // SOD - slave output disable - na
    obj->spi->CR1 = tmp;
    ssp_enable(obj);
}

void spi_frequency(spi_t *obj, int hz) {
    ssp_disable(obj);
    
    uint32_t PCLK = SystemCoreClock;
    
    int prescaler;
    
    for (prescaler = 2; prescaler <= 254; prescaler += 2) {
        int prescale_hz = PCLK / prescaler;
        
        // calculate the divider
        int divider = floor(((float)prescale_hz / (float)hz) + 0.5f);
        
        // check we can support the divider
        if (divider < 256) {
            // prescaler
            obj->spi->CPSR = prescaler;
            
            // divider
            obj->spi->CR0 &= ~(0xFFFF << 8);
            obj->spi->CR0 |= (divider - 1) << 8;
            ssp_enable(obj);
            return;
        }
    }
    error("Couldn't setup requested SPI frequency");
}

static inline int ssp_disable(spi_t *obj) {
    return obj->spi->CR1 &= ~(1 << 1);
}

static inline int ssp_enable(spi_t *obj) {
    return obj->spi->CR1 |= (1 << 1);
}

static inline int ssp_readable(spi_t *obj) {
    return obj->spi->SR & (1 << 2);
}

static inline int ssp_writeable(spi_t *obj) {
    return obj->spi->SR & (1 << 1);
}

static inline void ssp_write(spi_t *obj, int value) {
    while (!ssp_writeable(obj));
    obj->spi->DR = value;
}

static inline int ssp_read(spi_t *obj) {
    while (!ssp_readable(obj));
    return obj->spi->DR;
}

static inline int ssp_busy(spi_t *obj) {
    return (obj->spi->SR & (1 << 4)) ? (1) : (0);
}

int spi_master_write(spi_t *obj, int value) {
    ssp_write(obj, value);
    return ssp_read(obj);
}

int spi_slave_receive(spi_t *obj) {
    return (ssp_readable(obj) && !ssp_busy(obj)) ? (1) : (0);
}

int spi_slave_read(spi_t *obj) {
    return obj->spi->DR;
}

void spi_slave_write(spi_t *obj, int value) {
    while (ssp_writeable(obj) == 0) ;
    obj->spi->DR = value;
}

int spi_busy(spi_t *obj) {
    return ssp_busy(obj);
}
