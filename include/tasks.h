#ifndef TASKS_H
#define TASKS_H

#include <Arduino.h>
#include <Wire.h>

// SHT31 dirección
#define SHT31_ADDR 0x44

void LED_Task(void);
void SHT31_Task(void);
void Serial_Task(void);

extern float latestTemperature;
extern float latestHumidity;
extern uint8_t Error_code_G;

#endif
