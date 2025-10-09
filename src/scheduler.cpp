/**
 * @file scheduler.cpp
 * @brief Implementación del Scheduler Cooperativo basado en Timer1 (tick de 1 ms) para
 * microcontroladores AVR.
 *
 * Este módulo permite programar y ejecutar tareas periódicas o únicas en sistemas embebidos
 * mediante una estructura de planificación cooperativa. Cada tarea registrada es verificada
 * en cada interrupción del temporizador y ejecutada cuando corresponde.
 *
 * @version 1.0
 * @date 2025-10-09
 * @author
 * Doddy Castillo Caicedo
 */

#include "scheduler.h"

sTask SCH_tasks_G[SCH_MAX_TASKS];

/**
 * @brief Elimina una tarea del scheduler en el índice indicado.
 *
 * Esta función limpia los valores del slot de tarea, dejándolo disponible
 * para nuevas asignaciones.
 *
 * @param i Índice de la tarea a eliminar.
 */
static void SCH_Delete_Task(uint8_t i) {
    SCH_tasks_G[i].pTask = 0;
    SCH_tasks_G[i].Delay = 0;
    SCH_tasks_G[i].Period = 0;
    SCH_tasks_G[i].RunMe = 0;
}

/**
 * @brief Ejecuta una tarea programada y actualiza su estado.
 *
 * Esta función llama al puntero de función asociado, decrementa el contador `RunMe`
 * y, si la tarea es de ejecución única (Period = 0), la elimina automáticamente.
 *
 * @param i Índice de la tarea a ejecutar.
 */
static void SCH_Execute_Task(uint8_t i) {
    (*SCH_tasks_G[i].pTask)();
    SCH_tasks_G[i].RunMe--;
    if (SCH_tasks_G[i].Period == 0)
        SCH_Delete_Task(i);
}

/**
 * @brief Inicializa el scheduler y configura Timer1 para generar ticks de 1 ms.
 *
 * Esta función limpia la tabla de tareas y configura el temporizador 1 en modo CTC.
 * La interrupción asociada incrementará los contadores de las tareas según su retardo o periodo.
 */
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

/**
 * @brief Habilita las interrupciones globales para comenzar la ejecución del scheduler.
 */
void SCH_Start(void) {
    // habilitar interrupciones globales
    interrupts();
}

/**
 * @brief Agrega una tarea al scheduler.
 *
 * La tarea será almacenada en el primer espacio disponible en la tabla `SCH_tasks_G`.
 *
 * @param pFunction Puntero a la función de la tarea.
 * @param DELAY Retardo inicial en milisegundos antes de la primera ejecución.
 * @param PERIOD Periodo de repetición en milisegundos. Si es 0, la tarea se ejecuta solo una vez.
 * @return uint8_t Índice asignado a la tarea o `SCH_NO_TASK_AVAILABLE` si no hay espacio.
 */
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
    return SCH_NO_TASK_AVAILABLE; // sin espacio disponible
}

/**
 * @brief Ejecuta todas las tareas que tienen `RunMe > 0`.
 *
 * Cada tarea se ejecuta en orden y, si su periodo es 0, se elimina tras la ejecución.
 * Esta función se debe llamar continuamente dentro del bucle principal (`loop`).
 */
void SCH_Dispatch_Tasks(void) {
    for (uint8_t i = 0; i < SCH_MAX_TASKS; i++) {
        while (SCH_tasks_G[i].RunMe > 0 && SCH_tasks_G[i].pTask) {
            SCH_Execute_Task(i);
        }
    }
}

/**
 * @brief Rutina de servicio de interrupción del Timer1 cada 1 ms.
 *
 * Esta ISR se ejecuta automáticamente y llama a `SCH_Update()` para actualizar los contadores de
 * tareas.
 */
ISR(TIMER1_COMPA_vect) {
    SCH_Update();
}

/**
 * @brief Actualiza el estado de las tareas en función del tick de 1 ms.
 *
 * Esta función decrementa el `Delay` de cada tarea activa. Cuando `Delay` llega a 0,
 * incrementa el contador `RunMe` y reinicia el retardo según el periodo configurado.
 */
void SCH_Update(void) {
    for (uint8_t i = 0; i < SCH_MAX_TASKS; i++) {
        if (SCH_tasks_G[i].pTask) {
            if (SCH_tasks_G[i].Delay > 0) {
                SCH_tasks_G[i].Delay--;
                if (SCH_tasks_G[i].Delay == 0) {
                    SCH_tasks_G[i].RunMe++;
                    if (SCH_tasks_G[i].Period)
                        SCH_tasks_G[i].Delay = SCH_tasks_G[i].Period;
                }
            } else if (SCH_tasks_G[i].Period) {
                SCH_tasks_G[i].RunMe++;
                SCH_tasks_G[i].Delay = SCH_tasks_G[i].Period;
            }
        }
    }
}