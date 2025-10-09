/**
 * @file scheduler.h
 * @brief Definición de estructuras y funciones del Scheduler Cooperativo.
 *
 * Este archivo contiene la definición de la estructura de tareas, constantes,
 * y prototipos de funciones que permiten la planificación cooperativa de tareas
 * en sistemas embebidos basados en microcontroladores AVR (por ejemplo, ATmega32U4).
 *
 * Cada tarea se define mediante un puntero a función, un retardo inicial y un
 * periodo de repetición (en milisegundos o ticks del temporizador).
 *
 * @version 1.0
 * @date 2025-10-09
 * @author
 * Doddy Castillo Caicedo
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>

/**
 * @def SCH_MAX_TASKS
 * @brief Número máximo de tareas que puede manejar el scheduler simultáneamente.
 */
#define SCH_MAX_TASKS 5

/**
 * @def SCH_NO_TASK_AVAILABLE
 * @brief Código de error devuelto cuando no hay espacio disponible para agregar una nueva tarea.
 */
#define SCH_NO_TASK_AVAILABLE 0

/**
 * @struct sTask
 * @brief Estructura que representa una tarea programada dentro del scheduler.
 *
 * Contiene los datos necesarios para controlar la ejecución de cada tarea:
 * - Puntero a la función de la tarea.
 * - Retardo inicial (`Delay`).
 * - Periodo de repetición (`Period`).
 * - Bandera de ejecución (`RunMe`).
 */
typedef struct {
    void (*pTask)(void); /**< Puntero a la función que representa la tarea */
    uint16_t Delay;      /**< Tiempo restante hasta la próxima ejecución (en ticks) */
    uint16_t Period; /**< Periodo de repetición de la tarea (en ticks). Si es 0, se ejecuta una sola
                        vez */
    uint8_t RunMe;   /**< Contador de ejecuciones pendientes (incrementado por el temporizador) */
} sTask;

/**
 * @brief Arreglo global de tareas gestionadas por el scheduler.
 */
extern sTask SCH_tasks_G[SCH_MAX_TASKS];

/**
 * @brief Inicializa el scheduler y configura el Timer1 con tick de 1 ms.
 *
 * Esta función limpia la tabla de tareas y configura el temporizador para
 * generar interrupciones periódicas de 1 ms.
 */
void SCH_Init(void);

/**
 * @brief Habilita las interrupciones globales y arranca el scheduler.
 */
void SCH_Start(void);

/**
 * @brief Agrega una nueva tarea al scheduler.
 *
 * @param pFunction Puntero a la función de la tarea a ejecutar.
 * @param DELAY Retardo inicial (en milisegundos/ticks) antes de la primera ejecución.
 * @param PERIOD Periodo de repetición (en milisegundos/ticks). Si es 0, la tarea es única.
 * @return uint8_t Índice de la tarea agregada o `SCH_NO_TASK_AVAILABLE` si no hay espacio.
 */
uint8_t SCH_Add_Task(void (*pFunction)(), const uint16_t DELAY, const uint16_t PERIOD);

/**
 * @brief Ejecuta las tareas cuyo contador `RunMe` sea mayor que cero.
 *
 * Esta función debe llamarse de forma continua dentro del bucle principal (`loop`).
 */
void SCH_Dispatch_Tasks(void);

/**
 * @brief Actualiza los contadores de las tareas activas en cada tick del temporizador.
 *
 * Esta función es llamada automáticamente por la ISR del Timer1 (1 ms).
 */
void SCH_Update(void);

#endif /* SCHEDULER_H */
