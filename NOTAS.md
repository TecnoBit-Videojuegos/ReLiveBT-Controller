# Notas del proyecto ReLiveBT-Controller

Fork de [BlueRetro](https://github.com/darthcloud/BlueRetro) (el original está archivado, ya no recibe cambios, así que no hay nada que sincronizar). Todo se edita y se compila desde la web de GitHub.

## Consolas que se compilan

El archivo `.github/workflows/build.yml` compila todas a la vez. Cada una sale como un artefacto aparte en la pestaña **Actions**.

| Nombre | Qué es | Archivo de configuración |
|---|---|---|
| `gamecube` | GameCube normal | `configs/hw2/gamecube` |
| `n64` | Nintendo 64 | `configs/hw2/n64` |
| `playstation` | PS1 / PS2 | `configs/hw2/playstation` |
| `sgc` | Sleek GameCube (GameCube + extras) | `configs/hw2/sgc` |

**Para agregar otra consola:** crear su archivo en `configs/hw2/` y escribir su nombre en la lista `br_config` de `build.yml`.

**Para compilar y bajar el firmware:** pestaña **Actions** → última ejecución de "build" → sección **Artifacts** → descargar `hw2_<consola>`. Si no arranca solo, usar **Run workflow**.

## Qué es exclusivo de cada consola

| Función | PlayStation | GameCube | N64 | SGC |
|---|---|---|---|---|
| Botón extra | GPIO23 | GPIO0 | GPIO0 | GPIO0 |
| Botón touch de power (pin 21) | No | No | No | Sí |
| LED de estado (pin 22) | No | No | No | Sí |
| Pantalla por UART (pin 25) | No | No | No | Sí |

Cómo está hecho:
- **Botón extra:** en `main/system/manager.c`, `BOOT_BTN_PIN` vale 23 si la consola es PlayStation (`CONFIG_BLUERETRO_SYSTEM_PSX_PS2`) y 0 en el resto.
- **Touch, LED y pantalla:** todo el código va dentro de `#ifdef CONFIG_BLUERETRO_SGC`. Esa opción solo se activa con la configuración `sgc` (definida en `main/Kconfig.projbuild`).

## Por qué se separó así (no deshacer)

Estos pines chocan con el control en PlayStation:
- Pin 21 = `P1_DSR` y pin 22 = `P2_RXD` en PS (el botón touch y el LED los pisaban).
- Pin 25 = `P2_DSR` en PS (la pantalla lo pisaba).

GameCube y N64 usan los pines 19, 5, 26 y 27, así que no chocan con nada.
PlayStation usa los pines 34, 33, 32, 19, 21, 5, 26, 27, 22, 25, 12 y 15. El 23 está libre.

**Si algún día se compila otra consola** (SNES, Jaguar, PC Engine, Parallel, CD-i, etc.), revisar antes sus pines: varias usan el 23, el 21, el 22 o el 25 y podrían chocar con lo anterior.

## Archivos que son nuestros (cambiados respecto a BlueRetro)

- `.github/workflows/build.yml`: compila todas las consolas.
- `main/system/manager.c`: botón extra, touch, LED de estado, aviso a la pantalla.
- `main/display_bridge.c` y `.h`: comunicación UART con la pantalla.
- `main/main.c`: arranca la pantalla solo en SGC.
- `main/Kconfig.projbuild`: opción `BLUERETRO_SGC`.
- `main/adapter/adapter.c`, `gameid.c`, `main/wired/nsi.c`: ID del juego y datos de la SD.
- `main/bluetooth/hci.c` y `host.c`: ajustes de Bluetooth.
- `configs/hw2/sgc`: configuración de la Sleek GameCube.

## Estado actual

- [x] Fork ordenado: ya no se edita `build.yml` a mano para cambiar de consola.
- [x] Botón extra, touch, LED y pantalla separados por consola.
- [x] Probar en hardware el firmware de PS: botón en GPIO23, puerto 2 y que la consola no se encienda o apague sola.
- [ ] Probar GameCube normal y SGC.
- [ ] Reemplazar el README (todavía es el de BlueRetro).

## Reglas para no volver a tener choques

1. Nunca editar `build.yml` para cambiar de consola. Solo agregar o quitar nombres de la lista.
2. Todo lo que sea de una sola consola va dentro de `#ifdef` o con su propia configuración. Nada de pines globales.
3. Antes de usar un pin nuevo, revisar que no lo use ninguna consola de la tabla.
4. Después de cada cambio, mirar **Actions** y confirmar que todas las consolas salen en verde.
