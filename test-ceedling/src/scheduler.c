#include "scheduler.h"
#include <stddef.h>

sTask SCH_tasks_G[SCH_MAX_TASKS];

static void SCH_Delete_Task(uint8_t i) {
    SCH_tasks_G[i].pTask = 0;
    SCH_tasks_G[i].Delay = 0;
    SCH_tasks_G[i].Period = 0;
    SCH_tasks_G[i].RunMe = 0;
}

static void SCH_Execute_Task(uint8_t i) {
    (*SCH_tasks_G[i].pTask)();
    SCH_tasks_G[i].RunMe--;
    if (SCH_tasks_G[i].Period == 0)
        SCH_Delete_Task(i);
}

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

void SCH_Dispatch_Tasks(void) {
    for (uint8_t i = 0; i < SCH_MAX_TASKS; i++) {
        while (SCH_tasks_G[i].RunMe > 0 && SCH_tasks_G[i].pTask) {
            SCH_Execute_Task(i);
        }
    }
}

void SCH_Update(void) {
    for (uint8_t i = 0; i < SCH_MAX_TASKS; i++) {
        sTask * task = &SCH_tasks_G[i];
        if (task->pTask == NULL)
            continue;
        if (task->Delay > 0) {
            task->Delay--;
            if (task->Delay == 0) {
                task->RunMe++;
                if (task->Period > 0)
                    task->Delay = task->Period;
            }
        } else if (task->Period > 0) {

            task->RunMe++;
            task->Delay = task->Period;
        }
    }
}
