/*
 * display_bridge.c
 *
 * Archivo NUEVO, no reemplaza ni modifica nada existente de BlueRetro.
 * Corre en el nucleo 0 (junto al Bluetooth normal) y hace una sola cosa:
 * cada 500ms revisa el GameID/hash que BlueRetro ya recibio de Swiss (via
 * gid_get(), ya incluido en el proyecto) y si cambio, lo manda tal cual
 * (como texto hexadecimal) por UART a un segundo ESP32 (el de la pantalla).
 *
 * NOTA IMPORTANTE: lo que gid_get() devuelve NO es el nombre del juego en
 * texto legible - es un hash unico calculado por Swiss a partir del
 * contenido del juego. La traduccion de "este hash = este juego" se hace
 * del lado de la pantalla, con una tabla que se va llenando a mano segun
 * los juegos que el usuario realmente tiene.
 *
 * Formato enviado por UART: el hash en texto + salto de linea.
 * Ejemplo: "06F64606FD31A657\n"
 *
 * Tambien expone display_bridge_send_gamepad_status(), llamada desde
 * sys_mgr (manager.c) cada vez que cambia el LED de un puerto, SOLO
 * cuando el sistema activo es la GameCube modificada. Reusa el mismo
 * UART_NUM_1 ya instalado aca, protegido por el mutex interno del
 * driver de UART de ESP-IDF -- es seguro llamarlo desde otra tarea.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "adapter/gameid.h"
#include "display_bridge.h"

#define DISPLAY_UART_PORT   UART_NUM_1
#define DISPLAY_UART_TX_PIN 25   /* GPIO25 - confirmado libre en el esquema del usuario */
#define DISPLAY_UART_BAUD    9600

static char last_sent_hash[24] = {0};
static uint32_t last_sent_sd_total = 0xFFFFFFFF;
static uint32_t last_sent_sd_free = 0xFFFFFFFF;

static void display_bridge_task(void *arg) {
    while (1) {
        const char *hex_id = gid_get();

        if (hex_id[0] != '\0' && strcmp(hex_id, last_sent_hash) != 0) {
            strncpy(last_sent_hash, hex_id, sizeof(last_sent_hash) - 1);

            char line[32];
            int len = snprintf(line, sizeof(line), "%s\n", hex_id);
            uart_write_bytes(DISPLAY_UART_PORT, line, len);

            printf("[display_bridge] Hash enviado: %s\n", hex_id);
        }

        uint32_t sd_total = sd_info_get_total();
        uint32_t sd_free = sd_info_get_free();
        if (sd_total != last_sent_sd_total || sd_free != last_sent_sd_free) {
            last_sent_sd_total = sd_total;
            last_sent_sd_free = sd_free;

            char line[32];
            int len = snprintf(line, sizeof(line), "SD:%lu,%lu\n",
                                (unsigned long)sd_total, (unsigned long)sd_free);
            uart_write_bytes(DISPLAY_UART_PORT, line, len);

            printf("[display_bridge] SD info enviado: %lu/%lu GB\n",
                   (unsigned long)sd_total, (unsigned long)sd_free);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void display_bridge_send_gamepad_status(uint8_t port, uint8_t connected) {
    char line[16];
    int len = snprintf(line, sizeof(line), "GP:%u,%u\n", port, connected ? 1 : 0);
    uart_write_bytes(DISPLAY_UART_PORT, line, len);

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
