# Presentador multimedia: reglas de continuidad

Este repositorio es un derivado local de OBS Studio 32.2.0. La rama de producto es
`feature/media-presenter`; `obs-original` señala el commit limpio usado como base.

Antes de modificar el proyecto, leer completos:

1. `docs/PRESENTER_MVP.md`
2. `docs/PRESENTADOR_CONTINUIDAD.md`
3. `docs/PRESENTADOR_ARQUITECTURA.md`
4. `docs/PRESENTADOR_HISTORIAL.md`

Antes de revisar o incorporar una versión nueva de OBS, leer además `docs/OBS_ACTUALIZACIONES.md` y ejecutar
`presenter-tools/Review-OBS-Update.cmd`.

## Principios que no se deben romper

- Conservar el código original de OBS. Ocultar o desacoplar funciones en el frontend antes que eliminarlas.
- Mantener las personalizaciones en commits pequeños sobre `feature/media-presenter`.
- Comparar siempre contra `obs-original`; no usar `master` como rama de desarrollo.
- La escena privada de `PresenterPanel` es la única fuente de verdad para vista previa y escenario.
- Al activar otro archivo debe existir una sola fuente multimedia activa.
- La biblioteca guarda rutas, no copia los archivos del usuario. No versionar medios ni configuración personal.
- No versionar `.deps`, `.tools`, `build_x64`, `build_x86` ni `dist`.
- No ejecutar el actualizador binario oficial sobre Presentador ni fusionar directamente `obs-public/master`.
- Una actualización de OBS se incorpora por commits justificados en una rama de revisión; `AISLADO` no significa
  automáticamente `necesario`.
- Probar por separado imagen, video, audio, cambio de medio, búsqueda, carpetas, persistencia, salida de pantalla y salida de audio.
- Para cambios en reproducción o audio, comprobar además el cambio repetido entre dos archivos; una primera reproducción correcta no es suficiente.

## Flujo mínimo antes de entregar un cambio

```powershell
git status --short
git diff --check
.\presenter-tools\Build-Presenter.cmd
.\presenter-tools\Package-Presenter.cmd
.\presenter-tools\Run-Presenter.cmd
```

Después de la prueba funcional, documentar el resultado y crear un commit descriptivo. No mezclar una actualización
masiva de OBS con una función del presentador en el mismo commit.

## Puntos de entrada

- Interfaz y coordinación: `frontend/widgets/PresenterPanel.cpp` y `.hpp`.
- Integración con la ventana de OBS: `frontend/widgets/OBSBasic*`.
- API multimedia añadida: `libobs/obs.h` y `libobs/obs-source.c`.
- Fuente FFmpeg y reproducción: `plugins/obs-ffmpeg/` y `shared/media-playback/`.
- Herramientas reproducibles: `presenter-tools/`.

Si el estado real y la documentación difieren, actualizar primero `docs/PRESENTADOR_HISTORIAL.md` con la evidencia
encontrada y después continuar.
