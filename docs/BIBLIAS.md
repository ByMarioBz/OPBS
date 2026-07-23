# Biblias del Presentador

La sección `Presentación > Biblia` descubre automáticamente los archivos `.txt` que se encuentren en:

```text
config/obs-studio/bibles
```

En la versión portátil esta carpeta está dentro de `dist/PresentadorMultimedia`. Cada archivo aparece como una opción
en el selector de biblias. El nombre se obtiene del nombre del archivo; por ejemplo,
`biblia_reina_valera_1960.txt` aparece como `Reina Valera 1960`.

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
OBS. Al mover el programa a otro equipo debe copiarse también la carpeta `config/obs-studio/bibles`.
