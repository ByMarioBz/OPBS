# OPBS 0.1.6

OPBS 0.1.6 introduce el nuevo espacio de trabajo modular y corrige la conservación de datos durante las
actualizaciones. Esta versión continúa basada en OBS Studio 32.2.0 y mantiene OPBS separado de una instalación normal
de OBS Studio.

## Interfaz modular

- Cinco paneles redimensionables, movibles y flotantes: Presentador/Escenario, Transmisión/En vivo, Multimedia,
  Herramientas y Reproductor de audio.
- Distribución inicial aproximada 31/69 y restauración automática del acomodo elegido por el usuario.
- Biblioteca multimedia con tarjetas compactas de 176 × 99 píxeles.
- Herramientas reúne Biblia, Presentación, Captura y NDI.
- Historial de hasta cuatro presentaciones importadas recientemente.
- Reproductor de audio independiente con lista persistente, reproducción/pausa, detener y línea de tiempo manipulable.
- El control de volumen fue retirado de la vista del presentador sin alterar sus medidores ni controles multimedia.

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
