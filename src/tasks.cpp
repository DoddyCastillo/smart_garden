#include <Arduino.h>
#include "tasks.h"
#include "SHT31.h"
#include "MHZ19.h"
#include "DS18B20.h"

/* === LED TASK ============================================================================== */

void LED_Task(void) {
    static bool state = false;
    state = !state;

    // Controlar LEDs integrados de placa Pro Micro (RX y TX LEDs)
    if (state) {
        RXLED1; // Encender LED RX
        TXLED1; // Encender LED TX
    } else {
        RXLED0; // Apagar LED RX
        TXLED0; // Apagar LED TX
    }
}

void LED_Backup_Task(void) {
    // Tarea de respaldo segura: mantiene los LEDs apagados
    RXLED0;
    TXLED0;
}

/* === SHT31 TASK ============================================================================ */

void SHT31_Task(void) {
    /* Delegar al módulo sht31 */
    SHT31_Tick();
}

/* === MH-Z19C TASK ========================================================================== */

void MHZ19_Task(void) {
    /* Delegar al módulo mhz19 (procesar señal PWM) */
    MHZ19_Tick();
}

/* === DS18B20 TASK ========================================================================== */

void DS18B20_Task(void) {
    /* Delegar al módulo ds18b20 */
    DS18B20_Tick();
}

/* === SERIAL TASK =========================================================================== */

void Serial_Task(void) {
    Serial.print(F("Temp(SHT31): "));
    Serial.print(SHT31_Temperature, 2);
    Serial.print(F(" °C | Hum: "));
    Serial.print(SHT31_Humidity, 2);
    Serial.print(F(" % | CO2: "));
    if (MHZ19_Valid) {
        Serial.print(MHZ19_CO2, 1);
        Serial.print(F(" ppm"));
    } else {
        Serial.print(F("Calibrando..."));
    }
    Serial.print(F(" | Temp(DS18B20): "));
    if (DS18B20_Valid) {
        Serial.print(DS18B20_Temperature, 2);
        Serial.println(F(" °C"));
    } else {
        Serial.println(F("Leyendo..."));
    }
}
