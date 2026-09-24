#ifndef _DISPLAY_BRIDGE_H_
#define _DISPLAY_BRIDGE_H_

#include <stdint.h>

/* Llamar UNA vez desde app_main(), despues de que BlueRetro haya
   terminado su propia inicializacion. */
void display_bridge_init(void);

/* Envia el estado de conexion de un puerto de gamepad (0-3) al ESP32
   de la pantalla, ej. "GP:0,1\n" = puerto 1 conectado.
   Se usa desde sys_mgr (set_port_led), solo cuando el sistema activo
   es la GameCube modificada -- ver el chequeo en manager.c. */
void display_bridge_send_gamepad_status(uint8_t port, uint8_t connected);

#endif /* _DISPLAY_BRIDGE_H_ */
