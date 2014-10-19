#ifndef MBED_I2CSLAVEX_H
#define MBED_I2CSLAVEX_H

#include "mbed.h"

namespace mbed {
    
/** An I2C Slave, used for communicating with an I2C Master device with support for Interrupts and multiple slave addresses
 *
 * Warning: Currently only somewhat tested with LPC1347. This should be concidered a proof of concept. Ideally this would
 * be merged into the original I2CSlave library.
 *
 * Example:
 * @code
 * // Simple Simulated I2C Read-Only EEPROM
 * #include <mbed.h>
 * #include "I2CSlaveX.h"
 * 
 * I2CSlaveX slave(p9, p10);
 * 
 * uint8_t eeprom_data[16] = {0);
 * uint8_t eeprom_addr = 0;
 * 
 * void receive_handler() {
 *     char addr;
 *     int i = slave.receive(&addr);
 *     switch (i) {
 *         case I2CSlave::ReadAddressed:
 *             slave.write(&eeprom_data[eeprom_addr], 1);
 *             printf("Write @ %02X [%02X] : %02X\n", addr, eeprom_addr, eeprom_data[eeprom_addr]);
 *             break;
 *         case I2CSlave::WriteAddressed:
 *             slave.read(&eeprom_addr, 1);
 *             printf("Read  @ %02X [%02X]\n", addr, eeprom_addr);
 *             break;
 *     }
 * }
 * 
 * int main() {
 *     slave.address(0xA0, 0);
 *     slave.address(0x30, 1);
 *     slave.attach(&receive_handler);
 *     
 *     while(1) {
 *         printf("Nothing to do really...");
 *         eeprom_data[0]++;
 *         wait(1);
 *     }
 * }
 * @endcode
 */ 
class I2CSlaveX : public I2CSlave
{
public:
    /** Create an I2C Slave interface, connected to the specified pins.
     *
     *  @param sda I2C data line pin
     *  @param scl I2C clock line pin
     */
    I2CSlaveX(PinName sda, PinName scl);
 
    /** Checks to see if this I2C Slave has been addressed.
     *
     *  @returns
     *  A status indicating if the device has been addressed, and how
     *  - NoData            - the slave has not been addressed
     *  - ReadAddressed     - the master has requested a read from this slave
     *  - WriteAddressed    - the master is writing to this slave
     *  - WriteGeneral      - the master is writing to all slave
     */
    int receive(char *data = NULL);

    /** Read from an I2C master.
     *
     *  @param data pointer to the byte array to read data in to
     *  @param length maximum number of bytes to read
     *
     *  @returns
     *       0 on success,
     *   non-0 otherwise
     */
    int read(char *data, int length);

    /** Write to an I2C master.
     *
     *  @param data pointer to the byte array to be transmitted
     *  @param length the number of bytes to transmite
     *
     *  @returns
     *       0 on success,
     *   non-0 otherwise
     */
    int write(const char *data, int length);

    /** Sets the I2C slave address.
     *
     *  @param address The address to set for the slave (ignoring the least
     *  signifcant bit). If set to 0, the slave will only respond to the
     *  general call address.
     */
    void address(int address, int idx = 0);
    
    /** Attach a function to call when state changed event occured on the I2C peripheral
     *
     *  @param fptr A pointer to a void function, or 0 to set as none
     */
    void attach(void (*fptr)(void));

    /** Attach a member function to call when state changed event occured on the I2C peripheral
     *
     *  @param tptr pointer to the object to call the member function on
     *  @param mptr pointer to the member function to be called
     */
    template<typename T>
    void attach(T* tptr, void (T::*mptr)(void)) {
        _receive.attach(tptr, mptr);
        enable_irq();
    }
    
    /** Enable IRQ. This method depends on hw implementation, might enable one
     *  port interrupts. For further information, check gpio_irq_enable().
     */
    void enable_irq();

    /** Disable IRQ. This method depends on hw implementation, might disable one
     *  port interrupts. For further information, check gpio_irq_disable().
     */
    void disable_irq();

    static void _irq_handler(uint32_t id, uint8_t addr, uint8_t state);
    
protected:
    FunctionPointer _receive;    
    
};

}
 
#endif
