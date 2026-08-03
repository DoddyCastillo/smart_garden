/************************************************************************************************
Copyright (c) 2025, Doddy Castillo Caicedo

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*************************************************************************************************/

/** @file tasks.h
 *  @brief Tareas cooperativas de la aplicación (LED, SHT31, Serial debug).
 *
 *  Este módulo declara las tareas que serán registradas en el scheduler
 *  cooperativo. La lógica de medición del SHT31 se encuentra aislada en
 *  el módulo @ref sht31, y aquí sólo se invocan sus funciones periódicas
 *  desde la tarea correspondiente.
 */

/** @addtogroup app_tasks
 *  @{
 */

#ifndef TASKS_H
#define TASKS_H

/* === Headers files inclusions ================================================================ */

#include <stdint.h>

/* === Cabecera C++ ============================================================================ */

#ifdef __cplusplus
extern "C" {
#endif

/* === Public macros definitions =============================================================== */

/**
 * @brief Pin asociado al LED de usuario.
 *
 * Ajusta este valor según el hardware (por ejemplo, Pro Micro).
 */
#define LED_PIN 17

/* === Public data type declarations =========================================================== */

/* (No se requieren tipos públicos adicionales por el momento) */

/* === Public variable declarations ============================================================ */

/* (Las variables de medición se exponen desde sht31.h) */

/* === Public function declarations ============================================================ */

/**
 * @brief Tarea cooperativa que conmuta el LED de estado.
 */
void LED_Task(void);

/**
 * @brief Tarea de respaldo segura en caso de que LED_Task se desborde.
 */
void LED_Backup_Task(void);

/**
 * @brief Tarea cooperativa que delega en @ref SHT31_Tick().
 *
 * Debe registrarse en el scheduler con el mismo periodo definido en
 * @ref SHT31_TASK_TICK_MS.
 */
void SHT31_Task(void);

/**
 * @brief Tarea cooperativa de depuración por puerto serie.
 *
 * Muestra los valores filtrados de T y H, CO2 y el estado de @ref SHT31_Status.
 */
void Serial_Task(void);

/**
 * @brief Tarea cooperativa que delega en @ref MHZ19_Tick().
 */
void MHZ19_Task(void);

/**
 * @brief Tarea cooperativa que delega en @ref DS18B20_Tick().
 */
void DS18B20_Task(void);

/* === End of documentation ==================================================================== */

#ifdef __cplusplus
}
#endif

/** @} End of module definition for doxygen */

#endif /* TASKS_H */
