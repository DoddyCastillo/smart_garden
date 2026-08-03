#include <Arduino.h>
#include "i2c_async.h"
#include "scheduler.h"
#include "tasks.h"
#include "SHT31.h"
#include "MHZ19.h"
#include "DS18B20.h"

void setup() {
    Serial.begin(9600);

    /* Inicializar I2C asíncrono */
    I2C_Init();

    /* Inicializar módulo SHT31 */
    SHT31_Init();

    /* Inicializar sensor CO2 MH-Z19C */
    MHZ19_Init();

    /* Inicializar sensor de temperatura DS18B20 */
    DS18B20_Init();

    /* Inicializar scheduler */
    SCH_Init();

    /* LED cada 1000 ms (1 segundo) con tarea de respaldo LED_Backup_Task */
    SCH_Add_Task(LED_Task, LED_Backup_Task, 0, 1000);

    /* SHT31_Tick cada SHT31_TASK_TICK_MS (10 ms) */
    SCH_Add_Task(SHT31_Task, 0, SHT31_TASK_TICK_MS);

    /* MHZ19_Tick cada 5 ms */
    SCH_Add_Task(MHZ19_Task, 0, 5);

    /* DS18B20_Tick cada DS18B20_TASK_TICK_MS (10 ms) */
    SCH_Add_Task(DS18B20_Task, 0, DS18B20_TASK_TICK_MS);

    /* Serial debug cada 1000 ms */
    SCH_Add_Task(Serial_Task, 1000, 1000);

    SCH_Start();
}

void loop() {
    SCH_Dispatch_Tasks();
}