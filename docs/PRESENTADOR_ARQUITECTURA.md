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
  tiempo, medidores, volumen, diálogos de pantallas/sonido/Biblia, importación de presentaciones y coordinación de
  fuentes.
- `frontend/widgets/PresentationImporter.cpp` y `.hpp`: convierte PDF con el motor nativo de Windows y PowerPoint
  mediante automatización COM; entrega imágenes PNG secuenciales al panel.
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

## Composición de transmisión

La transmisión usa tres escenas privadas coordinadas por `PresenterPanel`:

```text
OPBS Transmission Camera     --> cámara a lienzo completo
OPBS Transmission Presenter  --> Presenter Stage a lienzo completo
OPBS Transmission Both       --> fondo + cámara izquierda + presentador derecha
              |
              v
       OPBS Move Transition
              |
              +-- vista previa de transmisión
              +-- salida principal de transmisión/grabación
```

`Presenter Stage` sigue siendo la fuente de verdad del contenido multimedia, bíblico o de presentaciones. Las escenas
de transmisión solo reutilizan esa fuente y la cámara asignada para componer los tres modos; no deben crear una segunda
reproducción del contenido.

La fuente DirectShow de cámara conserva un estado `cameraEnabled` y expone en el diálogo el mismo procedimiento nativo
`activate(bool)` que usa OBS. Desactivar libera el grafo de captura; volver a activar lo construye otra vez, permitiendo
recuperar una capturadora USB después de desconectarla y conectarla. La selección se aplica en vivo y Cancelar restaura
la cámara y el estado anteriores.

El modo `Ambos` guarda en porcentajes independientes X, Y, ancho y alto para cámara y presentador. Su fondo puede ser
un `color_source`, `image_source` o `ffmpeg_source`; el valor predeterminado es negro. La transición entre `Cámaras`,
`Presentador` y `Ambos` es `move_transition`, con fundido como respaldo si el módulo no pudiera cargarse.

Move Transition 3.2.1 está fijado dentro de `plugins/move-transition` desde el commit upstream
`3be3a85100e4382dc48b1058027ef02b5d1e4fbc`. Su procedencia y licencia GPL-2.0 están documentadas en
`plugins/move-transition/OPBS_UPSTREAM.md`; no actualizarlo desde `master` sin una revisión explícita.

## Persistencia

La aplicación usa configuración portátil bajo:

```text
dist/OPBS/config/obs-studio
```

Se recuerdan biblioteca, carpetas, orden, carpeta activa, monitor, estado de salida, ajustes de sonido y geometría de
ventana. También se conservan la Biblia seleccionada, su tipografía, tamaño, alineación, posición de referencia, fondo
personalizado y repetición del fondo. Las diapositivas convertidas viven bajo la configuración portátil y sustituyen
atómicamente la importación anterior. Los registros de ejecución están bajo `config/obs-studio/logs` y son la primera
fuente para diagnosticar fallos.

Al restaurar la biblioteca, las rutas que ya no sean archivos válidos se descartan antes de reconstruir las tarjetas y
se vuelve a guardar la lista limpia. OPBS informa una sola vez los nombres eliminados; el aviso no se repite porque las
rutas ausentes dejan de formar parte de `presenter.ini`. El reparador de archivos de la colección heredada de OBS está
desactivado en este frontend: esa colección permanece oculta y no es la fuente de verdad de la biblioteca.

La geometría del divisor principal conserva la distribución aproximada 31/69 entre vistas previas y biblioteca. La
configuración de transmisión recuerda el modo activo, la duración de Move, el fondo de `Ambos` y las ocho medidas de la
composición.

No versionar esa configuración: contiene rutas locales y preferencias del usuario.

## Audio

La fuente multimedia se configura para monitorización. Al activar cada archivo se reconstruye el monitor de audio y se
restablece la monitorización después de que FFmpeg emita `media_started`. Esto reproduce automáticamente la operación
que antes solo ocurría al cambiar manualmente el dispositivo en `Sonido`.

Las fuentes globales heredadas de escritorio/micrófono se desactivan en el modo presentador para evitar competencia o
duplicación. Un dispositivo Bluetooth desconectado puede seguir siendo una preferencia guardada; Windows devolverá un
error de dispositivo invalidado y el usuario debe seleccionar una salida conectada.

La mezcla de transmisión es independiente de la monitorización local:

```text
medio activo --monitor-only--> salida local elegida en Sonido
      |
      +-- callback de audio crudo --> puente OPBS -- canal global 1 --+
entrada WASAPI elegida --------------------------- canal global 2 --+--> mezcla 0 --> emitir/grabar
escena de transmisión ---------------------------- canal global 0 --+    (video)
```

El puente copia el audio procesado del medio antes del volumen local, de modo que su control de dB y silencio pertenece
solo a la transmisión. La segunda entrada es una fuente privada `wasapi_input_capture`. Ambas permanecen en los canales
globales 1 y 2, por lo que siguen oyéndose al cambiar entre `Cámaras`, `Presentador` y `Ambos`. Cámara, fondo de video y
las entradas globales heredadas tienen sus mezcladores desactivados; por diseño no entran otras fuentes al mix 0.

La fuente DirectShow creada por OPBS lleva `opbs_disable_device_audio=true`. Este ajuste privado impide que el grafo de
video abra el pin de audio integrado de la capturadora; la entrada sonora se abre exclusivamente como WASAPI. Por eso
el dispositivo de video y el endpoint de audio pueden pertenecer al mismo equipo USB e incluso compartir nombre visible
sin competir: se identifican por IDs de subsistemas distintos y usan nombres internos OPBS diferentes.

## Rendimiento

- Las fuentes de audio/video se crean al reproducir y no para todas las tarjetas.
- Las miniaturas se solicitan de forma diferida.
- La biblioteca filtra por carpeta y nombre sin cargar medios completos.
- La escena contiene un único elemento activo.
- Los PDF se rasterizan en un flujo de trabajo explícito; no se crean fuentes OBS por cada página hasta seleccionarla.

Al ampliar la biblioteca, conservar virtualización/carga diferida. No crear una fuente OBS permanente por tarjeta.

## Dos canales de actualización

1. Las novedades del código base OBS se revisan desde `obs-public`; nunca se instalan directamente. Consultar
   `OBS_ACTUALIZACIONES.md`.
2. Las versiones terminadas de OPBS se distribuyen como GitHub Releases propios. El comprobador descarga el instalador
   y su SHA-256, valida la integridad y solo entonces abre el instalador.

La versión y el repositorio de OPBS viven en `presenter-tools/opbs-release.json`. El instalador usa NSIS, instala por
usuario en `%LOCALAPPDATA%\Programs\OPBS`, crea accesos directos y registra `Uninstall.exe`.
Las etiquetas `opbs-vX.Y.Z` están excluidas del cálculo interno de `OBS_VERSION`; el motor solo reconoce etiquetas
numéricas del proyecto upstream para evitar que ambas numeraciones interfieran.
Las compilaciones públicas de Windows mapean las rutas de fuentes y PDB a nombres neutros para no exponer el
directorio o usuario del equipo que genera el Release.
El scripting Lua/Python permanece en el código original, pero se compila desactivado mientras OPBS no lo exponga;
esto evita distribuir módulos SWIG innecesarios y metadatos de generación locales.

## Invariantes de producto

1. Seleccionar un archivo siempre reemplaza al anterior.
2. Vista previa y escenario muestran el mismo contenido.
3. Apagar escenario no detiene necesariamente la vista previa.
4. El control de tiempo solo se habilita para medios con duración válida.
5. Imágenes no exponen transporte temporal.
6. Música ignora portadas incrustadas para el reloj de reproducción.
7. Cambiar de archivo vuelve a aplicar la salida de audio sin abrir `Sonido`.
8. Toda preferencia persistida debe tolerar que el monitor, altavoz o archivo ya no exista.
