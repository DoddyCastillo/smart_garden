#include "MHZ19.h"

float MHZ19_CO2 = 400.0f;
bool MHZ19_Valid = false;

static volatile uint32_t ris_time_us = 0;
static volatile uint32_t high_duration_us = 0;
static volatile uint32_t cycle_duration_us = 0;
static volatile bool new_pulse_flag = false;

#define MHZ19_WINDOW 8
static float co2_window[MHZ19_WINDOW];
static uint8_t co2_idx = 0;
static uint8_t co2_count = 0;

// ISR de Interrupción Hardware por Cambio de Pin (PCINT0) para el Pin 9 (~PWM / PB5)
ISR(PCINT0_vect) {
    uint32_t now = micros();
    // Leer directamente el estado del registro físico del Pin 9 (PB5)
    bool pin_state = (PINB & (1 << PB5)) != 0;

    if (pin_state) {
        if (ris_time_us > 0) {
            cycle_duration_us = now - ris_time_us;
        }
        ris_time_us = now;
    } else {
        if (ris_time_us > 0) {
            high_duration_us = now - ris_time_us;
            new_pulse_flag = true;
        }
    }
}

void MHZ19_Init(void) {
    MHZ19_CO2 = 400.0f;
    MHZ19_Valid = false;
    ris_time_us = 0;
    high_duration_us = 0;
    cycle_duration_us = 0;
    new_pulse_flag = false;
    co2_idx = 0;
    co2_count = 0;

    for (uint8_t i = 0; i < MHZ19_WINDOW; i++) {
        co2_window[i] = 400.0f;
    }

    pinMode(MHZ19_PWM_PIN, INPUT_PULLUP);

    // Habilitar la interrupción de hardware PCINT5 para el Pin 9 (PB5)
    noInterrupts();
    PCICR |= (1 << PCIE0);     // Habilitar grupo PCINT0 (PB0-PB7)
    PCMSK0 |= (1 << PCINT5);   // Habilitar máscara para PB5 (Pin 9)
    interrupts();
}

void MHZ19_Tick(void) {
    // Salvaguarda de tiempo de espera: Si no se detectan flancos durante más de 2.5 segundos, invalida la señal
    if (ris_time_us > 0 && (micros() - ris_time_us > 2500000UL)) {
        MHZ19_Valid = false;
    }

    if (new_pulse_flag) {
        noInterrupts();
        uint32_t th_us = high_duration_us;
        uint32_t tc_us = cycle_duration_us;
        new_pulse_flag = false;
        interrupts();

        float th_ms = th_us / 1000.0f;
        float tc_ms = (tc_us > 0) ? (tc_us / 1000.0f) : 1004.0f;

        // Validar duraciones físicas del sensor (1004ms periodo)
        if (th_ms >= 2.0f && th_ms <= 1002.0f && tc_ms >= 900.0f && tc_ms <= 1100.0f) {
            float raw_ppm = MHZ19_RANGE_PPM * (th_ms - 2.0f) / (tc_ms - 4.0f);

            if (raw_ppm < 400.0f) raw_ppm = 400.0f;
            if (raw_ppm > MHZ19_RANGE_PPM) raw_ppm = MHZ19_RANGE_PPM;

            co2_window[co2_idx] = raw_ppm;
            co2_idx = (co2_idx + 1) % MHZ19_WINDOW;
            if (co2_count < MHZ19_WINDOW) co2_count++;

            float sum = 0.0f;
            for (uint8_t i = 0; i < co2_count; i++) {
                sum += co2_window[i];
            }

            MHZ19_CO2 = sum / (float)co2_count;
            MHZ19_Valid = true;
        }
    }
}
