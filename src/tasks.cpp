#include "tasks.h"

// Variables globales de estado
float latestTemperature = 0.0;
float latestHumidity = 0.0;
uint8_t Error_code_G = 0;

enum SHT31_State { SHT31_IDLE, SHT31_WAITING, SHT31_READ };

SHT31_State shtState = SHT31_IDLE;
uint16_t shtWaitCounter = 0;

const int LED_PIN = 17; // LED externo en pin 10

void LED_Task(void) {
    static bool state = false;
    digitalWrite(LED_PIN, state ? HIGH : LOW);
    state = !state;
}

// Máquina de estados NO bloqueante para SHT31
void SHT31_Task(void) {
    switch (shtState) {
    case SHT31_IDLE:
        // Enviar comando de medición
        Wire.beginTransmission(SHT31_ADDR);
        Wire.write(0x24);
        Wire.write(0x00);
        Wire.endTransmission();
        shtWaitCounter = 15; // esperar 15ms típicamente
        shtState = SHT31_WAITING;
        break;

    case SHT31_WAITING:
        if (shtWaitCounter > 0) {
            shtWaitCounter--;
        } else {
            shtState = SHT31_READ;
        }
        break;

    case SHT31_READ:
        Wire.requestFrom((uint8_t)SHT31_ADDR, (uint8_t)6);
        if (Wire.available() == 6) {
            uint16_t tData = (Wire.read() << 8) | Wire.read();
            Wire.read(); // CRC temp
            uint16_t hData = (Wire.read() << 8) | Wire.read();
            Wire.read(); // CRC hum

            latestTemperature = -45 + 175 * (tData / 65535.0);
            latestHumidity = 100 * (hData / 65535.0);
            Error_code_G = 0;
        } else {
            Error_code_G = 3; // error de lectura
        }
        shtState = SHT31_IDLE;
        break;
    }
}

void Serial_Task(void) {
    Serial.print("T=");
    Serial.print(latestTemperature, 1);
    Serial.print(" °C, H=");
    Serial.print(latestHumidity, 1);
    Serial.print(" % | Err=");
    Serial.println(Error_code_G);
}