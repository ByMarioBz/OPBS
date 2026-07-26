# Guía para BIONIC, LM Studio y otras IA locales

Esta guía es el contrato operativo para una IA local que prepare compilaciones y lanzamientos de OPBS. La IA puede
automatizar tareas repetibles, pero no decide por sí sola versiones, funciones, fusiones de OBS ni publicaciones.

## Objetivo

- Producto: `OPBS`.
- Significado: OBS con Presentador integrado.
- Primera versión: `0.1.0`.
- Rama de producto: `feature/media-presenter`.
- Base histórica inmutable: `obs-original`.
- Código oficial de OBS: remoto de solo lectura `obs-public`.
- Distribución de OPBS: repositorio GitHub propio configurado como `origin`.

## Reglas que la IA nunca debe romper

1. Leer `AGENTS.md` y todos los documentos que exige antes de editar.
2. No trabajar en `master`, no mover `obs-original` y no enviar código a `obs-public`.
3. No ejecutar ni reactivar el actualizador binario oficial de OBS.
4. No recuperar transmisión, grabación, captura u otras funciones ocultas sin una instrucción explícita del dueño.
5. No borrar código original de OBS para simplificar una función; ocultarlo o desacoplarlo.
6. No borrar medios, biblias, presentaciones ni configuraciones personales.
7. No incluir Biblias, preferencias locales ni símbolos de depuración en un Release público.
7. No publicar con cambios sin confirmar, pruebas fallidas o un árbol Git sucio.
8. No guardar contraseñas, tokens de GitHub o claves dentro del repositorio, scripts, instalador o aplicación.
9. No usar `gh release upload --clobber` ni sustituir artefactos de una versión ya publicada.
10. No inventar una versión. El dueño debe indicar el siguiente número SemVer.

## Preparación única de otra computadora

1. Restaurar el repositorio y dependencias con `PRESENTADOR_CONTINUIDAD.md`.
2. Instalar el compilador de instaladores:

```powershell
.\presenter-tools\Ensure-OPBS-InstallerTools.cmd
```

3. Instalar GitHub CLI:

```powershell
.\presenter-tools\Ensure-OPBS-PublishTools.cmd
```

4. El dueño inicia sesión personalmente; la IA no escribe ni lee la contraseña:

```powershell
gh auth login
```

5. Crear un repositorio público de GitHub para OPBS y configurarlo:

```powershell
.\presenter-tools\Configure-OPBS-GitHub.cmd PROPIETARIO/REPOSITORIO
```

El repositorio debe ser público porque el actualizador instalado no contiene credenciales. Para un repositorio privado
haría falta un servicio de actualizaciones independiente; nunca se debe incrustar un token personal en OPBS.

## Compilar una versión indicada por el dueño

Ejemplo para la versión inicial:

```powershell
.\presenter-tools\Build-OPBS-Release.cmd -Version 0.1.0 `
  -GitHubRepository PROPIETARIO/REPOSITORIO
```

Este comando:

1. modifica únicamente la versión y configuración de lanzamiento;
2. configura CMake con esa versión;
3. compila OPBS;
4. crea el paquete de desarrollo y un portable limpio;
5. genera el instalador NSIS;
6. genera el ZIP portable y el SHA-256 del instalador.

Los resultados quedan en:

```text
release/0.1.0/OPBS-Setup-x64.exe
release/0.1.0/OPBS-Setup-x64.exe.sha256
release/0.1.0/OPBS-Portable-x64.zip
```

`release` no se versiona. Sus archivos se adjuntan a GitHub Releases.

## Pruebas obligatorias antes de publicar

- `git diff --check`.
- Compilación completa sin errores.
- Inicio de `dist/OPBS/bin/64bit/OPBS.exe`.
- Nombre y versión correctos en la ventana.
- Imagen, video y audio.
- Cambio repetido entre dos medios.
- Línea de tiempo y salida de audio.
- Biblia y fondo.
- Importación de PDF/presentaciones.
- Proyección y monitor recordado.
- Instalación en una ruta de prueba.
- Acceso directo de escritorio y menú Inicio.
- Aparición del desinstalador.
- Desinstalación conservando configuración por defecto.
- Actualizador: error claro si falta repositorio y comparación correcta cuando existe un Release posterior.

Registrar resultados en `PRESENTADOR_HISTORIAL.md`.

## Commit, etiqueta y publicación

Después de revisar manualmente los cambios:

```powershell
git add -- RUTAS_REVISADAS
git commit -m "Release OPBS 0.1.0"
git tag -a opbs-v0.1.0 -m "OPBS 0.1.0"
git push origin feature/media-presenter
git push origin opbs-v0.1.0
.\presenter-tools\Publish-OPBS-Release.cmd
```

La publicación se detiene si:

- Git tiene cambios sin commit;
- la etiqueta no apunta al commit actual;
- falta alguno de los tres artefactos;
- GitHub CLI no está autenticado;
- el repositorio no es público.

GitHub genera además archivos del código fuente desde la etiqueta. Esto ayuda a cumplir GPL-2.0, pero las notas del
Release deben recordar que OPBS deriva de OBS Studio y que el código modificado está disponible en el repositorio.

## Prompt recomendado para la IA local

```text
Trabaja en OPBS siguiendo AGENTS.md y docs/IA_LOCAL_OPBS.md. El dueño autorizó preparar la versión X.Y.Z, pero no
publicarla todavía. No modifiques master ni obs-original, no recuperes transmisión o grabación y no uses el actualizador
oficial de OBS. Cambia la versión únicamente mediante presenter-tools/Set-OPBS-Version.cmd, compila con
Build-OPBS-Release.cmd, ejecuta todas las validaciones documentadas y detente antes de cualquier git push, etiqueta o
GitHub Release. Reporta archivos cambiados, pruebas y cualquier bloqueo.
```

Para publicar, el dueño debe dar una segunda instrucción explícita después de revisar el resultado.

## Firma de código

El instalador funciona sin certificado, pero Windows puede mostrar SmartScreen porque el ejecutable no está firmado.
Una versión distribuida ampliamente debería firmar `OPBS.exe` y `OPBS-Setup-x64.exe` con un certificado de firma de
código y marca de tiempo. La clave privada nunca debe entregarse a una IA ni guardarse en Git.
