# Historial y estado de ingeniería

Última actualización: 25 de julio de 2026.

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
  marca `audio_only`, excluye video del decodificador y se reproduce en flujo normal en lugar del caché de cuadros.
- El control temporal aplica el salto durante el movimiento y lo repite brevemente al soltar, evitando depender de un
  único evento del ratón. Media-playback conserva además un salto que coincida con un reinicio de la fuente.

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
- Se reprodujo el fallo de MP3 con portada y se verificó la corrección final: salto hacia adelante de `0:10` a `2:27`,
  avance hasta `2:38`, salto hacia atrás a `0:59` y avance posterior hasta `1:11`, sin regresar al inicio.
- Repetición individual: una canción situada en `3:13 / 3:15` volvió al inicio y continuó hasta `0:10` con el botón
  activado; al desactivarlo, el mismo final detuvo el medio y regresó a `0:00`.
- Interfaz: `Editar` aparece antes de `Pantallas`, su opción de ajuste 16:9 puede marcarse y desmarcarse, y el control de
  repetición queda deshabilitado para imágenes.
- Biblioteca: las carpetas existentes aparecen bajo `Multimedia`; se comprobaron las selecciones independientes y
  vacías de `Presentación > Biblia` y `Presentación > Presentaciones`, incluida la restauración de la sección elegida
  después de cerrar y volver a abrir la aplicación.
- Biblia: el archivo Reina Valera 1960 cargó `31,104` registros. La consulta `Apocalipsis 22:20` y la consulta por
  contenido `Ciertamente vengo en breve` devolvieron la misma tarjeta con la referencia y el texto correctos. También
  se comprobó la lista desplegable de traducciones.
- Proyección bíblica: al seleccionar `Apocalipsis 22:20`, la vista previa mostró fondo negro, texto blanco centrado,
  versículo de mayor tamaño y referencia menor.
- Biblioteca multimedia: el clic derecho sobre una imagen mostró únicamente `Eliminar`; la prueba cerró el menú sin
  quitar elementos. La implementación elimina la referencia persistida y conserva el archivo físico.
- Portabilidad: `portable/bin/64bit/Presentador.exe` inició correctamente con Reina Valera 1960 incluida.
- Configuración bíblica: el nuevo menú `Biblia` abrió su importador y editor 16:9. Un TXT sin etiquetas mostró
  `Biblia incompatible`; Reina Valera 1960 fue aceptada, cargó 31,104 versículos y quedó seleccionable en la biblioteca.
  La vista previa reaccionó al cambiar el tamaño de 96 a 120 y la referencia de centro inferior a izquierda superior;
  al guardar, `Apocalipsis 22:20` se renderizó con esos mismos ajustes en la escena real.
- Fondo bíblico: el editor acepta imagen o video, conserva la ruta y el estado de repetición y mantiene el último
  fotograma de un video sin bucle mediante `clear_on_media_end=false`. El fondo se ajusta para cubrir el lienzo 16:9.
- Presentaciones: `Archivo > Importar > PDF` convirtió un PDF de prueba de tres páginas en tarjetas `1`, `2`, `3`.
  Se seleccionó la tarjeta `2` y la misma imagen apareció en la vista previa. La primera prueba reveló una ruta WinRT
  inválida; se cambió a carga del PDF mediante flujo de memoria y la repetición de la prueba terminó correctamente.
- PowerPoint: el importador y su error guiado se compilaron, pero la conversión real queda pendiente en un equipo con
  Microsoft PowerPoint instalado.
- Entrega: `Build-Presenter.cmd`, `Package-Presenter.cmd` y `Run-Presenter.cmd -SafeMode` terminaron correctamente; la
  aplicación respondió al iniciar y `Create-Portable.cmd` regeneró `portable/bin/64bit/Presentador.exe`.
- Actualizaciones de OBS: se deshabilitó el instalador binario oficial dentro de Presentador y se añadió una revisión
  selectiva por versión, archivo y área protegida. OBS 32.2.1 quedó revisado con cero conflictos y aplazado porque su
  única corrección funcional respecto de 32.2.0 afecta la captura de juegos, todavía oculta en este producto.
  La compilación, el empaquetado, el inicio en modo seguro y la regeneración de `portable` terminaron correctamente;
  se comprobó el marcador `disable_updater.txt` en los dos paquetes y la aplicación respondió al iniciar.
- Identidad OPBS: se creó la versión propia `0.1.0`, se mostró en título y cabecera, y se añadió
  `Ayuda > Buscar actualizaciones de OPBS`. La comprobación usa GitHub Releases del repositorio propio y exige validar
  el SHA-256 antes de ejecutar `OPBS-Setup-x64.exe`; el repositorio queda pendiente de configurar.
- Distribución: NSIS 3.12 generó el instalador, el ZIP portable y su SHA-256. El instalador creó `OPBS.exe`,
  `Uninstall.exe`, un acceso directo de escritorio y accesos del menú Inicio en una instalación temporal.
- Desinstalación: la prueba silenciosa eliminó ejecutables, complementos y accesos directos, conservó la configuración
  del usuario y no dejó el desinstalador ni el directorio del menú Inicio.
- Actualización OPBS: la comprobación automática silenciosa se ejecutó al iniciar sin mostrar errores cuando el
  repositorio aún no está configurado; la comprobación manual conserva mensajes claros para orientar la configuración.
- Repositorio OPBS: se creó el repositorio público `ByMarioBz/OPBS`, se configuró como `origin` y se incorporó su
  dirección al manifiesto de actualización de OPBS 0.1.0.
- Privacidad de Releases: el empaquetador elimina símbolos de depuración y rechaza cualquier configuración o dato
  local dentro del portable. Las Biblias de prueba ya no se copian a los paquetes destinados a publicación.
- Versiones independientes: las etiquetas `opbs-vX.Y.Z` se excluyen del cálculo de versión de OBS para mantener
  separadas la versión del producto y la versión del motor upstream.
- Empaquetado Release: el filtrado de símbolos usa la extensión real de cada archivo para evitar el comportamiento
  ambiguo de `-Include` con `-LiteralPath` en Windows PowerShell.
- Rutas privadas: MSVC usa `/pathmap` y `/PDBALTPATH` para que los binarios públicos no incorporen el nombre de usuario
  ni el directorio local de compilación.
- MSVC 18 requiere `/experimental:deterministic` para aplicar `/pathmap`; las opciones se definen antes de separar las
  compilaciones x64 y x86 para cubrir también los módulos auxiliares de 32 bits.
- Scripting: Lua/Python se conserva en el árbol de OBS, pero `Build-Presenter.ps1` lo desactiva mientras OPBS no lo
  utilice, evitando que los wrappers SWIG incorporen rutas locales en la entrega.
- La selección guardada `Auriculares (BT3280)` devolvió desde Windows el error `88890004`, correspondiente a un
  dispositivo invalidado/no disponible durante esa sesión. No confundirlo con un fallo del mezclador.

## Prueba pendiente inmediata

La línea de tiempo de música quedó verificada. Sigue pendiente comprobar el audio físico con una salida conectada:
elegirla una vez, cambiar entre video y dos canciones sin volver a abrir `Sonido`, y confirmar sonido en cada cambio.

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

Los logs portátiles se encuentran en `dist/OPBS/config/obs-studio/logs`. Para audio buscar
`Audio monitoring device`, `audio_monitor` y errores WASAPI; para medios buscar el nombre de la fuente y FFmpeg.
