# OPBS

OPBS es una aplicación de presentación multimedia basada en OBS Studio. La **P** representa el presentador integrado:
una interfaz enfocada en biblioteca multimedia, salida a escenario, audio, Biblia y presentaciones.

> Estado actual: **OPBS 0.1.0**, versión inicial de prueba para Windows.

## Funciones actuales

- Biblioteca multimedia con carpetas, búsqueda, orden persistente y arrastrar y soltar.
- Reproducción de imágenes, video y audio con controles, volumen, línea de tiempo y repetición.
- Salida de escenario a una pantalla conectada.
- Biblioteca y proyección de versículos bíblicos con diseño configurable.
- Importación de presentaciones PDF y compatibilidad guiada para PowerPoint.
- Instalador propio, versión portable y actualizaciones mediante GitHub Releases.

## Compilar en Windows

La guía reproducible de compilación, empaquetado y publicación está en
[`docs/COMPILACION_Y_RELEASES.md`](docs/COMPILACION_Y_RELEASES.md).

```powershell
.\presenter-tools\Build-Presenter.cmd
.\presenter-tools\Package-Presenter.cmd
.\presenter-tools\Run-Presenter.cmd -SafeMode
```

Para generar el instalador y el ZIP de una versión:

```powershell
.\presenter-tools\Build-OPBS-Release.cmd -Version 0.1.0 -GitHubRepository ByMarioBz/OPBS
```

## Continuidad con OBS

El código original de OBS se conserva. `obs-original` identifica la base limpia y `obs-public` permite revisar
actualizaciones del proyecto upstream sin aplicarlas automáticamente sobre las funciones protegidas de OPBS.

Consulta:

- [`docs/PRESENTADOR_CONTINUIDAD.md`](docs/PRESENTADOR_CONTINUIDAD.md)
- [`docs/PRESENTADOR_ARQUITECTURA.md`](docs/PRESENTADOR_ARQUITECTURA.md)
- [`docs/OBS_ACTUALIZACIONES.md`](docs/OBS_ACTUALIZACIONES.md)

## Licencia

OPBS deriva de OBS Studio y conserva su licencia GNU General Public License v2.0 o posterior. Consulta
[`COPYING`](COPYING) y el archivo original [`README.rst`](README.rst) para la información del proyecto base.
