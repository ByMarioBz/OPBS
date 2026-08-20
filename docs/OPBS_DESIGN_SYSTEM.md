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
| Fondo raíz | `#05070A` | Separadores y espacio exterior |
| Fondo de ventana | `#080A0E` | Ventana y diálogos |
| Superficie | `#11151A` | Paneles principales |
| Superficie elevada | `#171C22` | Tarjetas, menús y listas |
| Superficie interactiva | `#20262E` | Botones secundarios |
| Borde | `#303945` | Separación de componentes |
| Texto principal | `#F4F7FA` | Etiquetas y acciones |
| Texto secundario | `#98A3B1` | Metadatos y tiempo |
| Acento OPBS | `#0A84FF` | Selección, foco y acción primaria |
| Acento claro | `#38A0FF` | Bordes activos |
| Directo/peligro | `#C93646` | Transmitir, grabar y estados críticos |
| Audio correcto | `#32D583` | Medidores y confirmaciones |

El azul significa interacción o selección. El rojo se reserva para salida en vivo, grabación, error o acción
destructiva. Ningún texto informativo debe usar esos colores si no es interactivo o un estado.

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
- Altura mínima de control: 28 px; objetivo habitual: 30–32 px.
- Radio pequeño: 4 px; control: 6–7 px; tarjeta: 8 px.
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
- **Paneles:** barra de título plana, superficie única y separador negro suficiente para redimensionar.
- **Tarjetas:** miniatura 16:9, nombre legible, borde de selección y tooltip para rutas largas.
- **Buscadores:** campo local a su panel, botón de limpiar y foco azul; el placeholder explica el alcance.
- **Botones:** verbo concreto o icono estándar con tooltip y nombre accesible.
- **Diálogos:** grupos claros, acción principal a la derecha y Cancelar siempre disponible.
- **Controles multimedia:** orden estable, iconos familiares, foco por teclado y estado reproducir/pausar perceptible.
- **Estados vacíos:** explican qué falta y cuál es la acción siguiente; no se usan como mensajes de error.
- **Indicadores de estado:** usan cápsulas compactas y nunca barras decorativas de ancho completo.
- **Búsqueda en escritorio:** conserva un ancho de lectura razonable en ventanas grandes; el espacio restante pertenece
  al contenido y a los metadatos del panel.
- **Selección lateral:** emplea una superficie elevada con tinte moderado; el azul brillante se reserva para el foco y
  las acciones primarias, evitando contornos intensos alrededor de listas completas.

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

## Implementación

- Los estilos compartidos viven en `frontend/widgets/OpbsDesignSystem.cpp` y `.hpp`.
- `ApplicationStyleSheet()` cubre ventana, menús, diálogos y controles Qt generales.
- `PresenterStyleSheet()` cubre el espacio modular, paneles, bibliotecas, transporte y transmisión.
- Los colores nuevos deben representar una función semántica existente o añadirse primero a esta especificación.
- No añadir hojas de estilo extensas directamente a un panel salvo que sean exclusivas de una vista previa dinámica.
