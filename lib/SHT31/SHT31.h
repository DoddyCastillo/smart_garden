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

#ifndef TEMPLATE_H
#define TEMPLATE_H

/** @file sht31.h
 *  @brief Tareas cooperativas de la aplicación (LED, SHT31, Serial debug).
 *
 *  Este módulo declara las tareas que serán registradas en el scheduler
 *  cooperativo, así como variables globales de estado asociadas a la lectura
 *  del sensor SHT31 (temperatura y humedad filtradas) y códigos de error.
 *
 *  @author Doddy Castillo Caicedo
 *  @date   2025-11-16
 */

/** @addtogroup sht31_module SHT31 Sensor Module
 *  @{
 */

/* === Headers files inclusions ================================================================ */
#include <stdint.h>

/* === Cabecera C++ ============================================================================ */

#ifdef __cplusplus
extern "C" {
#endif

/* === Macros definitions ===================================================================== */

/**
 * @brief Dirección I2C del sensor SHT31 (7 bits).
 */
#define SHT31_ADDR 0x44

/**
 * @brief Periodo (en ms) con el que se debe llamar a @ref SHT31_Tick().
 *
 * Debe coincidir con el periodo de la tarea del scheduler que invoque
 * a @ref SHT31_Tick(). Si registras la tarea cada 10 ms, define 10 aquí.
 */
#define SHT31_TASK_TICK_MS 10U

/**
 * @brief Periodo de muestreo del sensor (en ms).
 *
 * Define cada cuánto tiempo se inicia una nueva medición en el SHT31.
 * Por ejemplo:
 *  - 1000 ms → 1 Hz
 *  - 250 ms  → 4 Hz
 *  - 5000 ms → 0.2 Hz
 */
#define SHT31_PERIOD_MS 1000U

/**
 * @brief Tiempo de conversión del SHT31 (en ms).
 *
 * El datasheet indica un tiempo típico de ~15 ms para alta resolución.
 * Se recomienda dejar un margen (ej. 20 ms).
 */
#define SHT31_CONV_TIME_MS 20U

/**
 * @brief Tamaño de la ventana de la media móvil para T y H.
 */
#define SHT31_MA_WINDOW 8U

/* === Public data type declarations =========================================================== */

typedef enum {
    SHT31_STATUS_OK = 0,        /**< Última lectura correcta              */
    SHT31_STATUS_I2C_WRITE = 1, /**< Error al enviar comando I2C          */
    SHT31_STATUS_I2C_READ = 2,  /**< Error al leer datos I2C              */
    SHT31_STATUS_CRC = 3,       /**< Error de CRC en T o H                */
    SHT31_STATUS_WAITING = 4,   /**< Esperando tiempo de conversión       */
    SHT31_STATUS_BUS_BUSY = 5   /**< Bus I2C ocupado al intentar operar   */
} SHT31_Status_t;

/* === Public variable declarations ============================================================ */

/**
 * @brief Última temperatura medida (filtrada) en °C.
 */
extern float SHT31_Temperature;

/**
 * @brief Última humedad relativa medida (filtrada) en %.
 */
extern float SHT31_Humidity;

/**
 * @brief Código de estado/errores actual del módulo SHT31.
 *
 * Véase @ref SHT31_Status_t.
 */
extern uint8_t SHT31_Status;

/**
 * @brief Código de estado/errores del módulo SHT31/I2C.
 *
 * - 0 = OK
 * - 1 = Error al enviar comando I2C (write)
 * - 2 = Error al leer datos I2C (read)
 * - 3 = Error de CRC
 * - 4 = Esperando conversión ("waiting")
 * - 5 = Bus I2C ocupado ("bus busy")
 */
extern uint8_t Error_code_G;

/* === Public function declarations ============================================================ */

/**
 * @brief Inicializa las estructuras internas del módulo SHT31.
 *
 * No inicializa el bus I2C (eso se hace en el driver I2C asíncrono).
 * Resetea la ventana de media móvil y los códigos de estado.
 */
void SHT31_Init(void);

/**
 * @brief Función periódica (no bloqueante) para gestionar el SHT31.
 *
 * Debe llamarse cada @ref SHT31_TASK_TICK_MS milisegundos. Internamente:
 *   - Inicia nuevas mediciones con la frecuencia definida por
 *     @ref SHT31_PERIOD_MS.
 *   - Gestiona la máquina de estados para el envío de comandos y lectura
 *     de datos usando el driver I2C asíncrono.
 *   - Actualiza @ref SHT31_Temperature y @ref SHT31_Humidity aplicando
 *     un filtro de media móvil.
 *   - Actualiza @ref SHT31_Status con el estado/errores actuales.
 */
void SHT31_Tick(void);

/**
 * @brief Devuelve una cadena de texto descriptiva para el estado actual.
 *
 * @return Puntero a una cadena estática que describe @ref SHT31_Status.
 */
const char * SHT31_GetStatusText(void);

/* === End of documentation ==================================================================== */

#ifdef __cplusplus
}
#endif

/** @} End of module definition for doxygen */

#endif /* TEMPLATE_H */