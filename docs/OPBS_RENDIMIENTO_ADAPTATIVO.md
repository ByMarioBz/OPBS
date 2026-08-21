# Rendimiento adaptativo de OPBS

OPBS ajusta su coste local y su salida de transmisión sin alterar la escena privada del presentador ni el proyector de
escenario. El controlador está separado de la interfaz para que Broadcast Presenter pueda reutilizar la misma política
cuando disponga de su propio motor.

## Perfil inicial del equipo

Al abrir la aplicación se consideran la RAM física y los procesadores lógicos. El lienzo de composición permanece en
1920 × 1080, mientras que la resolución codificada y los FPS se eligen con una política conservadora:

| Perfil | Umbral aproximado | Salida | FPS | Bitrate inicial recomendado |
|---|---|---:|---:|---:|
| Muy limitado | hasta 4 GB o 2 hilos | 854 × 480 | 30 | 1800 Kbps |
| Limitado | hasta 8 GB o 4 hilos | 1280 × 720 | 30 | 4000 Kbps |
| Equilibrado | menos de 16 GB o 8 hilos | 1920 × 1080 | 30 | 6000 Kbps |
| Alto | 16 GB y 8 hilos o más | 1920 × 1080 | 60 | 6000 Kbps |

Cuando existe NVENC, Quick Sync o AMF se prefiere el codificador de hardware H.264. Sin uno compatible se utiliza x264
con un preset más ligero en el perfil muy limitado. La decisión y los recursos detectados quedan en el registro.

## Carga local

Una muestra por segundo reúne CPU del proceso, memoria disponible, tiempo medio de render, fotogramas de render
perdidos y fotogramas omitidos por codificación. Tres muestras consecutivas con CPU mayor o igual a 85 %, memoria
física utilizada mayor o igual a 92 %, render por encima del 90 % de su presupuesto o pérdida mayor o igual a 2 %
activan el modo restringido.

El modo restringido:

- conserva escenario, audio, grabación y transmisión;
- conserva la vista previa principal del presentador;
- pausa la vista previa local duplicada de transmisión;
- suspende miniaturas de cámara y captura;
- reduce la frecuencia de actualización de las líneas de tiempo.

Después de 15 muestras saludables restaura gradualmente esas tareas. Las miniaturas de imágenes se decodifican fuera
del hilo de interfaz con una concurrencia de 1, 2 o 4 trabajos según el perfil. Solo se precargan ocho tarjetas cercanas
y un archivo dañado no se reintenta indefinidamente.

## Red y doble destino

YouTube y Facebook comparten un codificador. Por ello sus salidas RTMP no ejecutan controladores de bitrate
independientes: `PresenterPanel` calcula el peor valor de congestión y pérdida entre los destinos y actualiza una sola
vez el codificador compartido.

- Presión moderada durante tres muestras: reduce el bitrate 15 %.
- Presión grave durante dos muestras: reduce el bitrate 22 %.
- Conexión saludable durante 25 muestras: recupera 6 %.
- Cada cambio tiene un periodo de enfriamiento para impedir oscilación.
- El bitrate nunca supera el configurado por el usuario ni baja del mínimo de la resolución activa.

Si la red permanece grave 60 segundos después de alcanzar el bitrate mínimo, OPBS baja un nivel de resolución y
reconecta automáticamente. La escalera es 1080p → 720p → 480p y admite como máximo dos descensos por sesión. No se
realiza esta reconexión mientras existe una grabación activa.

## Memoria y almacenamiento

La biblioteca continúa guardando rutas y no copias de los medios. Solo la fuente seleccionada se mantiene activa; las
miniaturas usan imágenes reducidas y trabajo limitado. La protección nativa de grabación comprueba cada segundo el
espacio disponible y detiene la escritura antes de agotar el volumen. Los datos de usuario permanecen fuera de los
binarios instalados y no se regeneran al actualizar.

## Diagnóstico

Buscar en el registro:

```text
OPBS adaptive host profile
OPBS adaptive encoder
OPBS adaptive bitrate
OPBS adaptive UI entered constrained mode
OPBS adaptive resolution changed
```

El tooltip de los indicadores de destino muestra bitrate actual, CPU, memoria utilizada y carga de render. Una prueba
real de red requiere claves privadas y debe comprobar por separado un destino, dos destinos y una grabación local.
