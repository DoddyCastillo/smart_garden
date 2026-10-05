#ifndef RYLR998_H
#define RYLR998_H

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Dirección ID única para este nodo sensor emisor.
 */
#ifndef RYLR998_NODE_ADDRESS
#define RYLR998_NODE_ADDRESS 2
#endif

/**
 * @brief Dirección ID del Gateway receptor (en gateway_garden es 1).
 */
#ifndef RYLR998_GATEWAY_ADDRESS
#define RYLR998_GATEWAY_ADDRESS 1
#endif

/**
 * @brief ID de Red LoRa compartido (en gateway_garden es 6).
 */
#ifndef RYLR998_NETWORK_ID
#define RYLR998_NETWORK_ID 6
#endif

/**
 * @brief Frecuencia de trabajo en Hz (915 MHz para coincidir con el Gateway).
 */
#ifndef RYLR998_FREQUENCY_HZ
#define RYLR998_FREQUENCY_HZ 915000000UL
#endif

/**
 * @brief Velocidad en baudios del puerto serie del RYLR998.
 */
#define RYLR998_BAUD_RATE 115200

/**
 * @brief Periodo con el que el scheduler invoca a RYLR998_Tick() en ms (5 ms para respuesta inmediata).
 */
#define RYLR998_TICK_MS 5U

/**
 * @brief Intervalo de intento de establecimiento de conexión (Handshake PING) con el Gateway en ms.
 * CONFIGURABLE: Por defecto 30000 ms (30 segundos).
 */
#ifndef RYLR998_HANDSHAKE_INTERVAL_MS
#define RYLR998_HANDSHAKE_INTERVAL_MS 30000UL
#endif

/**
 * @brief Tiempo máximo de espera para recibir la respuesta ACK del Gateway en ms.
 * CONFIGURABLE: Por defecto 5000 ms (5 segundos).
 */
#ifndef RYLR998_HANDSHAKE_TIMEOUT_MS
#define RYLR998_HANDSHAKE_TIMEOUT_MS 5000UL
#endif

/**
 * @brief Tiempo asignado a los sensores para estabilizar y capturar lecturas válidas tras despertar.
 */
#define RYLR998_ACQUISITION_WINDOW_MS 2500UL

/**
 * @brief Bandera global que indica si los sensores deben activarse o permanecer dormidos.
 */
extern bool Sensors_Active;

/**
 * @brief Indica si el módem completó su configuración AT inicial.
 */
extern bool RYLR998_Ready;

/**
 * @brief Obtiene el nombre del estado actual de la máquina de comunicación para depuración serie.
 */
const char* RYLR998_GetStateString(void);

/**
 * @brief Inicializa Serial1 y el módulo RYLR998.
 */
void RYLR998_Init(void);

/**
 * @brief Máquina de estados periódica no bloqueante para gestionar handshake, sensores y telemetría.
 */
void RYLR998_Tick(void);

/**
 * @brief Envía un paquete de datos dirigido a un nodo destino mediante AT+SEND.
 */
bool RYLR998_Send(uint16_t target_addr, const char *data);

#ifdef __cplusplus
}
#endif

#endif /* RYLR998_H */
