#include "DS18B20.h"

float DS18B20_Temperature = 0.0f;
bool DS18B20_Valid = false;

typedef enum {
    DS18B20_STATE_IDLE = 0,
    DS18B20_STATE_WAIT_CONV,
    DS18B20_STATE_READ_SCRATCHPAD
} DS18B20_State_t;

static DS18B20_State_t dsState = DS18B20_STATE_IDLE;
static uint16_t dsPeriodTicks = 0;
static uint16_t dsWaitTicks = 0;

#define DS18B20_CONV_TICKS   (DS18B20_CONV_TIME_MS / DS18B20_TASK_TICK_MS)
#define DS18B20_PERIOD_TICKS (DS18B20_PERIOD_MS / DS18B20_TASK_TICK_MS)

// Primitivas de temporización de nivel bajo del protocolo 1-Wire
static bool onewire_reset(void) {
    pinMode(DS18B20_PIN, OUTPUT);
    digitalWrite(DS18B20_PIN, LOW);
    delayMicroseconds(480);

    noInterrupts();
    pinMode(DS18B20_PIN, INPUT_PULLUP);
    delayMicroseconds(70);
    bool presence = (digitalRead(DS18B20_PIN) == LOW);
    interrupts();

    delayMicroseconds(410);
    return presence;
}

static void onewire_write_bit(uint8_t bit) {
    pinMode(DS18B20_PIN, OUTPUT);
    digitalWrite(DS18B20_PIN, LOW);

    if (bit) {
        delayMicroseconds(6);
        pinMode(DS18B20_PIN, INPUT_PULLUP);
        delayMicroseconds(64);
    } else {
        delayMicroseconds(60);
        pinMode(DS18B20_PIN, INPUT_PULLUP);
        delayMicroseconds(10);
    }
}

static uint8_t onewire_read_bit(void) {
    uint8_t bit = 0;
    pinMode(DS18B20_PIN, OUTPUT);
    digitalWrite(DS18B20_PIN, LOW);
    delayMicroseconds(2);

    pinMode(DS18B20_PIN, INPUT_PULLUP);
    delayMicroseconds(10);

    if (digitalRead(DS18B20_PIN)) {
        bit = 1;
    }
    delayMicroseconds(50);
    return bit;
}

static void onewire_write_byte(uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
        onewire_write_bit(data & 0x01);
        data >>= 1;
    }
}

static uint8_t onewire_read_byte(void) {
    uint8_t data = 0;
    for (uint8_t i = 0; i < 8; i++) {
        if (onewire_read_bit()) {
            data |= (1 << i);
        }
    }
    return data;
}

// Algoritmo Checksum CRC-8 Dallas/Maxim (polinomio X^8 + X^5 + X^4 + 1)
static uint8_t ds18b20_crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0x00;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t inbyte = data[i];
        for (uint8_t j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) {
                crc ^= 0x8C;
            }
            inbyte >>= 1;
        }
    }
    return crc;
}

void DS18B20_Init(void) {
    dsState = DS18B20_STATE_IDLE;
    dsPeriodTicks = 0;
    dsWaitTicks = 0;
    DS18B20_Temperature = 0.0f;
    DS18B20_Valid = false;
    pinMode(DS18B20_PIN, INPUT_PULLUP);
}

void DS18B20_Tick(void) {
    switch (dsState) {

    case DS18B20_STATE_IDLE:
        if (++dsPeriodTicks >= DS18B20_PERIOD_TICKS) {
            dsPeriodTicks = 0;
            if (onewire_reset()) {
                onewire_write_byte(0xCC); // Skip ROM
                onewire_write_byte(0x44); // Convert T
                dsWaitTicks = DS18B20_CONV_TICKS;
                dsState = DS18B20_STATE_WAIT_CONV;
            } else {
                DS18B20_Valid = false;
            }
        }
        break;

    case DS18B20_STATE_WAIT_CONV:
        if (dsWaitTicks > 0) {
            dsWaitTicks--;
        } else {
            dsState = DS18B20_STATE_READ_SCRATCHPAD;
        }
        break;

    case DS18B20_STATE_READ_SCRATCHPAD:
        if (onewire_reset()) {
            onewire_write_byte(0xCC); // Skip ROM
            onewire_write_byte(0xBE); // Read Scratchpad

            uint8_t scratchpad[9];
            for (uint8_t i = 0; i < 9; i++) {
                scratchpad[i] = onewire_read_byte();
            }

            if (ds18b20_crc8(scratchpad, 8) == scratchpad[8]) {
                int16_t rawTemp = (int16_t)((uint16_t)scratchpad[1] << 8 | scratchpad[0]);
                DS18B20_Temperature = (float)rawTemp / 16.0f;
                DS18B20_Valid = true;
            } else {
                DS18B20_Valid = false;
            }
        } else {
            DS18B20_Valid = false;
        }
        dsState = DS18B20_STATE_IDLE;
        break;

    default:
        dsState = DS18B20_STATE_IDLE;
        break;
    }
}
