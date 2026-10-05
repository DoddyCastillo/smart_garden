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

static volatile uint8_t *reg_port = nullptr;
static volatile uint8_t *reg_ddr  = nullptr;
static volatile uint8_t *reg_pin  = nullptr;
static uint8_t pin_mask = 0;

// Salida LOW activa en 1 ciclo de reloj
static inline void onewire_pin_low(void) {
    *reg_ddr  |= pin_mask;
    *reg_port &= ~pin_mask;
}

// Entrada con pull-up activo (libera el bus a VDD o permite al esclavo forzar nivel bajo)
static inline void onewire_pin_high(void) {
    *reg_ddr  &= ~pin_mask;
    *reg_port |= pin_mask;
}

// Lectura directa en registro PIN (1 ciclo de reloj)
static inline uint8_t onewire_pin_read(void) {
    return (*reg_pin & pin_mask) ? 1 : 0;
}

// Primitivas de temporización de nivel bajo del protocolo 1-Wire
static bool onewire_reset(void) {
    noInterrupts();
    onewire_pin_low();
    interrupts();
    delayMicroseconds(480);

    noInterrupts();
    onewire_pin_high();
    delayMicroseconds(70);
    bool presence = (onewire_pin_read() == 0);
    interrupts();

    delayMicroseconds(410);
    return presence;
}

static void onewire_write_bit(uint8_t bit) {
    if (bit) {
        // Escribir '1': pull-down corto (3 us), liberar bus y esperar recuperación
        noInterrupts();
        onewire_pin_low();
        delayMicroseconds(3);
        onewire_pin_high();
        interrupts();
        delayMicroseconds(60);
    } else {
        // Escribir '0': pull-down largo (60 us), liberar bus y esperar recuperación
        noInterrupts();
        onewire_pin_low();
        delayMicroseconds(60);
        onewire_pin_high();
        interrupts();
        delayMicroseconds(10);
    }
}

static uint8_t onewire_read_bit(void) {
    uint8_t bit = 0;
    noInterrupts();
    onewire_pin_low();
    delayMicroseconds(2);
    onewire_pin_high();
    delayMicroseconds(8);
    // Muestreo preciso a los 10-11 us desde el flanco de bajada (estándar Dallas 1-Wire)
    bit = onewire_pin_read();
    interrupts();
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
    pin_mask = digitalPinToBitMask(DS18B20_PIN);
    uint8_t port = digitalPinToPort(DS18B20_PIN);
    reg_port = portOutputRegister(port);
    reg_ddr  = portModeRegister(port);
    reg_pin  = portInputRegister(port);

    onewire_pin_high(); // Modo entrada pull-up

    dsState = DS18B20_STATE_IDLE;
    dsPeriodTicks = DS18B20_PERIOD_TICKS; // Listo para disparar medición en el primer tick
    dsWaitTicks = 0;
    DS18B20_Temperature = 0.0f;
    DS18B20_Valid = false;
}

void DS18B20_Trigger(void) {
    dsState = DS18B20_STATE_IDLE;
    dsPeriodTicks = DS18B20_PERIOD_TICKS; // Disparar conversión 1-Wire inmediatamente
}

void DS18B20_Tick(void) {
    switch (dsState) {

    case DS18B20_STATE_IDLE:
        if (++dsPeriodTicks >= DS18B20_PERIOD_TICKS) {
            dsPeriodTicks = 0;
            if (onewire_reset()) {
                Serial.println(F("    [DS18B20] Presencia 1-Wire detectada. Enviando Convert T (0x44)..."));
                onewire_write_byte(0xCC); // Skip ROM
                onewire_write_byte(0x44); // Convert T
                dsWaitTicks = DS18B20_CONV_TICKS;
                dsState = DS18B20_STATE_WAIT_CONV;
            } else {
                Serial.println(F("    [DS18B20] Error: Sin pulso de presencia 1-Wire en Pin 4."));
                DS18B20_Valid = false;
                // Reintentar en 250 ms (25 ticks)
                dsPeriodTicks = DS18B20_PERIOD_TICKS - 25;
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

            uint8_t crc = ds18b20_crc8(scratchpad, 8);
            bool all_zeros = true;
            bool all_ones = true;
            for (uint8_t i = 0; i < 9; i++) {
                if (scratchpad[i] != 0x00) all_zeros = false;
                if (scratchpad[i] != 0xFF) all_ones = false;
            }

            if (crc == scratchpad[8] && !all_zeros && !all_ones) {
                int16_t rawTemp = (int16_t)((uint16_t)scratchpad[1] << 8 | scratchpad[0]);
                DS18B20_Temperature = (float)rawTemp / 16.0f;
                DS18B20_Valid = true;
                Serial.print(F("    [DS18B20] ¡Lectura exitosa! Temp = "));
                Serial.print(DS18B20_Temperature, 2);
                Serial.println(F(" °C"));
            } else {
                DS18B20_Valid = false;
                Serial.print(F("    [DS18B20] Fallo CRC o bus flotante. Scratchpad: "));
                for (uint8_t i = 0; i < 9; i++) {
                    if (scratchpad[i] < 0x10) Serial.print('0');
                    Serial.print(scratchpad[i], HEX);
                    Serial.print(' ');
                }
                Serial.print(F("| CRC calc: 0x"));
                Serial.println(crc, HEX);
            }
        } else {
            DS18B20_Valid = false;
            Serial.println(F("    [DS18B20] Error: Sin presencia al leer Scratchpad."));
        }
        dsState = DS18B20_STATE_IDLE;
        break;

    default:
        dsState = DS18B20_STATE_IDLE;
        break;
    }
}
