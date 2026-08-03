#ifndef MHZ19_H
#define MHZ19_H

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Pin digital asignado a la señal PWM del sensor MH-Z19C.
 * Configurado en el Pin 9 (marcado con ~ PWM en la placa Pro Micro) usando PCINT5.
 */
#ifndef MHZ19_PWM_PIN
#define MHZ19_PWM_PIN 9
#endif

/**
 * @brief Rango máximo de detección del sensor (5000.0 ppm).
 */
#ifndef MHZ19_RANGE_PPM
#define MHZ19_RANGE_PPM 5000.0f
#endif

/**
 * @brief Concentración de CO2 calculada y filtrada en ppm (partes por millón).
 */
extern float MHZ19_CO2;

/**
 * @brief Estado de validez de la señal PWM capturada.
 */
extern bool MHZ19_Valid;

/**
 * @brief Inicializa el Pin 9 (~PWM) y configura la interrupción por cambio de pin (PCINT5).
 */
void MHZ19_Init(void);

/**
 * @brief Función periódica no bloqueante para procesar las mediciones de CO2.
 */
void MHZ19_Tick(void);

#ifdef __cplusplus
}
#endif

#endif /* MHZ19_H */
