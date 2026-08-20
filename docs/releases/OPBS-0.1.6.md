# OPBS 0.1.6

OPBS 0.1.6 introduce el nuevo espacio de trabajo modular y corrige la conservación de datos durante las
actualizaciones. Esta versión continúa basada en OBS Studio 32.2.0 y mantiene OPBS separado de una instalación normal
de OBS Studio.

## Interfaz modular

- Cinco paneles redimensionables, movibles y flotantes: Presentador/Escenario, Transmisión/En vivo, Multimedia,
  Herramientas y Reproductor de audio.
- Distribución inicial aproximada 31/69 y restauración automática del acomodo elegido por el usuario.
- Distribución refinada con Multimedia en la parte superior derecha y Herramientas/Reproductor de audio en la fila
  inferior, conservando Presentador y Transmisión en la columna izquierda.
- Biblioteca multimedia con tarjetas compactas de 176 × 99 píxeles.
- Herramientas reúne Biblia, Presentación, Captura y NDI.
- Historial de hasta cuatro presentaciones importadas recientemente.
- Reproductor de audio independiente con lista persistente, reproducción/pausa, detener y línea de tiempo manipulable.
- El control de volumen fue retirado de la vista del presentador sin alterar sus medidores ni controles multimedia.
- Nueva identidad visual en negro, grafito, blanco y azul para distinguir claramente OPBS.
- Jerarquía más limpia en el espacio de trabajo: títulos sin duplicados, búsqueda multimedia superior con contador e
  iconos de reproducción de alto contraste.
- Indicadores compactos, selecciones laterales más sobrias y mejor aprovechamiento del espacio en ventanas grandes.
- Paneles de superficie continua, sin la doble barra superior heredada del aspecto de OBS.
- Cabeceras propias e iconografía monocroma original para los cinco paneles del espacio de trabajo.
- Corrección de cabeceras recortadas y nueva paleta oscura neutral con contraste calculado para texto, acciones y foco.
- Jerarquía visual más profunda con materiales opacos, buscadores con icono, selección lateral discreta y controles
  multimedia y de transmisión agrupados.
- Corrección de iconos recortados al usar escalado de pantalla de Windows y aplicación del sistema visual a las
  ventanas de Pantallas, Sonido, Transmisión y Biblia.
- Botones de Transmitir, Grabar, Cámaras, Presentador y Ambos renovados con iconografía propia y selector segmentado
  para los tres modos de composición.
- Menús contextuales en Multimedia, Reproductor de audio, Captura y NDI para agregar contenido compatible, usar nombres
  internos y quitar referencias sin borrar archivos originales.
- Repetición individual persistente para cada video o canción de Multimedia, con marca visible en su menú.
- Indicadores LIVE y REC con duración, más salud de señal independiente para YouTube y Facebook basada en la salida
  real. Se integran en la franja superior junto al control de Escenario para no reducir la vista previa. Los controles
  de transmisión y grabación distinguen mejor sus estados activos.
- Buscadores multimedia y bíblico con icono propio centrado y estable bajo escalado de Windows.
- Buscadores adaptables al ancho de sus docks y menús contextuales elevados con el lenguaje visual propio de OPBS.
- Revisión visual integral de paneles, diálogos, campos y selecciones, con menús contextuales más compactos, sombra del
  sistema, glifos vectoriales originales y movimiento breve condicionado por las preferencias visuales de Windows.
- Selector de archivos oscuro con ruta inicial válida y restauración del arrastre directo sobre carpetas multimedia
  vacías.
- Corrección del rótulo del segundo destino para distinguir Facebook de YouTube en configuraciones ya existentes.
- Multimedia y Herramientas funcionan simultáneamente con cuadrículas independientes; los medios arrastrados ya no
  pueden aparecer dentro de Presentación, Captura o NDI.
- Buscador multimedia independiente y buscador de Herramientas visible únicamente al seleccionar Biblia.
- Captura muestra la cámara configurada en Transmisión y permite agregar cámaras o ventanas del equipo desde su botón
  inferior derecho.
- La cabecera principal adopta el diseño compacto: menú superior, franja negra y control de Escenario a la derecha.
  La importación multimedia continúa disponible desde `Archivo > Importar > Multimedia`.
- Primer sistema visual propio de OPBS: estilos unificados para menús, diálogos, listas, tarjetas y controles, foco de
  teclado visible y una paleta semántica preparada para heredarse en Broadcast Presenter.

## Actualizaciones y datos

- Antes de abrir OPBS desde sus accesos directos, el nuevo lanzador consulta rápidamente si existe una versión nueva.
- El aviso muestra las notas de la versión y permite elegir `Actualizar ahora` o `Posponer`.
- Tras instalar una actualización, OPBS vuelve a abrirse automáticamente.
- La biblioteca, Biblias, presentaciones, ajustes de pantalla, sonido y transmisión se guardan en la carpeta personal
  `%APPDATA%\opbs`, separada de los binarios instalados.
- Al actualizar desde 0.1.5, el instalador migra de forma conservadora los datos que estaban dentro de la carpeta del
  programa. Los archivos existentes en la nueva ubicación nunca se sobrescriben.
- El cierre solicitado por el actualizador es ordenado para que OPBS guarde los últimos cambios antes de instalar.
- Los Releases públicos dejan de incluir el ZIP portable. El instalador de Windows x64 es el único paquete público.

## Artefactos

- `OPBS-Setup-x64.exe`: instalador y actualizador para Windows x64.
- `OPBS-Setup-x64.exe.sha256`: verificación SHA-256 obligatoria antes de ejecutar el instalador descargado.

## Licencia

OPBS es un trabajo derivado de OBS Studio y se distribuye bajo GPL-2.0. El código fuente modificado está disponible en
este repositorio.
