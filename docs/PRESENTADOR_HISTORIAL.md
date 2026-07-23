# Historial y estado de ingeniería

Última actualización: 22 de julio de 2026.

## Base

- Repositorio original: `obsproject/obs-studio`.
- Versión base: OBS Studio 32.2.0.
- Commit base y referencia inmutable `obs-original`: `7546be7266dde276d82d4681fe1ab4fd8e32cf2b`.
- Rama de producto: `feature/media-presenter`.

## Commits del producto

| Commit | Cambio principal |
|---|---|
| `1464af3` | Primer modo Presentador: interfaz dividida, biblioteca, preview y proyector de escenario. |
| `55d2292` | Persistencia, pantallas, sonido, controles inferiores, arrastrar/reordenar y ajustes. |
| `1d8d5db` | Transporte clásico, teclas multimedia y primera activación de audio. |
| `c34437f` | Carpetas, búsqueda por carpeta y optimizaciones de biblioteca. |

El trabajo posterior corrige dos fallos observados en prueba real:

- El salto de tiempo estándar de OBS quedaba encolado y podía reiniciar o ignorar la posición. Se añadió una llamada
  inmediata y el control se cambió a `AbsoluteSlider`.
- El audio solo aparecía después de cambiar manualmente de salida y se perdía al seleccionar otro medio. Ahora el
  monitor se reconstruye por activación y después de `media_started`.
- MP3 con portada incrustada mezclaba el reloj de la imagen estática con el del audio. La fuente activa de música se
  marca `audio_only` y excluye video del decodificador.

Estas correcciones deben conservarse juntas porque atraviesan frontend, libobs, obs-ffmpeg y media-playback.

## Evidencia de compilación

Configuración comprobada:

```text
Windows x64
Visual Studio 18 2026 Build Tools
CMake 4.4.0
RelWithDebInfo
ENABLE_BROWSER=OFF
obs-deps 2026-07-15 x64
obs-deps-qt6 2026-07-15 x64
```

El objetivo `obs-studio` compiló correctamente y generó:

```text
build_x64/rundir/RelWithDebInfo/bin/64bit/obs64.exe
build_x64/rundir/RelWithDebInfo/bin/64bit/obs.dll
build_x64/rundir/RelWithDebInfo/obs-plugins/64bit/obs-ffmpeg.dll
```

## Pruebas realizadas

- Video: la línea de tiempo saltó de aproximadamente `0:01` a `1:20` en un archivo de `1:45` y continuó.
- Audio: medidores estéreo activos y duración detectada.
- Se reprodujo el fallo de MP3 con portada que hacía retroceder el reloj; se implementó `audio_only` y se recompiló.
- La selección guardada `Auriculares (BT3280)` devolvió desde Windows el error `88890004`, correspondiente a un
  dispositivo invalidado/no disponible durante esa sesión. No confundirlo con un fallo del mezclador.

## Prueba pendiente inmediata

La última compilación con `audio_only` quedó instalada y abierta, pero la comprobación automatizada final fue
interrumpida antes de seleccionar la canción. Al retomar:

1. Abrir `dist/PresentadorMultimedia/INICIAR_PRESENTADOR.bat` si no está abierto.
2. Elegir una salida de audio físicamente conectada.
3. Reproducir un MP3 durante 10 segundos.
4. Mover la línea de tiempo cerca del 75 % y confirmar que el tiempo salta y sigue aumentando sin regresar.
5. Cambiar a video y después a otra canción sin abrir `Sonido`; confirmar audio en ambos cambios.
6. Revisar medidores y el log más reciente.

No declarar resuelto el audio físico hasta completar esa secuencia con un dispositivo conectado.

## Próximos límites conocidos

- La monitorización depende de que Windows mantenga válido el identificador del dispositivo elegido.
- La biblioteca referencia archivos externos; aún no existe un modo de colección que copie medios al proyecto.
- Los efectos de salida actuales deben validarse con distintos formatos y dispositivos.
- La segunda tarjeta del diálogo de pantallas sigue reservada para una salida futura.

## Cómo investigar regresiones

```powershell
git log --oneline obs-original..feature/media-presenter
git diff obs-original...feature/media-presenter -- frontend/widgets/PresenterPanel.cpp
git bisect start
git bisect bad feature/media-presenter
git bisect good obs-original
```

Los logs portátiles se encuentran en `dist/PresentadorMultimedia/config/obs-studio/logs`. Para audio buscar
`Audio monitoring device`, `audio_monitor` y errores WASAPI; para medios buscar el nombre de la fuente y FFmpeg.
