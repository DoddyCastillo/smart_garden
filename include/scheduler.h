#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>

#define SCH_MAX_TASKS 5

typedef struct {
    void (*pTask)(void); // puntero a la tarea
    uint16_t Delay;      // tiempo hasta próxima ejecución (ticks)
    uint16_t Period;     // periodo (ticks)
    uint8_t RunMe;       // flag de ejecución
} sTask;

void SCH_Init(void);
void SCH_Start(void);
uint8_t SCH_Add_Task(void (*pFunction)(), const uint16_t DELAY, const uint16_t PERIOD);
void SCH_Dispatch_Tasks(void);
void SCH_Update(void);
bool SCH_Delete_Task(const uint8_t index);

#endif
