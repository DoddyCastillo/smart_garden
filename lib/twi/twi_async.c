#include "twi_async.h"
#include "Arduino.h"

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif

#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

static volatile TWI_Transaction twiAsyncState;

static void twi_async_setAddress(uint8_t address);

static void twi_async_setAddress(uint8_t address) {
    twiAsyncState.address = address << 1; // Shift left for write mode
}

void twi_async_init(void) {
    twiAsyncState.state = TWI_ASYNC_STATE_IDLE;

    digitalWrite(SDA, 1);
    digitalWrite(SCL, 1);

    cbi(TWSR, TWPS0);
    cbi(TWSR, TWPS1);

    TWBR = ((F_CPU / TWI_FREQ) - 16) / 2;

    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
}
