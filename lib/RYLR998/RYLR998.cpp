#include "RYLR998.h"
#include "SHT31.h"
#include "MHZ19.h"
#include "DS18B20.h"

bool Sensors_Active = false;
bool RYLR998_Ready = false;

typedef enum {
    RYLR_BOOT_WAIT = 0,
    RYLR_SEND_TEST_AT,
    RYLR_SET_ADDRESS,
    RYLR_SET_NETWORKID,
    RYLR_SET_BAND,
    RYLR_SET_PARAMETER,
    RYLR_SET_MODE,
    RYLR_STANDBY_SLEEP,
    RYLR_WAIT_HANDSHAKE_ACK,
    RYLR_ACQUIRING_SENSORS,
    RYLR_WAIT_TELEMETRY_TX
} rylr_state_t;

static rylr_state_t state = RYLR_BOOT_WAIT;
static uint32_t state_timer = 0;
static uint32_t last_handshake_attempt = 0;
static uint32_t acquisition_start_time = 0;

static char line_buf[100];
static uint8_t line_pos = 0;
static bool got_ok = false;
static bool got_err = false;
static bool got_ack = false;

static void send_cmd(const char *cmd) {
    got_ok = false;
    got_err = false;
    Serial.print(F("    [LoRa TX CMD]: "));
    Serial.println(cmd);
    Serial1.print(cmd);
    Serial1.print(F("\r\n"));
    state_timer = millis();
}

const char* RYLR998_GetStateString(void) {
    switch (state) {
        case RYLR_BOOT_WAIT:          return "Iniciando módem";
        case RYLR_SEND_TEST_AT:
        case RYLR_SET_ADDRESS:
        case RYLR_SET_NETWORKID:
        case RYLR_SET_BAND:
        case RYLR_SET_PARAMETER:
        case RYLR_SET_MODE:           return "Configurando AT";
        case RYLR_STANDBY_SLEEP:      return "Reposo (Sensores Dormidos)";
        case RYLR_WAIT_HANDSHAKE_ACK: return "Esperando ACK Gateway";
        case RYLR_ACQUIRING_SENSORS:  return "Midiendo Sensores";
        case RYLR_WAIT_TELEMETRY_TX:  return "Transmitiendo Datos";
        default:                      return "Desconocido";
    }
}

void RYLR998_Init(void) {
    state = RYLR_BOOT_WAIT;
    state_timer = millis();
    last_handshake_attempt = 0;
    line_pos = 0;
    got_ok = false;
    got_err = false;
    got_ack = false;
    Sensors_Active = false;
    RYLR998_Ready = false;

    // Inicializar puerto hardware UART Serial1 (Pines 0 RX y 1 TX)
    Serial1.begin(RYLR998_BAUD_RATE);
}

static void process_serial_input(void) {
    while (Serial1.available() > 0) {
        char c = (char)Serial1.read();
        if (c == '\r' || c == '\n') {
            if (line_pos > 0) {
                line_buf[line_pos] = '\0';

                // Imprimir cada línea recibida del módem
                Serial.print(F("    [LoRa UART RX]: "));
                Serial.println(line_buf);

                if (strstr(line_buf, "+OK") != NULL) {
                    got_ok = true;
                } else if (strstr(line_buf, "+ERR=") != NULL) {
                    got_err = true;
                }

                // Detectar paquete recibido (+RCV=...,ACK,...) sea de la dirección 1 o 0
                if (strstr(line_buf, "+RCV") != NULL && strstr(line_buf, "ACK") != NULL) {
                    got_ack = true;
                    Serial.println(F("    >>> [LoRa] ¡ACK confirmado por el módem local!"));
                }

                line_pos = 0;
            }
        } else {
            if (line_pos < sizeof(line_buf) - 1) {
                line_buf[line_pos++] = c;
            } else {
                line_pos = 0; // Desborde defensivo
            }
        }
    }
}

bool RYLR998_Send(uint16_t target_addr, const char *data) {
    size_t len = strlen(data);
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "AT+SEND=%u,%u,%s", target_addr, (unsigned int)len, data);
    send_cmd(cmd);
    return true;
}

void RYLR998_Tick(void) {
    process_serial_input();
    uint32_t now = millis();

    switch (state) {

    /* --- FASE 1: CONFIGURACIÓN INICIAL DEL MÓDEM --- */
    case RYLR_BOOT_WAIT:
        if (now - state_timer >= 800) {
            Serial.println(F("\n>>> [LoRa Init] Iniciando configuración por comandos AT..."));
            send_cmd("AT");
            state = RYLR_SEND_TEST_AT;
        }
        break;

    case RYLR_SEND_TEST_AT:
        if (got_ok) {
            char cmd[32];
            snprintf(cmd, sizeof(cmd), "AT+ADDRESS=%u", RYLR998_NODE_ADDRESS);
            send_cmd(cmd);
            state = RYLR_SET_ADDRESS;
        } else if (now - state_timer >= 1000) {
            send_cmd("AT");
        }
        break;

    case RYLR_SET_ADDRESS:
        if (got_ok) {
            char cmd[32];
            snprintf(cmd, sizeof(cmd), "AT+NETWORKID=%u", RYLR998_NETWORK_ID);
            send_cmd(cmd);
            state = RYLR_SET_NETWORKID;
        } else if (now - state_timer >= 1000) {
            send_cmd("AT");
            state = RYLR_SEND_TEST_AT;
        }
        break;

    case RYLR_SET_NETWORKID:
        if (got_ok) {
            char cmd[32];
            snprintf(cmd, sizeof(cmd), "AT+BAND=%lu", RYLR998_FREQUENCY_HZ);
            send_cmd(cmd);
            state = RYLR_SET_BAND;
        } else if (now - state_timer >= 1000) {
            send_cmd("AT");
            state = RYLR_SEND_TEST_AT;
        }
        break;

    case RYLR_SET_BAND:
        if (got_ok) {
            send_cmd("AT+PARAMETER=9,7,1,12");
            state = RYLR_SET_PARAMETER;
        } else if (now - state_timer >= 1000) {
            send_cmd("AT");
            state = RYLR_SEND_TEST_AT;
        }
        break;

    case RYLR_SET_PARAMETER:
        if (got_ok) {
            send_cmd("AT+MODE=0");
            state = RYLR_SET_MODE;
        } else if (now - state_timer >= 1000) {
            send_cmd("AT");
            state = RYLR_SEND_TEST_AT;
        }
        break;

    case RYLR_SET_MODE:
        if (got_ok) {
            RYLR998_Ready = true;
            Sensors_Active = false; // Sensores en reposo
            last_handshake_attempt = now - RYLR998_HANDSHAKE_INTERVAL_MS; // Disparar primer intento de inmediato
            state = RYLR_STANDBY_SLEEP;
            Serial.println(F(">>> [LoRa Init] Módem RYLR998 configurado exitosamente. Entrando en Modo Reposo.\n"));
        } else if (now - state_timer >= 1000) {
            send_cmd("AT");
            state = RYLR_SEND_TEST_AT;
        }
        break;

    /* --- FASE 2: REPOSO Y DISPARO DE HANDSHAKE (CADA 30s) --- */
    case RYLR_STANDBY_SLEEP:
        Sensors_Active = false; // Mantener sensores dormidos

        if (now - last_handshake_attempt >= RYLR998_HANDSHAKE_INTERVAL_MS) {
            last_handshake_attempt = now;
            got_ack = false;
            got_ok = false;

            Serial.println(F("\n=================================================="));
            Serial.println(F(">>> [LoRa] 30s cumplidos. Enviando Handshake (PING) al Gateway..."));
            Serial.println(F("=================================================="));

            // Enviar solicitud de handshake "PING" al Gateway (ID 1)
            RYLR998_Send(RYLR998_GATEWAY_ADDRESS, "PING");
            state_timer = now;
            state = RYLR_WAIT_HANDSHAKE_ACK;
        }
        break;

    /* --- FASE 3: ESPERA DE RESPUESTA ACK (TIMEOUT 5s) --- */
    case RYLR_WAIT_HANDSHAKE_ACK:
        if (got_ack) {
            // ¡Handshake exitoso! Activar sensores
            Sensors_Active = true;
            DS18B20_Trigger();
            acquisition_start_time = now;
            Serial.println(F(">>> [LoRa] ¡ACK recibido del Gateway! Conexión confirmada."));
            Serial.println(F(">>> [Sensores] ¡DESPERTANDO SENSORES! Iniciando ciclo de captura (2.5s)..."));
            state = RYLR_ACQUIRING_SENSORS;
        } else if (now - state_timer >= RYLR998_HANDSHAKE_TIMEOUT_MS) {
            // Timeout de 5s: Gateway no disponible -> Sensores permanecen dormidos
            Sensors_Active = false;
            Serial.println(F(">>> [LoRa] Timeout (5s) sin respuesta del Gateway. Gateway apagado o fuera de alcance."));
            Serial.println(F(">>> [Sensores] Los sensores PERMANECEN DORMIDOS para ahorrar energía."));
            state = RYLR_STANDBY_SLEEP;
        }
        break;

    /* --- FASE 4: CAPTURA ACTIVA DE SENSORES --- */
    case RYLR_ACQUIRING_SENSORS:
        Sensors_Active = true;

        // Esperar la ventana de adquisición para que SHT31, DS18B20 (750ms) y MH-Z19C tomen lectura
        if (now - acquisition_start_time >= RYLR998_ACQUISITION_WINDOW_MS) {
            // Construir trama de telemetría con los valores recién medidos
            char payload[80];
            char t1_str[10], h_str[10], co2_str[10], t2_str[10];

            dtostrf(SHT31_Temperature, 1, 2, t1_str);
            dtostrf(SHT31_Humidity, 1, 2, h_str);
            dtostrf(MHZ19_CO2, 1, 1, co2_str);
            dtostrf(DS18B20_Temperature, 1, 2, t2_str);

            snprintf(payload, sizeof(payload), "T1:%s,H:%s,CO2:%s,T2:%s",
                     t1_str, h_str, co2_str, t2_str);

            Serial.print(F(">>> [LoRa] Transmitiendo telemetría al Gateway: "));
            Serial.println(payload);

            // Enviar telemetría al Gateway
            RYLR998_Send(RYLR998_GATEWAY_ADDRESS, payload);
            state_timer = now;
            state = RYLR_WAIT_TELEMETRY_TX;
        }
        break;

    /* --- FASE 5: ESPERA DE CONFIRMACIÓN Y DORMIR SENSORES --- */
    case RYLR_WAIT_TELEMETRY_TX:
        if (got_ok || (now - state_timer >= 1500)) {
            // Envío completado con éxito -> Dormir sensores
            Sensors_Active = false;
            last_handshake_attempt = now; // Reiniciar cuenta de 30 segundos
            Serial.println(F(">>> [LoRa] Telemetría enviada (+OK)."));
            Serial.println(F(">>> [Sensores] DURMIENDO SENSORES. Próximo intento en 30 segundos.\n"));
            state = RYLR_STANDBY_SLEEP;
        }
        break;

    default:
        state = RYLR_BOOT_WAIT;
        break;
    }
}
