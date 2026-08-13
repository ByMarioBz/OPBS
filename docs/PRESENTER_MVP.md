# Presentador multimedia

OPBS es una aplicación de escritorio para organizar y proyectar imágenes, videos y audio, construida sobre OBS Studio
32.2.0. La `P` identifica el Presentador integrado. La versión actual de desarrollo es `0.1.5`.
El frontend clásico de OBS está temporalmente oculto, pero su implementación permanece disponible para recuperar e
integrar funciones de forma gradual.

## Estado actual

- Espacio de trabajo modular con cinco paneles acoplables, flotantes y redimensionables: `Presentador / escenario`,
  `Transmisión / en vivo`, `Multimedia`, `Herramientas` y `Reproductor de audio`. El acomodo se conserva entre
  ejecuciones y la distribución inicial sigue la proporción aproximada 31/69 del diseño de producto.
- Biblioteca persistente dividida en `Multimedia` y `Presentación`; esta última incluye las bibliotecas independientes
  `Biblia` y `Presentaciones`.
- `Presentación` pasa al panel `Herramientas`, junto con las entradas reservadas `Captura` y `NDI`. El panel mantiene un
  historial local de las cuatro presentaciones importadas más recientes y permite volver a activarlas.
- Carpetas multimedia con búsqueda local, arrastrar y soltar, reordenamiento y carga diferida.
- Buscador bíblico por texto o referencia, selector de traducciones locales y cuadrícula limitada de resultados para
  mantener el rendimiento con biblias completas.
- Menú `Biblia` para importar traducciones TXT compatibles y configurar el lienzo con vista previa: tipografía, tamaño,
  alineación del versículo, seis posiciones posibles para la referencia y fondo personalizado de imagen o video.
- Proyección bíblica 16:9 con fondo negro predeterminado, versículo blanco y referencia blanca de menor tamaño. Los
  fondos de video pueden repetirse; sin repetición conservan el último fotograma al terminar.
- Menú `Archivo > Importar` con opciones `PowerPoint` y `PDF`. Cada página o diapositiva se convierte en una imagen
  numerada y reemplaza la presentación importada anteriormente.
- Menú contextual `Eliminar` en las carpetas de `Multimedia`; quita la referencia de la biblioteca sin borrar el
  archivo original del equipo.
- Tarjetas con nombre y miniatura; una selección sustituye inmediatamente al contenido anterior.
- Vista previa local y escenario a pantalla completa sobre un monitor elegido.
- Interruptor rápido para activar o apagar la salida de escenario.
- Controles anterior, reproducir/pausar, detener, siguiente y repetición continua del archivo actual, más teclas
  multimedia mientras la aplicación está activa.
- Línea de tiempo, medidores estéreo y volumen del contenido.
- Reproductor de audio local independiente con lista persistente por arrastrar y soltar, línea de tiempo manipulable y
  controles reproducir/pausar y detener. No sustituye el contenido que se está proyectando.
- Menú `Editar` con ajuste opcional de fotos y videos al área completa 16:9; la preferencia se conserva.
- Ventanas `Pantallas`, `Sonido` y `Biblia`; se conservan monitor, dispositivo, audio y layout bíblico seleccionados.
- Posición y tamaño de la ventana principal persistentes.
- Ejecución portátil local en `dist/OPBS` para desarrollo, sin alterar una instalación normal de OBS. Los Releases
  públicos se distribuyen únicamente mediante el instalador propio.
- Actualizador oficial de OBS inactivo; OPBS consulta exclusivamente los Releases del repositorio GitHub configurado y
  verifica el SHA-256 antes de abrir su instalador.
- Instalador propio de Windows con acceso directo de escritorio, accesos del menú Inicio y desinstalador.
- Lanzador previo al inicio que comprueba versiones, permite posponer, conserva los datos fuera de los binarios y vuelve
  a abrir OPBS automáticamente después de actualizar.

## Ramas y base

- `obs-original`: OBS Studio limpio en `7546be7` (etiqueta `32.2.0`).
- `master`: seguimiento de la base pública; no desarrollar aquí.
- `feature/media-presenter`: producto y única rama de trabajo actual.
- remoto `obs-public`: `https://github.com/obsproject/obs-studio.git`, configurado sin permiso de escritura.

Comparación completa con OBS original:

```powershell
git diff --stat obs-original...feature/media-presenter
git diff obs-original...feature/media-presenter
```

## Continuar el proyecto

- Preparar otro equipo y transferir el historial: `PRESENTADOR_CONTINUIDAD.md`.
- Entender componentes, datos y flujo de reproducción: `PRESENTADOR_ARQUITECTURA.md`.
- Cargar traducciones y revisar el formato de versículos: `BIBLIAS.md`.
- Importar y sustituir presentaciones: `PRESENTACIONES.md`.
- Revisar decisiones, commits, pruebas y trabajo pendiente: `PRESENTADOR_HISTORIAL.md`.
- Revisar o incorporar versiones nuevas de OBS: `OBS_ACTUALIZACIONES.md`.
- Compilar, empaquetar y publicar versiones: `COMPILACION_Y_RELEASES.md`.
- Reglas para colaboradores: `../AGENTS.md`.

## Licencia

OBS Studio y este trabajo derivado se distribuyen bajo GPL-2.0. Al distribuir binarios deben conservarse los avisos y
ofrecerse el código fuente correspondiente, incluidas estas modificaciones.
