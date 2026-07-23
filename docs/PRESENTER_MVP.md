# Presentador multimedia

Aplicación de escritorio para organizar y proyectar imágenes, videos y audio, construida sobre OBS Studio 32.2.0.
El frontend clásico de OBS está temporalmente oculto, pero su implementación permanece disponible para recuperar e
integrar funciones de forma gradual.

## Estado actual

- Biblioteca persistente con carpetas, búsqueda local, arrastrar y soltar, reordenamiento y carga diferida.
- Tarjetas con nombre y miniatura; una selección sustituye inmediatamente al contenido anterior.
- Vista previa local y escenario a pantalla completa sobre un monitor elegido.
- Interruptor rápido para activar o apagar la salida de escenario.
- Controles anterior, reproducir/pausar, detener, siguiente y repetición continua del archivo actual, más teclas
  multimedia mientras la aplicación está activa.
- Línea de tiempo, medidores estéreo y volumen del contenido.
- Menú `Editar` con ajuste opcional de fotos y videos al área completa 16:9; la preferencia se conserva.
- Ventanas `Pantallas` y `Sonido`; se conservan monitor, dispositivo, volumen, ganancia y efecto seleccionados.
- Posición y tamaño de la ventana principal persistentes.
- Ejecución portátil en `dist/PresentadorMultimedia`, sin alterar una instalación normal de OBS.

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
- Revisar decisiones, commits, pruebas y trabajo pendiente: `PRESENTADOR_HISTORIAL.md`.
- Reglas para asistentes y colaboradores: `../AGENTS.md`.

## Licencia

OBS Studio y este trabajo derivado se distribuyen bajo GPL-2.0. Al distribuir binarios deben conservarse los avisos y
ofrecerse el código fuente correspondiente, incluidas estas modificaciones.
