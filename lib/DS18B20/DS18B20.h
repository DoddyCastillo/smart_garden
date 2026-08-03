#ifndef DS18B20_H
#define DS18B20_H

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Pin digital asignado a la línea de datos 1-Wire (DQ) del sensor DS18B20.
 * Configurado por defecto en el Pin 4 (requiere resistencia pull-up de 4.7k a VDD).
 */
#ifndef DS18B20_PIN
#define DS18B20_PIN 4
#endif

/**
 * @brief Periodo (en ms) con el que el planificador invoca a DS18B20_Tick().
 */
#define DS18B20_TASK_TICK_MS 10U

/**
 * @brief Periodo de muestreo entre lecturas de temperatura (2000 ms = 2 segundos).
 */
#define DS18B20_PERIOD_MS 2000U

/**
 * @brief Tiempo de conversión de resolución de 12 bits en el DS18B20 (750 ms).
 */
#define DS18B20_CONV_TIME_MS 750U

/**
 * @brief Última temperatura medida en °C.
 */
extern float DS18B20_Temperature;

/**
 * @brief Estado de validez de la última lectura (CRC correcto y presencia hallada).
 */
extern bool DS18B20_Valid;

/**
 * @brief Inicializa el pin 1-Wire y las variables internas del módulo.
 */
void DS18B20_Init(void);

/**
 * @brief Función periódica no bloqueante para gestionar la máquina de estados 1-Wire.
 */
void DS18B20_Tick(void);

#ifdef __cplusplus
}
#endif

#endif /* DS18B20_H */
