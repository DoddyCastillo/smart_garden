#include "i2c_async.h"
#include "Arduino.h"

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif

#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

volatile I2C_Transaction i2cTransaction;

static void i2c_setAddress(uint8_t address) {
    i2cTransaction.address = address << 1; // Shift left for write mode
}

void I2C_Init(void) {

    i2cTransaction.state = I2C_STATE_IDLE;

    digitalWrite(SDA, 1);
    digitalWrite(SCL, 1);

    cbi(TWSR, TWPS0);
    cbi(TWSR, TWPS1);

    TWBR = ((F_CPU / TWI_FREQ) - 16) / 2;

    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
}

// void I2C_Init(uint32_t freqHz) {
//     TWSR = 0x00; // Prescaler = 1
//     TWBR = ((F_CPU / freqHz) - 16) / 2;
//     TWCR = (1 << TWEN) | (1 << TWIE);
//     i2cTransaction.state = I2C_STATE_IDLE;
// }

bool I2C_BeginTransmission(uint8_t addr, uint8_t * data, uint8_t len,
                           void (*callback)(bool success)) {
    if (i2cTransaction.state != I2C_STATE_IDLE)
        return false;

    i2c_setAddress(addr);
    i2cTransaction.txBuffer = data;
    i2cTransaction.txLength = len;
    i2cTransaction.txIndex = 0;
    i2cTransaction.onComplete = callback;
    i2cTransaction.state = I2C_STATE_START;

    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN) | (1 << TWIE);
    return true;
}

bool I2C_RequestFrom(uint8_t addr, uint8_t * buf, uint8_t len, void (*callback)(bool success)) {
    if (i2cTransaction.state != I2C_STATE_IDLE)
        return false;

    i2cTransaction.address = (addr << 1) | 0x01; // Read mode
    i2cTransaction.rxBuffer = buf;
    i2cTransaction.rxLength = len;
    i2cTransaction.rxIndex = 0;
    i2cTransaction.onComplete = callback;
    i2cTransaction.state = I2C_STATE_START;

    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN) | (1 << TWIE);
    return true;
}

I2C_State I2C_GetState(void) {
    return i2cTransaction.state;
}

ISR(TWI_vect) {
    uint8_t status = TWSR & 0xF8;

    switch (status) {
    case 0x08: // START transmitted
    case 0x10: // Repeated START
        TWDR = i2cTransaction.address;
        TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
        break;

    case 0x18: // SLA+W transmitted, ACK received
        TWDR = i2cTransaction.txBuffer[i2cTransaction.txIndex++];
        TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
        break;

    case 0x28: // Data transmitted, ACK received
        if (i2cTransaction.txIndex < i2cTransaction.txLength) {
            TWDR = i2cTransaction.txBuffer[i2cTransaction.txIndex++];
            TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
        } else {
            // Stop condition
            TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
            i2cTransaction.state = I2C_STATE_DONE;
            if (i2cTransaction.onComplete)
                i2cTransaction.onComplete(true);
            i2cTransaction.state = I2C_STATE_IDLE;
        }
        break;

    case 0x40: // SLA+R transmitted, ACK received
        if (i2cTransaction.rxLength > 1)
            TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA) | (1 << TWIE);
        else
            TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
        break;

    case 0x50: // Data received, ACK returned
        i2cTransaction.rxBuffer[i2cTransaction.rxIndex++] = TWDR;
        if (i2cTransaction.rxIndex < (i2cTransaction.rxLength - 1))
            TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA) | (1 << TWIE);
        else
            TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
        break;

    case 0x58: // Data received, NACK returned
        i2cTransaction.rxBuffer[i2cTransaction.rxIndex++] = TWDR;
        TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
        i2cTransaction.state = I2C_STATE_DONE;
        if (i2cTransaction.onComplete)
            i2cTransaction.onComplete(true);
        i2cTransaction.state = I2C_STATE_IDLE;
        break;

    default:
        // Error o NACK
        i2cTransaction.state = I2C_STATE_ERROR;
        TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
        if (i2cTransaction.onComplete)
            i2cTransaction.onComplete(false);
        i2cTransaction.state = I2C_STATE_IDLE;
        break;
    }
}
