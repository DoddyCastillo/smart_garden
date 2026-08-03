#ifndef I2C_ASYNC_H
#define I2C_ASYNC_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TWI_FREQ 100000L

typedef enum { I2C_STATE_IDLE = 0, I2C_STATE_START, I2C_STATE_DONE, I2C_STATE_ERROR } I2C_State;

typedef struct {
    uint8_t address;

    uint8_t * txBuffer;
    uint8_t txLength;
    uint8_t txIndex;

    uint8_t * rxBuffer;
    uint8_t rxLength;
    uint8_t rxIndex;

    I2C_State state;
    void (*onComplete)(bool success);
} I2C_Transaction;

void I2C_Init(void);
bool I2C_BeginTransmission(uint8_t addr, uint8_t * data, uint8_t len,
                           void (*callback)(bool success));
bool I2C_RequestFrom(uint8_t addr, uint8_t * buf, uint8_t len, void (*callback)(bool success));
I2C_State I2C_GetState(void);

#ifdef __cplusplus
}
#endif

#endif /* I2C_ASYNC_H */
