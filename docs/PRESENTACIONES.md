# Presentaciones

## Importar

En la barra superior abrir:

```text
Archivo > Importar > PowerPoint
Archivo > Importar > PDF
```

El resultado se agrega a `Presentaciones recientes`. Importar no abre automáticamente el archivo nuevo: conserva la
herramienta visible, la presentación cargada, la diapositiva seleccionada y el contenido activo del escenario. Para
abrir la presentación importada, el usuario la selecciona explícitamente en la lista de recientes. Sus tarjetas se
numeran desde `1` hasta la última diapositiva y conservan el orden del archivo original. Al seleccionar una tarjeta, la
imagen se envía a la misma escena privada que alimenta la vista previa y el escenario.

Después de seleccionar cualquier diapositiva, `←` y `→` permiten retroceder o avanzar sin volver a usar el ratón. Las
flechas se detienen en la primera y última diapositiva; no recorren en bucle. El atajo pertenece exclusivamente a la
cuadrícula enfocada, por lo que no modifica presentaciones mientras se escribe en otro control.

## Historial y activación

Solo hay una presentación cargada en el panel a la vez, pero se recuerdan hasta cuatro importaciones. La conversión
nueva se realiza primero en una carpeta temporal y solo se registra cuando termina correctamente. Si falla, conserva
sin cambios la lista y las diapositivas anteriores.

Cuando se supera el límite, la aplicación elimina la importación más antigua que no sea la presentación cargada. Esto
evita perder sus archivos mientras se navega o se proyecta durante una sesión en vivo.

Las imágenes convertidas se almacenan dentro de la configuración portátil:

```text
config/opbs/presentations/presentation-<identificador>
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
3. Mantener seleccionada la diapositiva `2`, importar un segundo archivo y comprobar que la vista previa, el escenario,
   la presentación cargada y la selección permanecen en `2`.
4. Confirmar que el segundo archivo aparece en `Presentaciones recientes` y solo se abre al seleccionarlo.
5. Repetir la importación mientras están visibles Biblia, Captura, NDI y Multimedia; ninguna vista debe cambiar.
6. Importar cinco presentaciones mientras la más antigua está cargada y comprobar que sus diapositivas siguen
   disponibles; debe retirarse otra importación no activa.
7. Cerrar y abrir la aplicación para confirmar que las imágenes convertidas siguen disponibles.
8. Seleccionar la primera diapositiva y recorrer toda la presentación con `→`; volver con `←` y confirmar que no existe
   un cuadro negro entre páginas ni navegación circular en los extremos.
