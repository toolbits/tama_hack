#include "I2CSlaveX.h"

#if DEVICE_I2CSLAVE

namespace mbed {

I2CSlaveX* instance;

I2CSlaveX::I2CSlaveX(PinName sda, PinName scl) : I2CSlave(sda, scl) {
}

// Add optional index argument for slaves that accept multiple addresses
void I2CSlaveX::address(int address, int idx) {
    int addr = (address & 0xFF) | 1;
    i2c_slave_address(&_i2c, idx, addr, 0);
}

// Add optional argument that will be set with the received data value (usually slave address)
int I2CSlaveX::receive(char *data) {
    if(data) {
        *data = _i2c.i2c->I2DAT;
    }
    return i2c_slave_receive(&_i2c);
}

// Return read length rather than match/no match
int I2CSlaveX::read(char *data, int length) {
    return i2c_slave_read(&_i2c, data, length);
}

// Return write length rather than match/no match
int I2CSlaveX::write(const char *data, int length) {
    return i2c_slave_write(&_i2c, data, length);
}

void I2CSlaveX::attach(void (*fptr)(void)) {
    if (fptr) {
        _receive.attach(fptr);
        enable_irq();
    } else {
        disable_irq();
    }
}

void I2CSlaveX::_irq_handler(uint32_t id, uint8_t addr, uint8_t state) {
    instance->_receive.call();
}

void I2CSlaveX::enable_irq() {
    // Hacky way to call this instance
    instance = this;
    NVIC_SetVector(I2C2_IRQn, (uint32_t)(&I2CSlaveX::_irq_handler));
    NVIC_EnableIRQ(I2C2_IRQn);    
}

void I2CSlaveX::disable_irq() {
    NVIC_DisableIRQ(I2C2_IRQn);    
}

}

#endif