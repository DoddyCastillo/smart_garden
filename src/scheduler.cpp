#include "scheduler.h"

sTask SCH_tasks_G[SCH_MAX_TASKS];

void SCH_Init(void) {
    // Vaciar la tabla de tareas
    for (uint8_t i = 0; i < SCH_MAX_TASKS; i++) {
        SCH_tasks_G[i].pTask = 0;
        SCH_tasks_G[i].Delay = 0;
        SCH_tasks_G[i].Period = 0;
        SCH_tasks_G[i].RunMe = 0;
    }

    // Configurar Timer1 para 1ms tick
    noInterrupts();
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;
    OCR1A = 249;                         // (16 MHz) / (64 prescaler) / 1000 Hz - 1
    TCCR1B |= (1 << WGM12);              // CTC
    TCCR1B |= (1 << CS11) | (1 << CS10); // prescaler 64
    TIMSK1 |= (1 << OCIE1A);             // habilitar interrupción
    interrupts();
}

void SCH_Start(void) {
    // habilitar interrupciones globales
    interrupts();
}

uint8_t SCH_Add_Task(void (*pFunction)(), const uint16_t DELAY, const uint16_t PERIOD) {
    for (uint8_t i = 0; i < SCH_MAX_TASKS; i++) {
        if (SCH_tasks_G[i].pTask == 0) {
            SCH_tasks_G[i].pTask = pFunction;
            SCH_tasks_G[i].Delay = DELAY;
            SCH_tasks_G[i].Period = PERIOD;
            SCH_tasks_G[i].RunMe = 0;
            return i;
        }
    }
    return SCH_MAX_TASKS; // sin espacio
}

bool SCH_Delete_Task(const uint8_t index) {
    if (index >= SCH_MAX_TASKS)
        return false;
    SCH_tasks_G[index].pTask = 0;
    SCH_tasks_G[index].Delay = 0;
    SCH_tasks_G[index].Period = 0;
    SCH_tasks_G[index].RunMe = 0;
    return true;
}

void SCH_Dispatch_Tasks(void) {
    for (uint8_t i = 0; i < SCH_MAX_TASKS; i++) {
        if (SCH_tasks_G[i].RunMe > 0) {
            (*SCH_tasks_G[i].pTask)();
            SCH_tasks_G[i].RunMe--;
            if (SCH_tasks_G[i].Period == 0) {
                SCH_Delete_Task(i);
            }
        }
    }
}

// --- Timer1 ISR cada 1 ms ---
ISR(TIMER1_COMPA_vect) {
    SCH_Update();
}

void SCH_Update(void) {
    for (uint8_t i = 0; i < SCH_MAX_TASKS; i++) {
        if (SCH_tasks_G[i].pTask) {
            if (SCH_tasks_G[i].Delay == 0) {
                SCH_tasks_G[i].RunMe++;
                if (SCH_tasks_G[i].Period) {
                    SCH_tasks_G[i].Delay = SCH_tasks_G[i].Period;
                }
            } else {
                SCH_tasks_G[i].Delay--;
            }
        }
    }
}