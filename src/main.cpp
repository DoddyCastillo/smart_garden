#include <Arduino.h>
#include <Wire.h>
#include "scheduler.h"
#include "tasks.h"

void setup() {
    Serial.begin(9600);
    Wire.begin();
    pinMode(17, OUTPUT);

    SCH_Init();

    // LED cada 200ms
    SCH_Add_Task(LED_Task, 0, 200);

    // Sensor SHT31 cada 1000ms
    SCH_Add_Task(SHT31_Task, 0, 1); // máquina de estados interna cada tick

    // Serial cada 1000ms
    SCH_Add_Task(Serial_Task, 1000, 1000);

    SCH_Start();
}

void loop() {
    SCH_Dispatch_Tasks();
}