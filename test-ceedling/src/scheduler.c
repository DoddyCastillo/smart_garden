#include "scheduler.h"

sTask SCH_tasks_G[SCH_MAX_TASKS];

void SCH_Init(void) {

    for (uint8_t i = 0; i < SCH_MAX_TASKS; i++) {
        SCH_tasks_G[i].pTask = 0;
        SCH_tasks_G[i].Delay = 0;
        SCH_tasks_G[i].Period = 0;
        SCH_tasks_G[i].RunMe = 0;
    }
}

uint8_t SCH_Add_Task(TaskFunction pFunction, uint16_t DELAY, uint16_t PERIOD) {
    for (uint8_t i = 0; i < SCH_MAX_TASKS; i++) {
        if (SCH_tasks_G[i].pTask == 0) {
            SCH_tasks_G[i].pTask = pFunction;
            SCH_tasks_G[i].Delay = DELAY;
            SCH_tasks_G[i].Period = PERIOD;
            SCH_tasks_G[i].RunMe = 0;
            return i;
        }
    }
    return SCH_NO_TASK_AVAILABLE; // sin espacio disponible
}

void SCH_Update(void) {
    for (uint8_t i = 0; i < SCH_MAX_TASKS; i++) {
        if (SCH_tasks_G[i].pTask) {
            if (SCH_tasks_G[i].Delay == 0) {
                // Si la tarea es periódica, la reprogramamos
                if (SCH_tasks_G[i].Period) {
                    SCH_tasks_G[i].Delay = SCH_tasks_G[i].Period;
                }
                // Indicamos que debe ejecutarse
                SCH_tasks_G[i].RunMe++;
            } else {
                // Todavía no llega a 0 → decrementamos delay
                SCH_tasks_G[i].Delay--;
            }
        }
    }
}