# Actualizaciones controladas de OBS

OPBS es un derivado de OBS, no una instalación oficial sin modificaciones. El actualizador binario de OBS no
puede distinguir las funciones originales de la interfaz, reproducción, audio y proyección añadidas aquí. Por eso no
debe instalarse directamente sobre Presentador.

## Protección aplicada

- Los paquetes de desarrollo, `dist` y `portable` contienen `bin/64bit/disable_updater.txt`.
- Los lanzadores añaden también `--disable-updater`.
- `obs-public` permite descargar código oficial, pero su dirección de envío permanece en `DISABLED`.
- `obs-original` continúa siendo la referencia inmutable de OBS 32.2.0 usada para crear el producto.
- La versión integrada se declara en `presenter-tools/obs-upstream-policy.json`.
- Los archivos modificados por Presentador y las áreas críticas se clasifican antes de aceptar cualquier cambio.

Esto impide que el actualizador oficial sustituya silenciosamente los binarios personalizados. Las nuevas versiones
terminadas de OPBS se distribuyen mediante el actualizador propio descrito en `IA_LOCAL_OPBS.md`; este documento regula
únicamente cómo incorporar código nuevo procedente de OBS.

## Consultar una actualización

```powershell
.\presenter-tools\Review-OBS-Update.cmd
```

El comando descarga etiquetas del remoto oficial, elige la versión estable más reciente y compara sus cambios con la
última base integrada. Para revisar una versión concreta y conservar el informe:

```powershell
.\presenter-tools\Review-OBS-Update.cmd -TargetVersion 32.2.1 `
  -ReportPath docs\upstream-reviews\OBS-32.2.1.md
```

Cada ruta recibe una categoría:

| Categoría | Significado |
|---|---|
| `CONFLICTO` | OBS y Presentador modificaron el mismo archivo. Requiere integración manual. |
| `SENSIBLE` | La ruta pertenece a reproducción, audio, frontend u otra área protegida. |
| `AISLADO` | No pisa hoy una personalización conocida. Todavía debe justificarse su utilidad. |

`AISLADO` no significa `necesario`. Una corrección para una función todavía desactivada puede aplazarse hasta que esa
función vuelva a formar parte del producto.

## Aplicar únicamente lo necesario

1. Mantener limpio `feature/media-presenter`.
2. Generar y leer el informe.
3. Crear una rama temporal, por ejemplo `codex/obs-32.2.1-review`.
4. Incorporar solo el commit oficial que resuelve una necesidad concreta; no mezclar la etiqueta completa por defecto.
5. Resolver manualmente cualquier ruta `CONFLICTO` o `SENSIBLE`, conservando los invariantes de
   `PRESENTADOR_ARQUITECTURA.md`.
6. Ejecutar las pruebas separadas de imagen, video, audio, cambio repetido de medio, Biblia, presentaciones,
   persistencia, pantalla y salida de audio.
7. Compilar, empaquetar y ejecutar antes de fusionar la rama de revisión.
8. Solo después de aprobar la integración, actualizar `lastIntegratedUpstream` y documentar el resultado.

No se debe cambiar `libobs/obs-config.h` únicamente para hacer que Presentador anuncie una versión oficial más nueva:
esa versión solo es correcta cuando el código aplicable de esa entrega se ha revisado e integrado.

## Cómo detecta OBS una versión nueva

En Windows, OBS inicia `TimedCheckForUpdates` si las actualizaciones están habilitadas y ha vencido el intervalo de
consulta. El proceso oficial:

1. descarga `branches.json` para conocer canales como `stable` o `beta`;
2. descarga el manifiesto del canal, normalmente `manifest.json`;
3. verifica la firma de los datos descargados con la clave pública integrada en OBS;
4. compara la versión semántica del manifiesto con `LIBOBS_API_VER`;
5. si hay una versión posterior, muestra las notas y descarga el actualizador oficial;
6. el actualizador compara los hashes de los archivos instalados, solicita parches o archivos completos, valida el
   contenido y sustituye los binarios.

Los desarrolladores publican primero el código y una etiqueta de versión en GitHub. Su automatización de lanzamiento
genera los instaladores y archivos portátiles, publica hashes y alimenta los manifiestos del servicio de actualización.
Ese sistema es apropiado para OBS oficial porque ellos controlan el conjunto completo de binarios. En Presentador se
reutiliza el código fuente nuevo, pero no la sustitución automática de binarios.

## Revisión de OBS 32.2.1

OBS 32.2.1 fue publicado el 24 de julio de 2026. Respecto de nuestra base 32.2.0 cambia cuatro rutas y su corrección
funcional es para el hook de captura de juegos que podía quedar en uso durante una actualización. Presentador todavía
no expone captura de juegos, transmisión ni grabación, por lo que la actualización queda revisada y aplazada. Debe
reconsiderarse cuando se diseñe la recuperación de esas funciones.
