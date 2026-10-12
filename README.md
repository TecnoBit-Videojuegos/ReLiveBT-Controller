# ReLiveBT Controller

Adaptador Bluetooth multijugador para controles en consolas retro, desarrollado por **TecnoBit Videojuegos**.

Este proyecto es un fork de [BlueRetro](https://github.com/darthcloud/BlueRetro), creado por Jacques Gagnon (darthcloud). El proyecto original está archivado y ya no recibe cambios, así que ReLiveBT continúa su desarrollo con modificaciones propias. Todo el crédito del trabajo base es de su autor y de la comunidad de BlueRetro.

## Qué es

Permite usar controles Bluetooth modernos (Wii, Switch, PS3, PS4, PS5, Xbox One, Xbox Series X|S y controles HID genéricos) en consolas retro, usando un adaptador con el chip ESP32.

## Qué cambia respecto a BlueRetro

- **Nombre Bluetooth:** el adaptador se anuncia como "ReLiveBT".
- **Soporte PlayStation (PS1/PS2) ajustado:** el botón extra usa GPIO23 en lugar de GPIO0.
- **Sleek GameCube (SGC):** una versión de GameCube con funciones extra:
  - Botón touch de encendido.
  - LED de estado.
  - Puente UART con una pantalla.
- **Compilación simplificada:** una configuración por consola, todas se compilan automáticamente en GitHub Actions.

## Consolas compiladas actualmente

| Nombre | Consola | Configuración |
|---|---|---|
| `gamecube` | GameCube | `configs/hw2/gamecube` |
| `n64` | Nintendo 64 | `configs/hw2/n64` |
| `playstation` | PlayStation 1 / 2 | `configs/hw2/playstation` |
| `sgc` | Sleek GameCube | `configs/hw2/sgc` |

El resto de las consolas de BlueRetro siguen en el código, pero no se compilan ni se prueban en este fork.

## Cómo obtener el firmware

1. Entra a la pestaña **Actions** de este repositorio.
2. Abre la ejecución más reciente de **build**.
3. En la sección **Artifacts**, descarga el de tu consola (por ejemplo `hw2_playstation`).

Cada descarga incluye `bootloader.bin`, `partition-table.bin`, `ota_data_initial.bin` y el firmware `BlueRetro...bin`.

## Notas de desarrollo

Los detalles técnicos (pines, qué es exclusivo de cada consola, reglas para no generar conflictos) están en [NOTAS.md](NOTAS.md).

## Documentación del proyecto original

La documentación general de BlueRetro (emparejado de controles, configuración, mapeos) sigue siendo válida en gran parte:
[Wiki de BlueRetro](https://github.com/darthcloud/BlueRetro/wiki)

## Licencia

El software se distribuye bajo la licencia [Apache-2.0](LICENSE), igual que BlueRetro. El hardware original de BlueRetro está bajo CERN-OHL-P-2.0.
