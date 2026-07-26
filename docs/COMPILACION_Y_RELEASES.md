# Compilación y publicación de OPBS

Esta guía describe el proceso reproducible para compilar, probar, empaquetar y publicar versiones de OPBS.

## Identidad del producto

- Producto: `OPBS`.
- Significado: OBS con Presentador integrado.
- Rama de producto: `feature/media-presenter`.
- Base histórica inmutable: `obs-original`.
- Código oficial de OBS: remoto de solo lectura `obs-public`.
- Distribución de OPBS: repositorio GitHub propio configurado como `origin`.

## Reglas de seguridad

1. Leer `AGENTS.md` y los documentos que exige antes de editar.
2. No trabajar en `master`, no mover `obs-original` y no enviar código a `obs-public`.
3. No ejecutar ni reactivar el actualizador binario oficial de OBS.
4. No recuperar transmisión, grabación, captura u otras funciones ocultas sin una instrucción explícita del dueño.
5. No borrar código original de OBS para simplificar una función; ocultarlo o desacoplarlo.
6. No borrar medios, Biblias, presentaciones ni configuraciones personales.
7. No incluir Biblias, preferencias locales, rutas privadas ni símbolos de depuración en un Release público.
8. No publicar con cambios sin confirmar, pruebas fallidas o un árbol Git sucio.
9. No guardar contraseñas, tokens de GitHub o claves dentro del repositorio, scripts, instalador o aplicación.
10. No sustituir artefactos de una versión ya publicada.
11. No inventar una versión: el dueño debe indicar el siguiente número SemVer.

## Preparar otra computadora

1. Restaurar el repositorio y las dependencias siguiendo `PRESENTADOR_CONTINUIDAD.md`.
2. Instalar las herramientas para generar el instalador:

```powershell
.\presenter-tools\Ensure-OPBS-InstallerTools.cmd
```

3. Instalar GitHub CLI:

```powershell
.\presenter-tools\Ensure-OPBS-PublishTools.cmd
```

4. Iniciar sesión en GitHub:

```powershell
gh auth login
```

5. Configurar el repositorio público de OPBS:

```powershell
.\presenter-tools\Configure-OPBS-GitHub.cmd PROPIETARIO/REPOSITORIO
```

El repositorio debe ser público porque el actualizador instalado no contiene credenciales. Nunca se debe incrustar un
token personal en OPBS.

## Compilar una versión

Ejemplo:

```powershell
.\presenter-tools\Build-OPBS-Release.cmd -Version 0.1.0 `
  -GitHubRepository PROPIETARIO/REPOSITORIO
```

Este comando actualiza la versión, configura CMake, compila OPBS y genera:

```text
release/0.1.0/OPBS-Setup-x64.exe
release/0.1.0/OPBS-Setup-x64.exe.sha256
release/0.1.0/OPBS-Portable-x64.zip
```

La carpeta `release` no se versiona.

## Pruebas obligatorias

- Ejecutar `git diff --check`.
- Completar la compilación sin errores.
- Iniciar `dist/OPBS/bin/64bit/OPBS.exe`.
- Comprobar nombre y versión.
- Probar imagen, video, audio y cambio repetido entre medios.
- Probar línea de tiempo y salida de audio.
- Probar Biblia y fondo usando datos locales de prueba.
- Probar importación de PDF o presentaciones.
- Probar proyección y monitor recordado.
- Probar instalación, accesos directos y desinstalación.
- Confirmar que el portable y el instalador no contienen Biblias, configuración, rutas privadas ni símbolos.
- Probar la comparación de versiones del actualizador.

Registrar los resultados en `PRESENTADOR_HISTORIAL.md`.

## Commit, etiqueta y publicación

Después de revisar los cambios:

```powershell
git add -- RUTAS_REVISADAS
git commit -m "Release OPBS X.Y.Z"
git tag -a opbs-vX.Y.Z -m "OPBS X.Y.Z"
git push origin feature/media-presenter
git push origin opbs-vX.Y.Z
.\presenter-tools\Publish-OPBS-Release.cmd
```

Compilar y publicar son acciones distintas. La publicación solo debe realizarse cuando haya una autorización explícita.
El publicador se detiene si Git tiene cambios, la etiqueta no apunta al commit actual, faltan artefactos, GitHub CLI
no está autenticado o el repositorio no es público.

GitHub genera archivos del código fuente desde la etiqueta. Las notas del Release deben indicar que OPBS deriva de OBS
Studio y que el código modificado está disponible bajo GPL-2.0.

## Firma de código

El instalador funciona sin certificado, pero Windows puede mostrar SmartScreen. Para una distribución amplia se deben
firmar `OPBS.exe` y `OPBS-Setup-x64.exe` con un certificado y marca de tiempo. La clave privada nunca debe guardarse en
Git ni dentro de los artefactos.
