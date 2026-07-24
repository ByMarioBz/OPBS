# Presentaciones

## Importar

En la barra superior abrir:

```text
Archivo > Importar > PowerPoint
Archivo > Importar > PDF
```

El resultado aparece en `Presentación > Presentaciones`. Las tarjetas se numeran desde `1` hasta la última
diapositiva y conservan el orden del archivo original. Al seleccionar una tarjeta, la imagen se envía a la misma escena
privada que alimenta la vista previa y el escenario.

## Sustitución

Solo hay una presentación importada activa. La conversión nueva se realiza primero en una carpeta temporal. Si termina
correctamente, la aplicación sustituye la carpeta anterior y actualiza la biblioteca. Si falla, conserva las
diapositivas anteriores.

Las imágenes convertidas se almacenan dentro de la configuración portátil:

```text
config/obs-studio/presentations/current
```

No deben versionarse.

## PowerPoint

La importación de `.ppt` y `.pptx` usa Microsoft PowerPoint mediante automatización COM y requiere que PowerPoint esté
instalado. La exportación produce imágenes planas: texto, imágenes y objetos visibles quedan incorporados, mientras que
animaciones, transiciones y apariciones temporizadas dejan de existir.

Si PowerPoint no está disponible, exportar el archivo a PDF en otro equipo e importarlo mediante la opción PDF.

## PDF

La importación PDF usa el motor nativo de Windows y no requiere instalar un conversor adicional. Cada página se
rasteriza como PNG con ancho de 1920 píxeles, conservando su proporción original.

## Prueba mínima

1. Importar un archivo de tres diapositivas y comprobar tarjetas `1`, `2`, `3`.
2. Seleccionar las tres tarjetas y confirmar que vista previa y escenario cambian en el mismo orden.
3. Importar un segundo archivo de dos diapositivas y comprobar que solo quedan `1` y `2`.
4. Cerrar y abrir la aplicación para confirmar que las imágenes convertidas siguen disponibles.
