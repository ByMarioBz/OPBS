# Historial y estado de ingeniería

Última actualización: 21 de agosto de 2026.

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

- Parche de cámara posterior a 0.1.5: `Transmisión > Cámaras` incorpora `Activar/Desactivar` mediante el procedimiento
  nativo de DirectShow. El estado se persiste, cambiar la selección la aplica en vivo y Cancelar restaura la anterior.
- Separación USB de video/audio: la fuente de cámara OPBS no abre el pin de audio DirectShow; la segunda entrada conserva
  su endpoint WASAPI y un nombre interno independiente. Esto permite seleccionar video y audio del mismo dispositivo
  físico aunque Windows muestre el mismo nombre en ambas listas.
- Verificación del parche: `Build-Presenter.cmd` y `Package-Presenter.cmd` terminaron correctamente y la copia empaquetada
  arrancó como OPBS. El registro `2026-08-09 19-00-40.txt` confirmó simultáneamente `video device audio disabled by
  OPBS`, la creación de `OPBS Camera` y la inicialización WASAPI de `OPBS Audio - Micrófono (HyperX SoloCast)`; cámara y
  entrada adicional quedaron abiertas por rutas independientes.

- Interfaz principal 0.1.5: el divisor horizontal usa por defecto aproximadamente 31 % para las dos vistas previas y
  69 % para la biblioteca, conserva el ajuste del usuario y migra distribuciones anteriores mediante la versión 5 del
  layout.
- Transmisión 0.1.5: se separaron las escenas privadas `Cámaras`, `Presentador` y `Ambos`. La composición predeterminada
  de `Ambos` coloca una cámara menor a la izquierda y el presentador mayor a la derecha sobre fondo negro.
- Lienzo de ambos: el diálogo de transmisión permite editar X, Y, ancho y alto de ambas tomas y seleccionar fondo de
  color, imagen o video con repetición opcional. Todos los valores quedan persistidos en la configuración portable.
- Move Transition: se integró la versión 3.2.1 fijada al commit upstream
  `3be3a85100e4382dc48b1058027ef02b5d1e4fbc` y se usa por defecto al cambiar entre los tres modos de transmisión. La
  compilación RelWithDebInfo incluyó el objetivo `move-transition`; el paquete se regeneró y el proceso portable cargó
  `dist/OPBS/obs-plugins/64bit/move-transition.dll` correctamente.
- Biblioteca y archivos externos: al iniciar, OPBS descarta y deja de persistir las rutas cuyos archivos fueron
  borrados fuera de la aplicación. Muestra un aviso propio con los nombres afectados una sola vez y mantiene desactivado
  el reparador de la colección heredada de OBS.
- Audio de transmisión: se añadió la página `Audio` con el presentador fijo, una entrada WASAPI seleccionable y dos
  controles nativos `VolumeControl`. El medio original permanece en `monitor-only`; un puente independiente y la
  entrada seleccionada ocupan exclusivamente los canales globales 1 y 2 del mix 0. Cámara y fondos no aportan audio.
- Revisión del flujo de salida: el lienzo OPBS se fuerza a 1920 × 1080 y 60 FPS desde la configuración del perfil; el
  destino principal usa el flujo `SimpleOutput` de OBS y el secundario reutiliza sus codificadores en otro
  `rtmp_output`. YouTube y Facebook coinciden con los servicios de `rtmp-services`; `Personalizado` usa `rtmp_custom`.
  La compilación, el empaquetado y el inicio portable finalizaron correctamente. Falta una prueba real con claves
  privadas para certificar la conexión de extremo a extremo y una grabación con dos medios consecutivos para escuchar
  físicamente la separación de mezclas.
- Release 0.1.5: las notas públicas se mantienen en `docs/releases/OPBS-0.1.5.md`; el publicador las usa como cuerpo del
  Release y agrega automáticamente los SHA-256 del instalador y del ZIP portable. El actualizador incluido a partir de
  0.1.5 muestra el cuerpo del Release en una ventana desplazable antes de descargar, conserva la validación SHA-256 y
  ejecuta el instalador solamente después de verificarlo.

- OPBS 0.1.5: la identidad visible de la ventana, los textos traducidos y los metadatos del ejecutable de Windows usan
  `OPBS`; `ProductName`, `ProductVersion`, `FileVersion` y `OriginalFilename` se verificaron como `OPBS`, `0.1.5`,
  `0.1.5` y `OPBS.exe` respectivamente.
- Configuración bíblica 0.1.5: el contenido vive dentro de un área desplazable, la vista previa mantiene una proporción
  16:9 y los botones Guardar/Cancelar permanecen visibles en una ventana de 776 × 859 píxeles.
- Barra lateral 0.1.5: la zona multimedia y el espacio inferior usan una proporción adaptable 65/35. En una ventana de
  1009 píxeles de alto, `Presentación` quedó en Y=667, seguida por `Biblia` y `Presentaciones` sin recortes.

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
- Frontend Tools: `forms/scripts.ui` solo participa cuando `ENABLE_SCRIPTING` está activo; esto evita símbolos MOC
  huérfanos al compilar OPBS sin Lua/Python.
- Residuos incrementales: `Create-Portable.ps1` elimina cualquier módulo Lua/Python conservado por una compilación
  anterior y el generador del instalador cancela la entrega si detecta alguno.
- Auditoría de documentación: el Release público `OPBS 0.1.0` no incluye traducciones bíblicas. Se detectaron
  referencias antiguas que afirmaban que el portable copiaba Biblias locales; deben considerarse obsoletas y
  corregirse para reflejar que cada usuario importa sus propios archivos TXT compatibles.
- La selección guardada `Auriculares (BT3280)` devolvió desde Windows el error `88890004`, correspondiente a un
  dispositivo invalidado/no disponible durante esa sesión. No confundirlo con un fallo del mezclador.

## 2026-08-12 - Rediseño modular en desarrollo

- Se sustituyó el divisor fijo por cinco paneles acoplables y flotantes con distribución inicial 31/69.
- Multimedia conserva carpetas, búsqueda y arrastrar/soltar con tarjetas compactas de 176 × 99 píxeles.
- Herramientas reúne Biblia, Presentación, Captura y NDI, y recuerda hasta cuatro importaciones de presentaciones.
- Se añadió un reproductor de audio independiente con lista persistente por arrastre, línea de tiempo y controles de
  reproducción/pausa y parada.
- La vista del presentador ya no muestra el deslizador de volumen; conserva transporte, tiempo y medidores.
- `Build-Presenter.cmd`/compilación incremental y `Package-Presenter.cmd` terminaron correctamente. La inspección visual
  confirmó la cuadrícula completa, las cinco barras de panel y el ciclo Herramientas acoplado -> flotante -> acoplado.
- Se cargó temporalmente `C:\Windows\Media\Alarm01.wav`: al seleccionar la pista comenzó la reproducción, cambió el
  botón a pausa, avanzó la línea de tiempo y `Detener` la devolvió al inicio. La referencia de prueba se retiró después.
- Se cerró y volvió a abrir OPBS; la cuadrícula 31/69, los cinco paneles acoplados y la selección Multimedia/General se
  restauraron correctamente. Queda pendiente probar el gesto de arrastrar desde el Explorador con una biblioteca real.

## 2026-08-12 - Actualización 0.1.5 a 0.1.6

- Los accesos directos del instalador apuntan a `OPBS-Launcher.ps1`, que comprueba actualizaciones antes de iniciar la
  aplicación y ofrece `Actualizar ahora` o `Posponer`. La comprobación automática tardía dentro de la ventana principal
  fue retirada para evitar dos avisos distintos.
- `OPBS-MigrateData.ps1` mueve de forma conservadora la configuración heredada desde la instalación hacia
  `%APPDATA%\opbs`; no reemplaza archivos que ya existan en el perfil del usuario.
- El instalador 0.1.6 vuelve a abrir OPBS automáticamente mediante el lanzador y los Releases públicos ya no generan ni
  publican el ZIP portable. El paquete local de desarrollo se conserva para las pruebas internas exigidas por el flujo.
- Se construyó `release\0.1.6\OPBS-Setup-x64.exe`. La carpeta de Release contiene únicamente el instalador y su archivo
  SHA-256; la auditoría del payload no encontró configuración personal, símbolos ni marcadores de modo portable.
- Prueba aislada real: se instaló el ejecutable público 0.1.5 en una ruta temporal, se añadieron una configuración, una
  Biblia y una diapositiva heredadas, y se ejecutó la actualización con el instalador 0.1.6 recién construido. El binario
  cambió de `OPBS 0.1.5` a `OPBS 0.1.6`, la aplicación volvió a abrirse automáticamente y los tres archivos migrados
  conservaron exactamente su SHA-256. La instalación temporal y sus accesos directos se retiraron al terminar.
- Esta fue una prueba local del instalador completo, no una descarga desde GitHub: la publicación de `opbs-v0.1.6`
  sigue requiriendo autorización explícita separada.

## 2026-08-12 - Identidad visual y Captura

- La interfaz adopta una paleta propia de negro, grafito, blanco y azul, con separadores negros, paneles más planos y
  azul reservado para acciones y estados activos. La inspección visual a 1920 × 1032 confirmó la distribución completa.
- Multimedia y Herramientas dejaron de compartir físicamente la cuadrícula. La búsqueda multimedia permanece visible
  en su panel al seleccionar Presentación o Captura, mientras Herramientas solo muestra buscador en Biblia.
- Se eliminó la aceptación de archivos en la lista lateral de Herramientas. Las rutas externas que anteriormente
  terminaron en secciones reservadas se devuelven a `General`; las diapositivas generadas por OPBS permanecen separadas.
- Captura muestra la cámara elegida en Transmisión y añade un botón inferior derecho con `Dispositivo de captura de
  video…` y `Captura de ventana…`. Las fuentes adicionales se conservan en la configuración y pueden enviarse al
  escenario al seleccionar su tarjeta.
- `Build-Presenter.cmd` terminó correctamente. En la prueba visual se abrió Captura, se verificó el botón y se enumeró
  correctamente una ventana disponible; el selector se canceló para no guardar una fuente de ensayo.
- `Package-Presenter.cmd`, el arranque final en modo seguro y la regeneración de `OPBS-Setup-x64.exe` 0.1.6 terminaron
  correctamente después de la revisión visual.

## 2026-08-14 - Cabecera compacta

- Se retiraron de la cabecera interna el nombre, la versión, el subtítulo y el botón grande de importación para evitar
  duplicar información de la ventana y acercar el área superior al diseño visual de producto.
- La cabecera ahora es una franja negra de 44 píxeles con `ESCENARIO` y un interruptor compacto `ON/OFF` a la derecha.
  La importación multimedia permanece disponible en `Archivo > Importar > Multimedia`.
- `Build-Presenter.cmd` y `Package-Presenter.cmd` terminaron correctamente; la aplicación empaquetada inició en modo
  seguro y su registro alcanzó `Startup complete` con la nueva interfaz cargada.

## 2026-08-19 - Sistema visual propio de OPBS

- Se creó `OPBS_DESIGN_SYSTEM.md` como especificación original del producto para color semántico, tipografía, espacio,
  geometría, estados interactivos, accesibilidad y movimiento. Las referencias externas quedan fuera del repositorio y
  del instalador; OPBS conserva comportamiento de Windows e identidad propia.
- `OpbsDesignSystem` centraliza la hoja de estilo compartida de la ventana, menús, diálogos, paneles, listas, tarjetas,
  formularios, barras de desplazamiento y controles del presentador.
- Se añadieron estados visibles de `hover`, pulsación, foco por teclado, selección, deshabilitado y salida en vivo, junto
  con nombres accesibles para búsquedas, líneas de tiempo, transporte, carpetas, capturas y listas principales.
- La segunda iteración cambia también la jerarquía: elimina encabezados internos duplicados, usa nombres de panel en
  estilo oración, coloca la búsqueda de Multimedia arriba junto a un contador compacto y diferencia las vistas previas
  mediante rótulos con acento azul. Los iconos estándar de transporte y listas se recolorean para mantener contraste
  sobre el tema oscuro.
- La tercera iteración reduce el uso ornamental del azul: los estados de vista previa pasan a cápsulas compactas, las
  selecciones laterales usan un tinte moderado y desaparecen los contornos de foco alrededor de listas completas. Los
  buscadores mantienen un ancho de lectura limitado en ventanas grandes, y el reproductor de audio explica su destino
  de arrastre cuando está vacío.
- La cuarta iteración retira la apariencia heredada de barras acoplables apiladas: cada panel se dibuja como una sola
  superficie redondeada, el título deja de tener caja y borde independientes y el control visual de desacoplar se
  oculta. El título conserva el arrastre y el doble clic para mover o convertir el panel en ventana.
- La quinta iteración reemplaza por completo el encabezado nativo visible de `QDockWidget` por una cabecera propia de
  OPBS. Cinco glifos vectoriales dibujados en tiempo de ejecución identifican Escenario, Multimedia, En vivo,
  Herramientas y Audio sin incorporar activos externos. Los eventos no consumidos se propagan al dock para conservar
  el comportamiento de movimiento y doble clic documentado por Qt.
- La sexta iteración corrige el recorte observado en esas cabeceras declarando una altura, política y tamaño sugerido
  estables al sistema de acoplamiento. La paleta abandona los negros azulados por seis niveles de gris neutral, con
  azul separado entre acción rellena y foco; los contrastes documentados superan `4.5:1` para texto normal.
- La séptima iteración adopta con mayor intensidad los principios auditados de jerarquía y materiales: cabeceras y barra
  superior usan capas opacas sutiles, los buscadores incorporan un glifo propio, la selección lateral reduce el relleno
  azul a un indicador de 3 px y los transportes/modos de transmisión se agrupan en controles continuos. Se ampliaron
  márgenes, radios y blancos internos sin introducir desenfoque sobre video ni copiar activos de otra plataforma.
- La octava iteración corrige el doble escalado que recortaba los iconos vectoriales en cabeceras y buscadores con DPI
  alto. Pantallas, Sonido, Transmisión y Biblia comparten ahora títulos, subtítulos, superficies y navegación lateral
  del sistema visual de OPBS. Los controles de emisión incorporan iconos originales y los modos Cámaras, Presentador
  y Ambos conservan un selector segmentado más claro sin alterar su comportamiento.
- La novena iteración estabiliza buscadores en paneles estrechos, unifica controles compactos y reduce los radios que
  producían esquinas negras exageradas. Añade menús contextuales separados para Multimedia, audio, Captura y NDI con
  importación restringida, alias persistentes y eliminación solo de OPBS. El bucle pasa a ser una propiedad persistente
  de cada medio. En vivo incorpora estados LIVE/REC, duración y salud real por destino basada en congestión y pérdida
  de fotogramas. Esta identidad queda declarada como base visual compartida con Broadcast Presenter.
- La décima iteración mueve toda la telemetría LIVE/REC y de destinos a la franja superior, a la izquierda del control
  de Escenario, recuperando el espacio vertical completo de la vista previa de transmisión. Los dos buscadores dejan de
  usar acciones incrustadas de Qt y dibujan su lupa centrada según la altura efectiva del campo para corregir su
  desplazamiento con DPI y paneles estrechos.
- La undécima iteración adopta el acomodo solicitado con Herramientas arriba y Multimedia/Audio abajo a la derecha,
  mediante un estado versionado de docks. El estado vacío de Multimedia vuelve a aceptar archivos arrastrados. El
  selector de archivos deja de depender del diálogo claro y de la última ruta inválida de Windows; ahora usa una vista
  Qt oscura con una carpeta inicial existente. También repara el nombre del segundo servicio a partir de su servidor
  RTMP para diferenciar Facebook de YouTube en configuraciones anteriores.
- La versión 16 del estado corrige una regresión del primer acomodo: cada dock se inserta directamente al construir el
  árbol de dos columnas, en lugar de añadir los cinco al mismo sector antes de dividirlos. Presentador/Transmisión quedan
  en la izquierda y Herramientas sobre Multimedia/Audio en la derecha sin agrupaciones intermedias de Qt.
- La versión 17 intercambia las secciones derechas según la decisión final: Multimedia queda arriba y Herramientas/Audio
  abajo. Los buscadores eliminan anchos máximos rígidos, reservan correctamente el espacio de lupa y borrado, y se
  expanden con el dock. Los menús contextuales adoptan una superficie elevada oscura, radios, espaciado e iconografía
  monocroma propios de OPBS; el bucle conserva su marca al extremo derecho.
- La duodécima iteración profundiza el lenguaje visual en toda la aplicación: eleva la escala mínima de controles,
  unifica radios y márgenes, suaviza la jerarquía de superficies y amplía el aire entre grupos. Los menús contextuales
  dejan de reutilizar pictogramas rellenos de Windows y dibujan glifos vectoriales propios de 16 px; recuperan la sombra
  nativa, reducen sus filas a 32 px, separan la acción destructiva y aparecen con una transición breve que respeta los
  efectos configurados en el sistema. Las animaciones de acoplamiento siguen la misma preferencia de Windows.
- `Build-Presenter.cmd` y `Package-Presenter.cmd` terminaron correctamente. OPBS inició en modo seguro, permaneció
  respondiendo y alcanzó `Startup complete` después de las nueve iteraciones; el registro no contiene errores de análisis
  de QSS. La herramienta de
  control visual bloqueó `OPBS.exe` por política, por lo que la inspección visual automatizada queda sustituida por una
  revisión manual de la ventana abierta.

## Prueba pendiente inmediata

La línea de tiempo de música quedó verificada. Sigue pendiente comprobar el audio físico con una salida conectada:
elegirla una vez, cambiar entre video y dos canciones sin volver a abrir `Sonido`, y confirmar sonido en cada cambio.

## 2026-08-21 - Rendimiento adaptativo

- Se retiró el coste fijo de 1920 × 1080 a 60 FPS. El lienzo del escenario permanece en 1080p, pero la salida se
  configura como 480p30, 720p30, 1080p30 o 1080p60 según memoria y procesadores lógicos.
- OPBS elige automáticamente NVENC, Quick Sync o AMF H.264 cuando están disponibles; x264 conserva presets ligeros como
  respaldo.
- Se creó un controlador reutilizable que mide CPU, presión de memoria, tiempo de render, pérdida de render/codificación
  y congestión/pérdida de cada RTMP. En presión local sostenida pausa tareas visuales duplicadas sin tocar escenario,
  audio, grabación o transmisión.
- Las imágenes visibles se decodifican en el grupo de trabajo de Qt y regresan al hilo de interfaz como miniaturas; la
  concurrencia y la precarga dependen del perfil para evitar bloqueos al abrir carpetas grandes.
- La doble transmisión desactiva los controladores RTMP independientes y usa un solo coordinador sobre el codificador
  compartido. El bitrate baja rápido, sube con histéresis y considera el peor destino.
- Después de 60 segundos de pérdida grave en el bitrate mínimo, una emisión sin grabación baja 1080p → 720p → 480p con
  reconexión automática y máximo de dos descensos por sesión.
- `Build-Presenter.cmd` y `Package-Presenter.cmd` terminaron correctamente. El arranque seguro detectó el perfil
  `balanced`, configuró 1920 × 1080 a 30 FPS, eligió NVENC, alcanzó `Startup complete`, permaneció respondiendo y usó
  aproximadamente 206 MB de memoria residente. La prueba real de adaptación RTMP sigue pendiente porque requiere
  claves privadas y condiciones de red controladas.

## 2026-08-21 - Estados de transmisión estables

- La franja superior reserva dimensiones fijas para LIVE, REC, YouTube y Facebook. Los cambios de texto, contador y
  calidad de señal ya no recalculan el ancho de la cabecera ni desplazan los docks de las vistas previas.
- Los botones Transmitir y Grabar también conservan un ancho estable entre sus estados inactivo y activo.
- El reloj de estado consulta cada segundo la actividad real de las salidas y vuelve a sincronizar ambos botones. Así,
  una transmisión o grabación que finalice por desconexión, error o ausencia de una notificación vuelve a mostrar
  `Transmitir` o `Grabar` sin quedar visualmente activa.
- `Build-Presenter.cmd` y `Package-Presenter.cmd` terminaron correctamente. Tras retirar a una copia recuperable los
  marcadores temporales dejados por cierres forzados de pruebas anteriores, OPBS 0.1.6 inició en modo seguro y alcanzó
  `Startup complete`. La finalización real de RTMP/REC sigue requiriendo una prueba con credenciales y destino válidos.

## 2026-08-21 - Miniaturas estáticas y listas ligeras

- Los videos de Multimedia conservan el primer fotograma válido como miniatura y desconectan inmediatamente el
  observador de previsualización. La tarjeta ya no cambia durante la reproducción ni mantiene renderizado periódico.
- Captura deja de generar miniaturas de cámara o ventana. La cámara configurada y las fuentes añadidas aparecen como
  filas compactas con su nombre, conservando selección, menú contextual y activación al hacer clic.
- Se corrigió una regresión de esa conversión: `QListView::ListMode` no acepta una cuadrícula de ancho cero como forma
  de expresar «usar todo el ancho». Al retirar ese tamaño, Qt vuelve a calcular filas visibles que ocupan el panel.
- Los buscadores de Multimedia y Biblia usan la misma altura fija, alineación vertical y geometría real del glifo. Se
  redujo el margen duplicado entre lupa y texto y se evitó el desplazamiento al recibir foco.
- `Build-Presenter.cmd` y `Package-Presenter.cmd` terminaron correctamente. OPBS 0.1.6 inició en modo seguro, permaneció
  respondiendo y alcanzó `Startup complete` sin errores de compilación ni análisis de estilos.

## 2026-08-22 - Preparación final de OPBS 0.1.6

- Se completó la compilación de entrega, el empaquetado y la generación del instalador público de OPBS 0.1.6. Las
  validaciones del instalador confirmaron que no contiene configuración personal, medios, símbolos de depuración ni
  módulos de scripting desactivados.
- El cálculo SHA-256 de compilación, publicación y actualización deja de depender de `Get-FileHash` y usa directamente
  la implementación criptográfica de .NET. Esto permite crear y verificar actualizaciones incluso en entornos de
  PowerShell donde ese comando no está registrado.
- El SHA-256 calculado directamente sobre `OPBS-Setup-x64.exe` resultó idéntico al archivo de checksum generado.
- El paquete final inició como OPBS 0.1.6 en modo seguro, permaneció respondiendo y alcanzó `Startup complete`. Los avisos
  de AJA/DeckLink sin hardware y una ruta multimedia antigua inexistente son opcionales o datos del perfil de prueba y
  no impidieron el arranque.

## 2026-08-27 - Inicio de OPBS 0.1.7 e identidad instalada

- La versión de desarrollo avanza a `0.1.7` sin cambiar el repositorio `ByMarioBz/OPBS`, las etiquetas `opbs-v`, los
  artefactos `OPBS-Setup-x64.exe`, el directorio técnico de instalación ni los datos existentes.
- La identidad visible instalada pasa a llamarse `Presenter Broadcast Studio`: título de ventana, nombre mostrado por
  Windows, instalador, accesos directos y mensajes del actualizador. Así se diferencia de OBS sin romper el canal de
  actualizaciones de las instalaciones 0.1.6.
- El instalador retira durante la actualización los accesos directos antiguos llamados `OPBS` y crea los nuevos sin
  modificar la biblioteca ni las preferencias.
- El ejecutable instalado pasa a llamarse `Presenter Broadcast Studio.exe`. El instalador elimina el antiguo
  `OPBS.exe` después de conservar los datos, y el lanzador y actualizador aceptan temporalmente ambos nombres para
  completar con seguridad una actualización desde 0.1.6 o anterior.
- Se inició un concepto de icono original de cámara con cuerpo de cristal y fondo de gradiente difuminado. El archivo
  maestro de Blender y la copia editable de Affinity permanecen como propuesta local hasta su aprobación visual; el
  icono distribuido todavía no se reemplaza.
- `Build-Presenter.cmd`, `Package-Presenter.cmd` y `Build-OPBS-Installer.cmd` terminaron correctamente después del
  cambio final de nombre. El paquete contiene únicamente `Presenter Broadcast Studio.exe`: sus metadatos visibles son
  `Presenter Broadcast Studio`, su `OriginalFilename` coincide con el nombre instalado y su `InternalName` permanece
  como `OPBS`. La aplicación abrió como `Versión 0.1.7 - Presenter Broadcast Studio` y permaneció respondiendo.
- La prueba local del actualizador reconoció una instalación `0.1.6`, encontró `0.1.7` y resolvió correctamente el
  instalador `OPBS-Setup-x64.exe`. El payload no contiene el antiguo `OPBS.exe`; no se creó etiqueta ni Release público.
- Se corrigió la cámara que quedaba negra o congelada después de guardar Transmisión. Guardar con el mismo dispositivo
  ya no destruye ni vuelve a activar la fuente DirectShow que estaba entregando imagen.
- Transmisión y Herramientas comparten una sola instancia cuando apuntan al mismo identificador de cámara. Las entradas
  duplicadas guardadas por versiones anteriores se liberan antes de abrir la fuente compartida y, si esa cámara estaba
  en el escenario del presentador, se vuelve a conectar a la nueva instancia sin exigir reiniciar la aplicación.
- La corrección compiló y se empaquetó correctamente; Presenter Broadcast Studio 0.1.7 alcanzó `Startup complete` y
  permaneció respondiendo. La validación definitiva de negociación DirectShow queda pendiente en la ATEM Mini del
  entorno de producción, ya que el equipo de desarrollo no reproduce sus restricciones exclusivas de hardware.
- La importación de PDF o PowerPoint pasa a ser no invasiva: agrega el archivo convertido a `Presentaciones recientes`
  sin seleccionar la presentación nueva, reconstruir las diapositivas visibles, cambiar de herramienta ni alterar el
  medio activo. Al aplicar el límite de cuatro recientes se protege el directorio de la presentación actualmente
  cargada para que sus diapositivas sigan disponibles durante una presentación en vivo.
- `Build-Presenter.cmd` y `Package-Presenter.cmd` terminaron correctamente con esta corrección. La copia empaquetada de
  Presenter Broadcast Studio 0.1.7 inició en modo seguro, permaneció respondiendo y registró `Startup complete`.
- El relevo del escenario inserta la nueva fuente antes de retirar la anterior y conserva temporalmente el último
  fotograma válido. Las diapositivas, imágenes, videos y capturas dejan así de mostrar el destello negro causado por una
  escena vacía entre ambas activaciones.
- La cuadrícula de Presentación incorpora navegación exclusiva con `←` y `→`, sin bucle y sin atajos globales que puedan
  interferir con buscadores, formularios, Multimedia, Captura o NDI.
- La compilación, el empaquetado y el arranque seguro terminaron correctamente con el relevo nuevo. Presenter Broadcast
  Studio 0.1.7 permaneció respondiendo y el registro alcanzó `Startup complete`.
- El inicio instalado deja de ejecutar PowerShell directamente desde los accesos directos. Un envoltorio de Windows
  Script Host inicia el lanzador sin consola visible, tanto desde Escritorio y menú Inicio como después de instalar.
- La consulta de metadatos del Release usa un cliente HTTP ligero con un límite estricto de dos segundos y carga los
  componentes gráficos solo cuando debe mostrar una ventana. La consulta real a GitHub terminó en aproximadamente
  0.76 segundos; una respuesta local retenida respetó el límite configurado y permitió continuar sin bloquear el inicio.
- La detección local simulada conservó correctamente el recorrido 0.1.6 -> 0.1.7 y resolvió el instalador y su checksum.
- `Build-Presenter.cmd`, `Package-Presenter.cmd` y `Build-OPBS-Installer.cmd` terminaron correctamente. El paquete se
  abrió mediante `wscript.exe` y el nuevo lanzador en aproximadamente 1.14 segundos, permaneció respondiendo y alcanzó
  `Startup complete`; la prueba final separada con `Run-Presenter.cmd -SafeMode` obtuvo el mismo resultado.
- La interfaz principal de 0.1.7 queda fijada como base visual. La lógica de transmisión separa ahora dos recorridos:
  Cámaras ↔ Presentador usa Corte o Desvanecimiento con duración configurable, mientras todo cambio que incluye Ambos
  conserva Move Transition y su duración independiente.
- La configuración de Move salió de `Lienzo de ambos` y vive en la nueva página `Transiciones`, junto al selector del
  cambio directo. Ambos efectos se sincronizan con la escena visible antes de intercambiar la fuente de salida para no
  introducir un corte o fotograma negro entre ellos.
- `Build-Presenter.cmd` y `Package-Presenter.cmd` terminaron correctamente. La revisión visual confirmó la página
  independiente con Desvanecimiento 250 ms y Move 600 ms, y ejecutó sin bloqueo los recorridos Presentador → Cámaras,
  Cámaras → Ambos y Ambos → Presentador. La aplicación alcanzó `Startup complete` y quedó abierta para prueba manual.

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
