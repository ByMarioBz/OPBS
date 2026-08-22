# Sistema visual de OPBS

Este documento define la identidad de interfaz propia de OPBS y la base que podrá heredar Broadcast Presenter. Es una
especificación interna creada para el producto; no copia recursos, componentes ni documentación de Apple u otros
productos.

## Principios

1. **Claridad operativa:** las acciones críticas de escenario, transmisión y grabación deben reconocerse de inmediato.
2. **Contenido primero:** las vistas previas y los medios tienen prioridad sobre la decoración.
3. **Comportamiento de Windows:** menús, ventanas, teclado, ratón, foco y diálogos deben sentirse naturales en Windows.
4. **Consistencia:** un mismo estado o acción conserva color, forma y comportamiento en toda la aplicación.
5. **Accesibilidad:** el color nunca es el único indicador; todo control importante tiene texto, icono o descripción.
6. **Rendimiento:** no se usan desenfoques permanentes, transparencias costosas ni animaciones decorativas sobre video.
7. **Identidad independiente:** OPBS no imita macOS, OBS ni otra aplicación; utiliza su propia paleta y componentes Qt.

## Paleta semántica oscura

| Función | Valor | Uso |
|---|---:|---|
| Fondo raíz | `#070709` | Separadores y espacio exterior |
| Fondo de ventana | `#0B0B0E` | Ventana y diálogos |
| Superficie | `#121216` | Paneles principales |
| Superficie elevada | `#19191E` | Tarjetas, menús y listas |
| Superficie interactiva | `#232329` | Botones secundarios |
| Borde | `#303038` | Separación de componentes |
| Texto principal | `#F5F5F7` | Etiquetas y acciones |
| Texto secundario | `#A6A6B0` | Metadatos y tiempo |
| Acción primaria | `#0969BD` | Controles rellenos con texto blanco |
| Foco/acento OPBS | `#5AAEFF` | Foco, borde activo y señal interactiva |
| Directo/peligro | `#C43D4D` | Transmitir, grabar y estados críticos |
| Audio correcto | `#35C98A` | Medidores y confirmaciones |

El azul significa interacción o selección. El rojo se reserva para salida en vivo, grabación, error o acción
destructiva. Ningún texto informativo debe usar esos colores si no es interactivo o un estado.

La escala de superficies mantiene incrementos pequeños y constantes de luminosidad para comunicar profundidad sin
gradientes ni transparencia costosa. Sobre la superficie principal, el texto primario alcanza aproximadamente `17:1`
y el secundario `7.7:1`; el texto blanco sobre la acción primaria alcanza aproximadamente `5.1:1`. El azul claro se
usa como elemento gráfico o contorno, no como texto pequeño sobre fondos claros.

## Tipografía

- Familia de Windows: `Segoe UI Variable`; Qt usa la sustitución disponible del sistema cuando no existe.
- Texto normal: 13 px.
- Texto secundario: 12–13 px con color secundario, no reducciones extremas.
- Títulos de panel: 11–13 px, peso 600–700.
- Título de sección principal: 19 px, peso 650.
- No usar mayúsculas para párrafos, mensajes, botones o nombres de archivos. Se permiten en rótulos breves de panel.

## Espaciado y geometría

- Unidad base: 4 px.
- Separaciones normales: 8 px y 12 px.
- Márgenes de panel: 10–16 px.
- Altura mínima de control: 34 px; objetivo habitual: 36–40 px para campos de entrada frecuentes.
- Radio pequeño: 7 px; control: 10–13 px; tarjeta o superficie elevada: 13–15 px.
- Los grupos relacionados permanecen próximos; los grupos diferentes se separan por al menos 12 px.

## Estados interactivos

Todo control debe contemplar:

- `normal`: superficie neutra y borde visible;
- `hover`: superficie más clara;
- `pressed`: superficie más oscura;
- `focus`: borde azul de 2 px visible con teclado;
- `selected/checked`: fondo azul y texto blanco;
- `disabled`: contraste reducido sin desaparecer;
- `error/live`: rojo acompañado de texto o icono.

La selección de una tarjeta se indica mediante borde y fondo, no solo mediante color. Un destino de arrastre válido se
resalta únicamente mientras el archivo está encima; un destino inválido conserva el cursor de operación no permitida.

## Componentes

- **Menú principal:** una sola fila compacta, sin repetir funciones en una cabecera secundaria.
- **Paneles:** una sola superficie continua; el título usa un material opaco ligeramente elevado y el separador exterior
  conserva el espacio suficiente para redimensionar. Arrastrar o hacer doble clic en el título mantiene las funciones
  de mover y desacoplar sin mostrar controles permanentes que compitan con el contenido.
- **Cabeceras de panel:** altura fija de 46 px, icono de 16 px, margen horizontal de 17 px y separación de 10 px. Su
  tamaño mínimo se declara al sistema de acoplamiento para impedir que el contenido invada o recorte el título.
- **Iconografía de panel:** Escenario, Multimedia, En vivo, Herramientas y Audio usan glifos originales de OPBS,
  monocromos, geométricos y de 16 px. Los símbolos describen la función y no son decoración de marca.
- **Tarjetas multimedia:** miniatura estática 16:9, nombre legible, borde de selección y tooltip para rutas largas.
  Las fuentes de Captura se muestran como filas de nombre, sin miniatura, para no duplicar decodificación de video.
- **Buscadores:** campo local a su panel, glifo original de búsqueda, botón de limpiar y foco azul; el placeholder
  explica el alcance. El glifo se dibuja dentro del propio campo y se centra con su altura real, evitando recortes o
  desplazamientos al cambiar el escalado de Windows. El campo se expande con el panel y solo cede espacio a controles
  del mismo alcance, como el selector de Biblia o el contador multimedia.
- **Botones:** verbo concreto o icono estándar con tooltip y nombre accesible.
- **Diálogos:** comparten el mismo sistema de superficies, títulos, subtítulos y navegación lateral que el espacio de
  trabajo; mantienen grupos claros, acción principal a la derecha y Cancelar siempre disponible.
- **Controles multimedia:** orden estable, iconos familiares, foco por teclado y estado reproducir/pausar perceptible.
- **Controles agrupados:** acciones consecutivas del mismo nivel comparten una superficie contenedora. El transporte y
  los modos de transmisión se perciben como unidades, mientras cada botón conserva su foco y nombre accesible.
- **Acciones de transmisión:** Transmitir y Grabar mantienen color de advertencia y un icono descriptivo; Cámaras,
  Presentador y Ambos forman un selector segmentado con iconos propios y un único estado activo.
- **Estado operativo:** LIVE, REC y los destinos remotos usan cápsulas compactas alineadas a la izquierda. El estado
  vive en la franja superior común, frente al interruptor de Escenario, para no reducir las vistas previas. Combina
  texto, color, duración y porcentaje cuando existe telemetría real; nunca depende solo de un punto de color.
- **Menús contextuales:** conservan el mismo material y espaciado del menú principal. Las acciones destructivas se
  separan de Agregar y Cambiar nombre; los estados por elemento muestran una marca al lado derecho del texto. Se
  presentan como una superficie elevada de radio amplio, sombra del sistema, filas compactas de 32 px, iconografía
  vectorial monocroma de 16 px y áreas de selección completas. No dependen de los iconos coloreados del Explorador de
  Windows. Su aparición usa un desvanecimiento de 140 ms únicamente cuando los efectos del sistema están habilitados.
- **Estados vacíos:** explican qué falta y cuál es la acción siguiente; no se usan como mensajes de error.
- **Indicadores de estado:** usan cápsulas compactas y nunca barras decorativas de ancho completo.
- **Búsqueda en escritorio:** conserva un ancho de lectura razonable en ventanas grandes; el espacio restante pertenece
  al contenido y a los metadatos del panel.
- **Selección lateral:** emplea una superficie elevada con tinte moderado; el azul brillante se reserva para el foco y
  un indicador de 3 px, evitando rellenos saturados y contornos intensos alrededor de listas completas.

## Accesibilidad y entrada

- Contraste objetivo de 4.5:1 para texto normal y 3:1 para texto grande o elementos gráficos esenciales.
- Las funciones principales deben poder operarse con teclado y ratón.
- Todo botón que solo muestre icono necesita tooltip y `accessibleName`.
- El orden de foco sigue la lectura visual de izquierda a derecha y de arriba abajo.
- No se reemplazan atajos estándar de Windows sin una razón funcional.
- Audio, cámara, escenario y transmisión muestran estado visible; nunca dependen únicamente de sonido o color.
- La interfaz debe tolerar el escalado de Windows y el redimensionamiento de la ventana sin ocultar acciones críticas.

## Movimiento y efectos

- Las transiciones explican un cambio de estado; no decoran controles estáticos.
- Duración recomendada de interfaz: 120–200 ms cuando Qt permita animación sin afectar el renderizado.
- Respetar una futura preferencia de movimiento reducido.
- No aplicar desenfoque en tiempo real sobre vistas previas, listas grandes o video.

## Referencia externa

Las guías de diseño externas pueden consultarse localmente para auditar claridad, accesibilidad y consistencia. No se
copian al repositorio, no forman parte del instalador y no definen la identidad visual. Ante un conflicto prevalecen
este documento, las convenciones de Windows, las necesidades de producción audiovisual y la licencia de OPBS.

Este sistema visual es la identidad compartida y evolutiva de OPBS y Broadcast Presenter. Cada producto podrá variar
su marca, licencia y motor, pero ambos deben conservar esta jerarquía, geometría y semántica operativa.

## Implementación

- Los estilos compartidos viven en `frontend/widgets/OpbsDesignSystem.cpp` y `.hpp`.
- `ApplicationStyleSheet()` cubre ventana, menús, diálogos y controles Qt generales.
- `PresenterStyleSheet()` cubre el espacio modular, paneles, bibliotecas, transporte y transmisión.
- Los colores nuevos deben representar una función semántica existente o añadirse primero a esta especificación.
- No añadir hojas de estilo extensas directamente a un panel salvo que sean exclusivas de una vista previa dinámica.
