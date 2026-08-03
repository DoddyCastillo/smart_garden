#include "SHT31.h"
#include "i2c_async.h" /* Tu driver I2C no bloqueante */

/* === Macros internas derivadas de los tiempos públicos ====================================== */

#define SHT31_CONV_TICKS   (SHT31_CONV_TIME_MS / SHT31_TASK_TICK_MS)
#define SHT31_PERIOD_TICKS (SHT31_PERIOD_MS / SHT31_TASK_TICK_MS)

/* === Variables públicas ===================================================================== */

float SHT31_Temperature = 0.0f;
float SHT31_Humidity = 0.0f;
uint8_t SHT31_Status = SHT31_STATUS_OK;

/* === Variables estáticas internas =========================================================== */

typedef enum { SHT31_STATE_IDLE = 0, SHT31_STATE_WAITING, SHT31_STATE_READ } SHT31_State_t;

static SHT31_State_t shtState = SHT31_STATE_IDLE;
static uint16_t shtPeriodTicks = 0;
static uint16_t shtWaitTicks = 0;

static uint8_t sht31_cmd[2] = {0x24, 0x00}; /* High repeatability, no clock stretching */
static uint8_t sht31_buf[6];

/* Ventanas para la media móvil */
static float tempWindow[SHT31_MA_WINDOW] = {0};
static float humWindow[SHT31_MA_WINDOW] = {0};
static uint8_t maIndex = 0;
static uint8_t maCount = 0;

/* === Prototipos internos ==================================================================== */

static uint8_t sht31_crc8(const uint8_t * data, int len);
static void sht31_update_moving_average(float newT, float newH);
static void sht31_on_write_complete(bool success);
static void sht31_on_read_complete(bool success);

/* === Implementación ======================================================================== */

void SHT31_Init(void) {
    shtState = SHT31_STATE_IDLE;
    shtPeriodTicks = 0;
    shtWaitTicks = 0;

    maIndex = 0;
    maCount = 0;
    for (uint8_t i = 0; i < SHT31_MA_WINDOW; i++) {
        tempWindow[i] = 0.0f;
        humWindow[i] = 0.0f;
    }

    SHT31_Temperature = 0.0f;
    SHT31_Humidity = 0.0f;
    SHT31_Status = SHT31_STATUS_OK;
}

/* CRC8 según datasheet de Sensirion (polinomio 0x31, init 0xFF) */
static uint8_t sht31_crc8(const uint8_t * data, int len) {
    uint8_t crc = 0xFF;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x80U) {
                crc = (uint8_t)((crc << 1) ^ 0x31U);
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/* Actualiza la media móvil con una nueva T y H */
static void sht31_update_moving_average(float newT, float newH) {
    tempWindow[maIndex] = newT;
    humWindow[maIndex] = newH;

    if (maCount < SHT31_MA_WINDOW) {
        maCount++;
    }

    maIndex++;
    if (maIndex >= SHT31_MA_WINDOW) {
        maIndex = 0;
    }

    float sumT = 0.0f;
    float sumH = 0.0f;

    for (uint8_t i = 0; i < maCount; i++) {
        sumT += tempWindow[i];
        sumH += humWindow[i];
    }

    SHT31_Temperature = sumT / (float)maCount;
    SHT31_Humidity = sumH / (float)maCount;
}

/* Callback de escritura I2C (comando de medición enviado) */
static void sht31_on_write_complete(bool success) {
    if (!success) {
        SHT31_Status = SHT31_STATUS_I2C_WRITE;
        shtState = SHT31_STATE_IDLE;
        return;
    }

    /* Comando enviado correctamente: esperar tiempo de conversión */
    shtWaitTicks = SHT31_CONV_TICKS;
    shtState = SHT31_STATE_WAITING;
    SHT31_Status = SHT31_STATUS_WAITING;
}

/* Callback de lectura I2C (datos recibidos) */
static void sht31_on_read_complete(bool success) {
    if (!success) {
        SHT31_Status = SHT31_STATUS_I2C_READ;
        shtState = SHT31_STATE_IDLE;
        return;
    }

    /* Comprobar CRC de T */
    if (sht31_crc8(&sht31_buf[0], 2) != sht31_buf[2]) {
        SHT31_Status = SHT31_STATUS_CRC;
        shtState = SHT31_STATE_IDLE;
        return;
    }

    /* Comprobar CRC de H */
    if (sht31_crc8(&sht31_buf[3], 2) != sht31_buf[5]) {
        SHT31_Status = SHT31_STATUS_CRC;
        shtState = SHT31_STATE_IDLE;
        return;
    }

    uint16_t rawT = (uint16_t)sht31_buf[0] << 8 | sht31_buf[1];
    uint16_t rawH = (uint16_t)sht31_buf[3] << 8 | sht31_buf[4];

    float T = -45.0f + 175.0f * (float)rawT / 65535.0f;
    float H = 100.0f * (float)rawH / 65535.0f;

    sht31_update_moving_average(T, H);

    SHT31_Status = SHT31_STATUS_OK;
    shtState = SHT31_STATE_IDLE;
}

void SHT31_Tick(void) {
    switch (shtState) {

    case SHT31_STATE_IDLE:
        /* Contamos hasta el periodo de muestreo */
        if (++shtPeriodTicks >= SHT31_PERIOD_TICKS) {
            shtPeriodTicks = 0;

            if (I2C_GetState() == I2C_STATE_IDLE) {
                bool ok = I2C_BeginTransmission(SHT31_ADDR, sht31_cmd, 2U, sht31_on_write_complete);
                if (!ok) {
                    SHT31_Status = SHT31_STATUS_I2C_WRITE;
                } else {
                    SHT31_Status = SHT31_STATUS_WAITING;
                }
            } else {
                /* Bus ocupado al intentar arrancar la medición */
                SHT31_Status = SHT31_STATUS_BUS_BUSY;
            }
        }
        break;

    case SHT31_STATE_WAITING:
        if (shtWaitTicks > 0U) {
            shtWaitTicks--;
        } else {
            /* Tiempo de conversión cumplido: solicitar datos */
            if (I2C_GetState() == I2C_STATE_IDLE) {
                bool ok = I2C_RequestFrom(SHT31_ADDR, sht31_buf, 6U, sht31_on_read_complete);
                if (!ok) {
                    SHT31_Status = SHT31_STATUS_I2C_READ;
                    shtState = SHT31_STATE_IDLE;
                } else {
                    shtState = SHT31_STATE_READ;
                    SHT31_Status = SHT31_STATUS_WAITING;
                }
            } else {
                /* Seguimos esperando a que se libere el bus */
                SHT31_Status = SHT31_STATUS_BUS_BUSY;
            }
        }
        break;

    case SHT31_STATE_READ:
        break;

    default:
        shtState = SHT31_STATE_IDLE;
        SHT31_Status = SHT31_STATUS_OK;
        break;
    }
}

const char * SHT31_GetStatusText(void) {
    switch ((SHT31_Status_t)SHT31_Status) {
    case SHT31_STATUS_OK:
        return "OK";
    case SHT31_STATUS_I2C_WRITE:
        return "I2C write error";
    case SHT31_STATUS_I2C_READ:
        return "I2C read error";
    case SHT31_STATUS_CRC:
        return "CRC error";
    case SHT31_STATUS_WAITING:
        return "waiting";
    case SHT31_STATUS_BUS_BUSY:
        return "bus busy";
    default:
        return "unknown";
    }
}