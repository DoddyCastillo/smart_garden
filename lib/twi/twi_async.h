#ifndef TWI_ASYNC_H
#define TWI_ASYNC_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdint.h>

#define TWI_FREQ 10000L

typedef enum {
    TWI_ASYNC_STATE_IDLE,
    TWI_ASYNC_STATE_START,
    TWI_ASYNC_STATE_SEND_ADDR,
    TWI_ASYNC_STATE_SEND_DATA,
    TWI_ASYNC_STATE_RECEIVE_DATA,
    TWI_ASYNC_STATE_STOP,
    TWI_ASYNC_STATE_DONE,
    TWI_ASYNC_STATE_ERROR
} TWI_ASYNC_State;

typedef struct {
    uint8_t address;
    uint8_t * txBuffer;
    uint8_t txLength;
    uint8_t txIndex;
    uint8_t * rxBuffer;
    uint8_t rxLength;
    uint8_t rxIndex;
    TWI_ASYNC_State state;
    void (*onComplete)(bool success);
} TWI_Transaction;

void twi_async_init(void);

#endif
