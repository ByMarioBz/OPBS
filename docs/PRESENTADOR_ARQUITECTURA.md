# Arquitectura del Presentador multimedia

## Objetivo de diseño

Reutilizar renderizado, fuentes FFmpeg, audio, monitores y proyectores de OBS, pero presentar una interfaz dedicada y
simple. El frontend original se conserva para recuperar funciones progresivamente.

## Flujo principal

```text
Biblioteca persistente
        |
        v
PresenterPanel::ActivateMedia
        |
        +-- imagen --> image_source
        |
        +-- video/audio --> ffmpeg_source
        |
        v
Escena privada con un solo elemento activo
        |
        +-- vista previa OBSQTDisplay
        |
        +-- OBSProjector de escenario a pantalla completa
        |
        +-- monitor de audio seleccionado
```

La vista previa y el escenario renderizan la misma escena. Cambiar de tarjeta detiene y retira la fuente anterior antes
de insertar la nueva; no deben existir dos contenidos superpuestos.

## Componentes modificados

### Interfaz

- `frontend/widgets/PresenterPanel.cpp` y `.hpp`: biblioteca, carpetas, búsqueda, persistencia, preview, transporte,
  tiempo, medidores, volumen, diálogos de pantallas/sonido y coordinación de fuentes.
- `frontend/widgets/OBSBasic.cpp` y `.hpp`: crea y muestra el modo presentador.
- `frontend/widgets/OBSBasic_Projectors.cpp`: acceso controlado al proyector de OBS.
- `frontend/cmake/ui-widgets.cmake`: incluye los archivos nuevos en la compilación.

### Reproducción

- `libobs/obs.h` y `libobs/obs-source.c`: añade un cambio de tiempo inmediato para evitar que la cola asíncrona deje
  obsoleto un salto solicitado desde la interfaz.
- `plugins/obs-ffmpeg/obs-ffmpeg-source.c`: propaga la intención `audio_only`.
- `shared/media-playback/media-playback/*`: en música omite la pista de video/portada durante la reproducción para que
  el reloj y los saltos dependan únicamente del audio.

La miniatura de una canción puede seguir generándose antes mediante la biblioteca. `audio_only` solo afecta la fuente
activa de reproducción.

## Persistencia

La aplicación usa configuración portátil de OBS bajo:

```text
dist/PresentadorMultimedia/config/obs-studio
```

Se recuerdan biblioteca, carpetas, orden, carpeta activa, monitor, estado de salida, ajustes de sonido y geometría de
ventana. Los registros de ejecución están bajo `config/obs-studio/logs` y son la primera fuente para diagnosticar fallos.

No versionar esa configuración: contiene rutas locales y preferencias del usuario.

## Audio

La fuente multimedia se configura para monitorización. Al activar cada archivo se reconstruye el monitor de audio y se
restablece la monitorización después de que FFmpeg emita `media_started`. Esto reproduce automáticamente la operación
que antes solo ocurría al cambiar manualmente el dispositivo en `Sonido`.

Las fuentes globales heredadas de escritorio/micrófono se desactivan en el modo presentador para evitar competencia o
duplicación. Un dispositivo Bluetooth desconectado puede seguir siendo una preferencia guardada; Windows devolverá un
error de dispositivo invalidado y el usuario debe seleccionar una salida conectada.

## Rendimiento

- Las fuentes de audio/video se crean al reproducir y no para todas las tarjetas.
- Las miniaturas se solicitan de forma diferida.
- La biblioteca filtra por carpeta y nombre sin cargar medios completos.
- La escena contiene un único elemento activo.

Al ampliar la biblioteca, conservar virtualización/carga diferida. No crear una fuente OBS permanente por tarjeta.

## Invariantes de producto

1. Seleccionar un archivo siempre reemplaza al anterior.
2. Vista previa y escenario muestran el mismo contenido.
3. Apagar escenario no detiene necesariamente la vista previa.
4. El control de tiempo solo se habilita para medios con duración válida.
5. Imágenes no exponen transporte temporal.
6. Música ignora portadas incrustadas para el reloj de reproducción.
7. Cambiar de archivo vuelve a aplicar la salida de audio sin abrir `Sonido`.
8. Toda preferencia persistida debe tolerar que el monitor, altavoz o archivo ya no exista.
