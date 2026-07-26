# Biblias del Presentador

La sección `Presentación > Biblia` descubre automáticamente los archivos `.txt` que se encuentren en:

```text
config/obs-studio/bibles
```

En las versiones portátiles esta carpeta está dentro de `dist/PresentadorMultimedia` o `portable`. Cada archivo aparece
como una opción en el selector de biblias. El nombre se obtiene del nombre del archivo; por ejemplo,
`biblia_reina_valera_1960.txt` aparece como `Reina Valera 1960`.

La forma recomendada de añadir una traducción es abrir `Biblia` en la barra superior, escribir su ruta o pulsar
`Examinar…` y después `Agregar Biblia`. El Presentador valida todos los bloques antes de copiar el archivo a su carpeta
interna. Un archivo sin texto, sin referencia, con etiquetas fuera de orden o con contenido ajeno a los bloques muestra
el aviso `Biblia incompatible` y no se agrega.

## Formato

Cada registro debe contener un texto y una referencia. Los marcadores no distinguen entre mayúsculas y minúsculas y
se permiten varias líneas de texto:

```text
------------------------------------------------------
[VErsiculo]
El que da testimonio de estas cosas dice: Ciertamente vengo en breve. Amén; sí, ven, Señor Jesús.
[referencia]
Apocalipsis 22:20
------------------------------------------------------
```

El archivo debe guardarse como UTF-8.

## Búsqueda

- Busca simultáneamente en el texto y la referencia.
- No distingue mayúsculas, minúsculas ni acentos.
- La Biblia no crea las 31,104 tarjetas al abrirse: espera una consulta.
- Se cuentan todas las coincidencias, pero se muestran como máximo 250 tarjetas al mismo tiempo para conservar el
  rendimiento.

Los archivos bíblicos se mantienen como datos locales de la aplicación y no forman parte del historial de código de
OBS ni de los paquetes públicos de OPBS. La aplicación no incluye la Reina-Valera 1960 ni otra traducción. Cada
usuario debe importar un TXT compatible que tenga derecho a utilizar. Al mover una configuración personal a otro
equipo debe copiarse también la carpeta `config/obs-studio/bibles`.

## Layout de proyección

El mismo diálogo `Biblia` muestra una vista previa 16:9 y permite elegir:

- tipo de letra;
- tamaño del versículo entre 24 y 180 píxeles;
- alineación izquierda, centro o derecha;
- referencia en izquierda, centro o derecha, tanto arriba como abajo.

La vista previa del diálogo cambia inmediatamente. Al guardar, los ajustes se conservan y se aplican a la vista previa
principal y a la pantalla de escenario. La referencia mantiene un tamaño menor que el versículo.
