/*
 * display_bridge.c
 *
 * Archivo NUEVO, no reemplaza ni modifica nada existente de BlueRetro.
 * Corre en el nucleo 0 (junto al Bluetooth normal) y envia por UART al
 * segundo ESP32 (el de la pantalla):
 *
 *   - Hash/GameID de Swiss (via gid_get()):   "06F64606FD31A657\n"
 *   - Info de la SD:                          "SD:<total>,<free>\n"
 *   - Estado de gamepads:                     "GP:<puerto>,<0|1>\n"
 *
 * NOTA IMPORTANTE: lo que gid_get() devuelve NO es el nombre del juego en
 * texto legible - es un hash unico calculado por Swiss a partir del
 * contenido del juego. La traduccion de "este hash = este juego" se hace
 * del lado de la pantalla.
 *
 * SINCRONIZACION CON EL DISPLAY:
 * Como el UART es unidireccional, el display no puede pedir el estado al
 * arrancar. Si la consola se prende desde el gamepad, el control ya esta
 * conectado cuando el display todavia esta cargando y los mensajes
 * enviados "una sola vez" se perderian. Para evitarlo, el estado se
 * trata como ESTADO REPETIDO y no como evento unico:
 *   - Los cambios se siguen enviando de inmediato (respuesta instantanea).
 *   - Cada STATE_RESEND_MS la tarea reenvia hash, SD y los 4 gamepads.
 * El display procesa estos mensajes de forma idempotente, asi que recibir
 * el mismo estado varias veces no tiene efectos secundarios.
 *
 * display_bridge_send_gamepad_status() se llama desde sys_mgr (manager.c)
 * cada vez que cambia el LED de un puerto, SOLO cuando el sistema activo
 * es la GameCube modificada. Reusa el mismo UART_NUM_1, protegido por el
 * mutex interno del driver de UART de ESP-IDF, asi que es seguro llamarlo
 * desde otra tarea.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "adapter/gameid.h"
#include "display_bridge.h"

#define DISPLAY_UART_PORT    UART_NUM_1
#define DISPLAY_UART_TX_PIN  25   /* GPIO25 - confirmado libre en el esquema del usuario */
#define DISPLAY_UART_BAUD    9600

#define TASK_PERIOD_MS       500
#define STATE_RESEND_MS      1000  /* cada cuanto se reenvia el estado completo */
#define GAMEPAD_PORTS        4

static char last_sent_hash[24] = {0};
static uint32_t last_sent_sd_total = 0xFFFFFFFF;
static uint32_t last_sent_sd_free = 0xFFFFFFFF;

/* Ultimo estado conocido de cada puerto. 0xFF = nunca se informo. */
static volatile uint8_t gp_state[GAMEPAD_PORTS] = {0xFF, 0xFF, 0xFF, 0xFF};

static void send_gp_line(uint8_t port, uint8_t connected) {
    char line[16];
    int len = snprintf(line, sizeof(line), "GP:%u,%u\n", port, connected ? 1 : 0);
    uart_write_bytes(DISPLAY_UART_PORT, line, len);
}

static void send_hash_line(const char *hex_id) {
    char line[32];
    int len = snprintf(line, sizeof(line), "%s\n", hex_id);
    uart_write_bytes(DISPLAY_UART_PORT, line, len);
}

static void send_sd_line(uint32_t total, uint32_t free_gb) {
    char line[32];
    int len = snprintf(line, sizeof(line), "SD:%lu,%lu\n",
                       (unsigned long)total, (unsigned long)free_gb);
    uart_write_bytes(DISPLAY_UART_PORT, line, len);
}

static void display_bridge_task(void *arg) {
    uint32_t since_resend_ms = 0;

    while (1) {
        bool resend = (since_resend_ms >= STATE_RESEND_MS);
        if (resend) {
            since_resend_ms = 0;
        }

        /* --- Hash del juego --- */
        const char *hex_id = gid_get();
        if (hex_id[0] != '\0') {
            if (strcmp(hex_id, last_sent_hash) != 0) {
                strncpy(last_sent_hash, hex_id, sizeof(last_sent_hash) - 1);
                send_hash_line(hex_id);
                printf("[display_bridge] Hash enviado: %s\n", hex_id);
            } else if (resend) {
                send_hash_line(hex_id);
            }
        }

        /* --- Info de la SD --- */
        uint32_t sd_total = sd_info_get_total();
        uint32_t sd_free = sd_info_get_free();
        if (sd_total != last_sent_sd_total || sd_free != last_sent_sd_free) {
            last_sent_sd_total = sd_total;
            last_sent_sd_free = sd_free;
            send_sd_line(sd_total, sd_free);
            printf("[display_bridge] SD info enviado: %lu/%lu GB\n",
                   (unsigned long)sd_total, (unsigned long)sd_free);
        } else if (resend) {
            send_sd_line(sd_total, sd_free);
        }

        /* --- Estado de gamepads (reenvio periodico) --- */
        if (resend) {
            for (uint8_t p = 0; p < GAMEPAD_PORTS; p++) {
                uint8_t st = gp_state[p];
                if (st != 0xFF) {
                    send_gp_line(p, st);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD_MS));
        since_resend_ms += TASK_PERIOD_MS;
    }
}

void display_bridge_send_gamepad_status(uint8_t port, uint8_t connected) {
    uint8_t state = connected ? 1 : 0;

    if (port < GAMEPAD_PORTS) {
        gp_state[port] = state;
    }

    /* Envio inmediato para que el display reaccione al instante */
    send_gp_line(port, state);

    printf("[display_bridge] Gamepad P%u: %s\n", port + 1, connected ? "conectado" : "desconectado");
}

void display_bridge_init(void) {
    uart_config_t uart_config = {
        .baud_rate = DISPLAY_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_param_config(DISPLAY_UART_PORT, &uart_config);
    uart_set_pin(DISPLAY_UART_PORT, DISPLAY_UART_TX_PIN, UART_PIN_NO_CHANGE,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(DISPLAY_UART_PORT, 256, 0, 0, NULL, 0);

    xTaskCreatePinnedToCore(display_bridge_task, "display_bridge", 2048, NULL, 1, NULL, 0);
}
