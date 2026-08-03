/**
 * @file scheduler.h
 * @brief Definición de estructuras y funciones del Scheduler Cooperativo Adaptativo (TTC-Adaptive).
 *
 * Este archivo contiene la interfaz para el planificador adaptativo con soporte para:
 * - Medición de WCET en tiempo real.
 * - Liberación de precisión mediante MTI (Timer1 Match A).
 * - Guardián de tareas (Task Guardian via Timer1 Match B).
 * - Aborto seguro de tareas desbordadas mediante setjmp/longjmp.
 * - Tareas de respaldo (Backup Tasks).
 *
 * @version 2.0 (Adaptive)
 * @date 2026-08-02
 */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>
#include <setjmp.h>

/**
 * @def SCH_MAX_TASKS
 * @brief Número máximo de tareas que puede manejar el scheduler simultáneamente.
 */
#define SCH_MAX_TASKS 5

/**
 * @def SCH_NO_TASK_AVAILABLE
 * @brief Código de error devuelto cuando no hay espacio disponible para agregar una nueva tarea.
 */
#define SCH_NO_TASK_AVAILABLE 0xFF

/**
 * @def SCH_CALCULATION_TIME
 * @brief Tiempo en milisegundos para la fase de calibración (Calculating Mode).
 */
#define SCH_CALCULATION_TIME 1000

/**
 * @enum SCH_Mode_t
 * @brief Modos de operación del planificador adaptativo.
 */
typedef enum {
    CALCULATING_MODE, /**< Fase de calibración inicial (aprende WCET y offsets) */
    OPERATING_MODE    /**< Fase operativa con Task Guardian activo */
} SCH_Mode_t;

/**
 * @struct sTask
 * @brief Estructura que representa una tarea dentro del planificador adaptativo.
 */
typedef struct {
    void (*pTask)(void);   /**< Puntero a la función principal de la tarea */
    void (*bTask)(void);   /**< Puntero a la función de respaldo en caso de fallo */
    uint16_t Delay;        /**< Tiempo restante hasta la próxima ejecución (en ticks) */
    uint16_t Period;       /**< Periodo de repetición de la tarea (en ticks) */
    uint8_t RunMe;         /**< Contador de ejecuciones pendientes */
    uint16_t WCET;         /**< Duración máxima medida (en ticks de 0.5 us) */
    uint16_t Req_Rls_Tm;   /**< Desfase de liberación requerido (en ticks de 0.5 us) */
    uint16_t Overruns;     /**< Contador de sobrecargas/fallos detectados */
} sTask;

/**
 * @brief Arreglo global de tareas gestionadas por el scheduler.
 */
extern sTask SCH_tasks_G[SCH_MAX_TASKS];

/**
 * @brief Contexto de restauración de pila para el aborto seguro de tareas (Task Guardian).
 */
extern jmp_buf SCH_jmp_env;

/**
 * @brief Inicializa el planificador adaptativo y configura el Timer1 en Modo 12 CTC.
 */
void SCH_Init(void);

/**
 * @brief Habilita las interrupciones globales y arranca el planificador.
 */
void SCH_Start(void);

/**
 * @brief Agrega una tarea con soporte opcional de función de respaldo (Backup Task).
 *
 * @param pFunction Puntero a la función principal de la tarea.
 * @param pBackup Puntero a la función de respaldo (puede ser nullptr si no requiere).
 * @param DELAY Retardo inicial (en milisegundos/ticks).
 * @param PERIOD Periodo de repetición (en milisegundos/ticks).
 * @return uint8_t Índice de la tarea o `SCH_NO_TASK_AVAILABLE` si no hay espacio.
 */
uint8_t SCH_Add_Task(void (*pFunction)(), void (*pBackup)(), const uint16_t DELAY, const uint16_t PERIOD);

/**
 * @brief Sobrecarga conveniente de SCH_Add_Task para tareas tradicionales sin función de respaldo.
 */
inline uint8_t SCH_Add_Task(void (*pFunction)(), const uint16_t DELAY, const uint16_t PERIOD) {
    return SCH_Add_Task(pFunction, nullptr, DELAY, PERIOD);
}

/**
 * @brief Bucle del despachador y punto de retorno para recuperación de sobrecargas.
 */
void SCH_Dispatch_Tasks(void);

/**
 * @brief Función de actualización vacía mantenida por compatibilidad de interfaz.
 */
void SCH_Update(void);

#endif /* SCHEDULER_H */
