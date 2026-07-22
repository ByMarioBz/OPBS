# Presentador multimedia

Esta rama transforma temporalmente el frontend de OBS Studio en una aplicación enfocada en presentar contenido
multimedia, sin eliminar el código ni las funciones originales de OBS.

## Estado de esta primera versión

- La ventana principal muestra una vista previa y una biblioteca en cuadrícula, repartidas aproximadamente al 50 %.
- Se pueden importar varios archivos individuales de imagen, video o audio.
- Cada archivo aparece con su nombre y una miniatura. Para video, la miniatura se genera con el sistema de capturas
  de fuentes de OBS; los archivos únicamente de audio muestran el icono correspondiente.
- Al seleccionar una tarjeta, el contenido anterior se detiene y se sustituye por el nuevo tanto en la vista previa
  como en la salida de escenario.
- El menú superior `Pantallas` muestra dos tarjetas. `Escenario` permite elegir un monitor conectado y abre en él
  una salida de pantalla completa. La segunda tarjeta queda reservada para una salida futura.
- Las áreas clásicas de OBS se ocultan en este modo, pero su implementación permanece intacta.

## Separación respecto de OBS original

- `obs-original`: referencia limpia al commit de OBS usado como punto de partida.
- `feature/media-presenter`: desarrollo de la nueva aplicación.

Para comparar el producto con la base original se puede usar:

```powershell
git diff obs-original...feature/media-presenter
```

## Arquitectura

`PresenterPanel` mantiene una escena privada de OBS que funciona como escenario. La vista previa y el proyector a
pantalla completa renderizan esa misma escena. Al hacer clic en otro archivo se reemplaza el único elemento activo de
la escena; de esta forma nunca se superponen dos contenidos.

La reproducción de video y audio utiliza `ffmpeg_source`, las imágenes utilizan `image_source`, y la salida de monitor
reutiliza `OBSProjector`. Esto conserva el renderizado, la aceleración y la compatibilidad multimedia de OBS.

## Compilación

El proyecto conserva el sistema oficial de compilación de OBS y su preset `windows-x64`. Se necesitan CMake 3.28 o
posterior, el Visual Studio indicado por el preset actual y las dependencias precompiladas de OBS. Después de instalar
esas herramientas:

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64
```

Este equipo todavía no tiene CMake ni el compilador de Visual Studio disponibles, por lo que la validación binaria debe
realizarse cuando estén instalados.

## Licencia

OBS Studio se distribuye bajo GPL-2.0. Una aplicación derivada y distribuida debe conservar las obligaciones de esa
licencia, incluidos los avisos y la disponibilidad del código fuente correspondiente.
