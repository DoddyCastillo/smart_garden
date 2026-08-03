/**
 * @file scheduler.cpp
 * @brief Implementación del Scheduler Cooperativo Adaptativo (TTC-Adaptive) basado en Timer1 (16 MHz / 8 prescaler)
 * para el microcontrolador ATmega32U4.
 */

#include "scheduler.h"
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

sTask SCH_tasks_G[SCH_MAX_TASKS];
jmp_buf SCH_jmp_env;

void SCH_Init(void) {
    for (uint8_t i = 0; i < SCH_MAX_TASKS; i++) {
        SCH_tasks_G[i].pTask = nullptr;
        SCH_tasks_G[i].bTask = nullptr;
        SCH_tasks_G[i].Delay = 0;
        SCH_tasks_G[i].Period = 0;
        SCH_tasks_G[i].RunMe = 0;
        SCH_tasks_G[i].WCET = 0;
        SCH_tasks_G[i].Req_Rls_Tm = 0;
        SCH_tasks_G[i].Overruns = 0;
    }

    noInterrupts();

    // Forzar reloj del CPU a 16 MHz limpios
    CLKPR = (1 << CLKPCE);
    CLKPR = 0;

    // Configurar Timer1 en Modo 12 CTC usando ICR1 como TOP
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    // TOP = 1999 para tick de 1 ms (16 MHz / 8 / 1000 Hz - 1)
    ICR1 = 1999;

    TCCR1B |= (1 << WGM13) | (1 << WGM12); // Modo 12 CTC
    TIMSK1 |= (1 << ICIE1);               // Interrupción de captura (Tick Update de 1ms)

    // Iniciar temporizador con Prescaler = 8
    TCCR1B |= (1 << CS11);

    // Configurar Watchdog de 250ms
    wdt_enable(WDTO_250MS);

    interrupts();
}

void SCH_Start(void) {
    interrupts();
}

uint8_t SCH_Add_Task(void (*pFunction)(), void (*pBackup)(), const uint16_t DELAY, const uint16_t PERIOD) {
    for (uint8_t i = 0; i < SCH_MAX_TASKS; i++) {
        if (SCH_tasks_G[i].pTask == nullptr) {
            noInterrupts();
            SCH_tasks_G[i].pTask = pFunction;
            SCH_tasks_G[i].bTask = pBackup;
            SCH_tasks_G[i].Delay = DELAY;
            SCH_tasks_G[i].Period = PERIOD;
            SCH_tasks_G[i].RunMe = 0;
            SCH_tasks_G[i].WCET = 0;
            SCH_tasks_G[i].Req_Rls_Tm = 0;
            SCH_tasks_G[i].Overruns = 0;
            interrupts();
            return i;
        }
    }
    return SCH_NO_TASK_AVAILABLE;
}

// ISR del Ciclo General (Tick Update de 1ms de precisión por Hardware)
ISR(TIMER1_CAPT_vect) {
    for (uint8_t Index = 0; Index < SCH_MAX_TASKS; Index++) {
        if (SCH_tasks_G[Index].pTask) {
            if (SCH_tasks_G[Index].Delay == 0) {
                SCH_tasks_G[Index].RunMe++;
                if (SCH_tasks_G[Index].Period != 0) {
                    SCH_tasks_G[Index].Delay = SCH_tasks_G[Index].Period - 1;
                }
            } else {
                SCH_tasks_G[Index].Delay--;
            }
        }
    }
}

// Despachador principal en Contexto de Usuario (no bloqueante)
void SCH_Dispatch_Tasks(void) {
    wdt_reset();

    for (uint8_t Index = 0; Index < SCH_MAX_TASKS; Index++) {
        if (SCH_tasks_G[Index].RunMe > 0) {
            noInterrupts();
            SCH_tasks_G[Index].RunMe--;
            interrupts();

            if (SCH_tasks_G[Index].pTask) {
                (*SCH_tasks_G[Index].pTask)();
            }
        }
    }

    // Entrar en modo de bajo consumo hasta la siguiente interrupción
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_enable();
    sleep_cpu();
    sleep_disable();
}

void SCH_Update(void) {
    // Mantención por compatibilidad
}