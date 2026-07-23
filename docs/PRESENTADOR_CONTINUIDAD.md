# Continuidad, compilación y traslado

Esta guía permite abrir el proyecto en otra computadora, cuenta o IDE sin depender de esta conversación.

## 1. Qué debe transferirse

El repositorio Git contiene el código y su historial. Estas carpetas locales son deliberadamente independientes:

| Ruta | Contenido | ¿Se versiona? | ¿Es necesaria en otro equipo? |
|---|---|---:|---:|
| `.git` | ramas, commits y referencia a OBS original | Sí, mediante Git | Sí |
| código + `docs` + `presenter-tools` | producto y documentación | Sí | Sí |
| `.deps` | dependencias binarias de OBS y Qt | No | Sí, se descarga o copia |
| `.tools` | CMake portátil usado en este equipo | No | No, si CMake está instalado |
| `build_x64` | resultados intermedios | No | No; se regenera |
| `dist/PresentadorMultimedia` | aplicación portátil compilada | No | Solo para ejecutar sin compilar |
| `portable` | copia limpia para compartir, con `Presentador.exe` y biblias locales | No | Para pruebas en otra PC |
| medios del usuario | imágenes, videos y canciones | No | Solo si se quieren conservar |

La biblioteca guarda rutas absolutas. Copiar la configuración portátil a otro equipo no copia los medios y las rutas
pueden dejar de existir. Para una prueba limpia se recomienda volver a importarlos.

## 2. Crear un paquete de traslado

Con el árbol de trabajo limpio:

```powershell
.\presenter-tools\Export-Project.cmd
```

Esto crea en la carpeta padre un archivo `.bundle` con todas las ramas y etiquetas locales. Para incluir también la
aplicación ya compilada:

```powershell
.\presenter-tools\Export-Project.cmd -IncludePortableApp
```

El `.bundle` es el respaldo principal porque conserva la historia y no depende de una cuenta en línea. También se
puede subir la rama `feature/media-presenter` a un repositorio Git propio. No intentar enviar cambios a `obs-public`.

## 3. Restaurar en otra computadora

```powershell
git clone .\PresentadorMultimedia-AAAAmmdd-HHmm.bundle PresentadorMultimedia
cd PresentadorMultimedia
git switch feature/media-presenter
git tag obs-original 7546be7266dde276d82d4681fe1ab4fd8e32cf2b
git remote add obs-public https://github.com/obsproject/obs-studio.git
git remote set-url --push obs-public DISABLED
```

Si se usa GitHub, GitLab o Azure DevOps propio, añadirlo como `origin` y subir las referencias:

```powershell
git remote add origin DIRECCION_DEL_REPOSITORIO
git push -u origin feature/media-presenter
git push origin obs-original
```

## 4. Herramientas verificadas en Windows

- Windows 11 x64.
- Visual Studio Build Tools con C++ de escritorio y Windows SDK.
- Generador usado: `Visual Studio 18 2026`.
- CMake 4.4.0; OBS requiere como mínimo 3.28.
- Git.
- Dependencias oficiales OBS `2026-07-15` para Windows x64.
- Dependencias Qt 6 oficiales OBS `2026-07-15` para Windows x64.

Los paquetes se publican en las versiones de `obsproject/obs-deps`. Extraerlos así:

```text
.deps/obs-deps-2026-07-15-x64
.deps/obs-deps-qt6-2026-07-15-x64
```

Los nombres, versiones y hashes esperados están en `CMakePresets.json`. Esta compilación desactiva CEF/Browser para
reducir dependencias (`ENABLE_BROWSER=OFF`). VLC es opcional; sin una instalación compatible el módulo se desactiva.

## 5. Compilar

La vía recomendada es el script del proyecto:

```powershell
.\presenter-tools\Build-Presenter.cmd
```

Configura `build_x64` cuando sea necesario y compila `obs-studio` como `RelWithDebInfo`. Se usa un solo trabajo de
compilación por defecto porque varias instancias de MSVC llegaron a competir por archivos PDB en este equipo. Para una
recompilación posterior se usa el mismo comando.

Equivalente manual:

```powershell
cmake -S . -B build_x64 -G "Visual Studio 18 2026" -A x64 `
  "-DCMAKE_PREFIX_PATH=$PWD/.deps/obs-deps-2026-07-15-x64;$PWD/.deps/obs-deps-qt6-2026-07-15-x64" `
  -DENABLE_BROWSER=OFF
cmake --build build_x64 --config RelWithDebInfo --target obs-studio --parallel 1
```

Si cambia la versión de Visual Studio o las dependencias, eliminar o renombrar `build_x64` y configurar de nuevo. No
reutilizar un `CMakeCache.txt` creado en otra ruta o computadora.

## 6. Empaquetar y ejecutar

```powershell
.\presenter-tools\Package-Presenter.cmd
.\presenter-tools\Run-Presenter.cmd
```

El empaquetado copia el contenido ejecutable de `build_x64/rundir/RelWithDebInfo` a
`dist/PresentadorMultimedia`. La configuración se guarda dentro de esa carpeta por `--portable`.

Para crear una copia limpia destinada a otra persona:

```powershell
.\presenter-tools\Create-Portable.cmd
```

El resultado queda en `portable`. El ejecutable es `portable/bin/64bit/Presentador.exe`; también puede iniciarse desde
`portable/INICIAR_PRESENTADOR.bat`. El marcador `portable/portable_mode.txt` conserva la configuración dentro del
paquete aun al abrir directamente el EXE. El generador copia las biblias locales, pero no las rutas ni la biblioteca
multimedia personal del equipo de desarrollo.

Para una sesión diagnóstica sin complementos no esenciales:

```powershell
.\presenter-tools\Run-Presenter.cmd -SafeMode
```

## 7. IDEs

- Visual Studio: abrir `build_x64/obs-studio.sln` después de configurar con CMake.
- VS Code: abrir la raíz; usar la terminal de PowerShell y los scripts de `presenter-tools`.
- CLion: abrir la raíz como proyecto CMake y replicar `CMAKE_PREFIX_PATH` y `ENABLE_BROWSER=OFF`.
- Otro editor: no necesita integración especial; CMake es la fuente de verdad.

Los archivos fuente principales son C/C++ y Qt. No editar los archivos generados dentro de `build_x64` o `dist`.

## 8. Flujo Git recomendado

```powershell
git switch feature/media-presenter
git status --short
git diff obs-original...HEAD --stat
```

Crear un commit por unidad funcional. Antes de confirmar:

```powershell
git diff --check
.\presenter-tools\Build-Presenter.cmd
```

Para incorporar una versión nueva de OBS, crear primero una rama específica, actualizar `obs-public`, resolver la
integración y probar todo el presentador. Nunca mover `obs-original`: esa referencia representa la base histórica
32.2.0 de este producto.
