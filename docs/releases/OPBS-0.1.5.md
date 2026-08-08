# OPBS 0.1.5

OPBS 0.1.5 es la primera gran actualización del Presentador integrado. Esta versión consolida la identidad independiente
de OPBS, estrena el espacio de transmisión y mejora la estabilidad de la biblioteca, la Biblia y el audio.

## Aplicación independiente de OBS Studio

- El producto, ejecutable, ventana, metadatos de Windows, registros y mensajes visibles ahora se identifican como OPBS.
- OPBS utiliza su propia configuración bajo `opbs`, separada de una instalación normal de OBS Studio.
- Instalar, actualizar o configurar OPBS no sustituye ni modifica OBS Studio; ambas aplicaciones pueden coexistir.
- Se desactivó el actualizador binario oficial de OBS dentro de OPBS. Las versiones de OPBS se distribuyen exclusivamente
  desde los Releases de `ByMarioBz/OPBS`.
- El instalador registra OPBS como una aplicación independiente por usuario, crea accesos directos de escritorio y menú
  Inicio e incluye su propio desinstalador.
- El paquete incluye el motor OBS Studio 32.2.0 y las bibliotecas y complementos necesarios para OPBS. No es necesario
  instalar OBS Studio por separado.

## Nuevo espacio de transmisión y grabación

- Nueva opción `Transmisión` en la barra superior con las secciones Emisión, Salida, Cámaras, Audio y Lienzo de ambos.
- Vista previa independiente de transmisión debajo de la vista previa del presentador.
- Controles directos para transmitir, grabar y cambiar entre las escenas `Cámaras`, `Presentador` y `Ambos`.
- Emisión nativa a un destino principal y un segundo destino opcional con la misma composición.
- Servicios disponibles simplificados a YouTube, Facebook y RTMP personalizado.
- Configuración de tasa de bits de video, tasa de bits de audio y carpeta de grabación.
- Lienzo fijo de transmisión a 1920 × 1080, 60 FPS y relación 16:9.
- Selección de dispositivos de captura de video conectados para la escena de cámara.
- La composición `Ambos` permite ajustar por separado posición y tamaño de la cámara y del presentador.
- Fondo de `Ambos` personalizable mediante color, imagen o video, con repetición opcional.
- Move Transition 3.2.1 se integra como transición predeterminada entre las tres escenas, con fundido como respaldo.
- Todas las preferencias de transmisión se conservan entre sesiones.

## Mezclador exclusivo de audio para transmisión

- Nueva página `Transmisión > Audio`.
- El audio del presentador es una entrada fija y siempre acompaña a Cámaras, Presentador y Ambos.
- Se puede seleccionar una segunda entrada de audio entre los dispositivos disponibles de Windows.
- Mezclador nativo de OBS con medidores, silencio y ajuste independiente en decibeles para las dos entradas.
- Los niveles de transmisión y grabación no cambian el volumen escuchado por los altavoces del presentador.
- Cámara, fondos y fuentes globales heredadas quedan fuera de la mezcla para impedir audio duplicado o accidental.
- Se valida la disponibilidad de las entradas configuradas antes de comenzar una emisión o grabación.

## Interfaz y Biblia

- Nueva distribución principal: aproximadamente 31 % del ancho para las vistas previas y 69 % para la biblioteca.
- Vistas previas más compactas y biblioteca con mayor espacio de trabajo.
- La distribución puede ajustarse y queda guardada para la siguiente ejecución.
- La sección Presentación de la barra lateral se reposicionó para mantener visibles Biblia y Presentaciones.
- La ventana de configuración bíblica ahora es adaptable y desplazable.
- La vista previa del lienzo bíblico conserva correctamente su proporción 16:9 sin cubrir los controles inferiores.
- Identidad visual y textos heredados actualizados de OBS a OPBS en las zonas expuestas por esta interfaz.

## Biblioteca y archivos eliminados

- Si un archivo multimedia fue eliminado o movido fuera de OPBS, su tarjeta se retira automáticamente de la biblioteca.
- OPBS muestra un aviso propio con la lista de nombres afectados y no vuelve a mostrarlo para las mismas rutas.
- Se desactivó el reparador de archivos de la colección heredada de OBS, que no forma parte de la biblioteca OPBS.
- La limpieza modifica solamente la referencia guardada por OPBS; nunca elimina otros archivos de la computadora.

## Actualizaciones

- Las instalaciones de OPBS 0.1.0 detectarán OPBS 0.1.5 mediante el Release público de GitHub.
- El actualizador descarga `OPBS-Setup-x64.exe` y su SHA-256, verifica la integridad y solo entonces abre el instalador.
- El actualizador incluido desde 0.1.5 muestra las notas completas de cada nueva versión antes de descargarla.
- La biblioteca y preferencias existentes se conservan al instalar una versión nueva sobre OPBS.

## Descargas

- `OPBS-Setup-x64.exe`: instalador recomendado para Windows x64.
- `OPBS-Portable-x64.zip`: paquete portable para ejecutar OPBS sin instalación.
- `OPBS-Setup-x64.exe.sha256`: verificación de integridad del instalador.

OPBS deriva de OBS Studio y se distribuye bajo GPL-2.0. El código fuente modificado está disponible en este repositorio.
