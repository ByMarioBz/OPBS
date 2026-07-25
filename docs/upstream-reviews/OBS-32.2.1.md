# Revisión de OBS 32.2.1

- Generada: 2026-07-25
- Base integrada: 32.2.0 (`7546be7266dde276d82d4681fe1ab4fd8e32cf2b`)
- Objetivo oficial: 32.2.1 (`0052d024fd6a5ff1aa04c76cbdffd3085a5dfacc`)
- Resultado: 0 conflictos, 0 cambios sensibles, 4 cambios aislados

## Commits oficiales

- `d7a6a8e1b` win-capture: Resolve inject helper and hook path to absolute paths
- `74579d845` CI: Increase number of update deltas generated for macOS to 5
- `e2e0bfc8f` win-capture: Log hook DLL install / update errors
- `0052d024f` libobs: Update version to 32.2.1

## Archivos cambiados

| Clasificación | Archivo | Motivo |
|---|---|---|
| `AISLADO` | `.github/workflows/publish.yaml` | No coincide con un archivo modificado por Presentador. |
| `AISLADO` | `libobs/obs-config.h` | No coincide con un archivo modificado por Presentador. |
| `AISLADO` | `plugins/win-capture/game-capture-file-init.c` | No coincide con un archivo modificado por Presentador. |
| `AISLADO` | `plugins/win-capture/game-capture.c` | No coincide con un archivo modificado por Presentador. |

`AISLADO` significa que el cambio no pisa actualmente una personalización; no significa que sea necesario.

## Decisión del producto

Aplazada. La corrección funcional de esta entrega afecta `win-capture` y la captura de juegos no está expuesta en el
Presentador actual. Se volverá a evaluar antes de recuperar captura, transmisión o grabación. No se actualiza
`lastIntegratedUpstream` ni la versión declarada del producto.
