# Protocolo
## Mensajes

### Solicitud de procesamiento
Cop: 
Master -> YAMA : indica sobre que archivo desea operar

**Formato**

    |nombreArchivo|

**Respuesta**
+ Caso: no se pudo acceder al archivo pedido
  * Mensaje de estado indicando error


+ Caso: exito
  1. N mensajes de informacion de workers
  2. Mensaje de estado indicando fin de lista

### Informacion de workers (transformacion)
Cop:
YAMA -> Master : informacion de los workers sobre los que se debera operar

**Formato**

    [ip:puerto]
    
** Comportamiento : Master **

  + Espera un mensaje con resumen de conexiones (exitosas y fallidas entre Master y los nodos)

### Envio de ejecutable
Cop:
Master -> Worker : archivo de ejecucion

**Formato**

    |archivo|

### Orden de reduccion global
Cop:
Master -> Worker : orden de ejecucion de la reduccion global enviada al worker encargado

**Formato**

    [ip:puerto]
    
**Comportamiento : Worker**
  + Comienza a conectarse a los workers (al recibir este mensaje se designa como encargado)

### Adquisicion de bloque
FileSystem -> DataNode : Devolverá el contenido del bloque solicitado almacenado en el Espacio de Datos

**Formato**

    |numeroDeBloque|

### Bloque leido
Cop:
DataNode -> FileSystem : Bloque de archivo

**Formato**

    |contenidoDelBloque|

### Escritura de bloque
Cop:
FileSystem -> DataNode : Grabará los datos enviados en el bloque solicitado del Espacio de Datos

**Formato**

    |numeroDeBloque|contenidoDelBloque|

### Almacenado de archivo
Cop:
{YAMA,Worker,Consola} -> FileSystem : Almacena el archivo

**Formato**

    |rutaCompleta|nombreArchivo|tipo(texto o binario)|contenindo|

**Respuesta**

  + Mensaje de estado con exito o informacion de error

### Lectura de archivo
Cop:
Consola -> FileSystem : Lee un archivo

**Formato**

    |rutaCompleta|nombreArchivo|
**Respuesta**

  + Caso: no se pudo acceder al archivo pedido
    + Mensaje de estado indicando error
  + Caso: exito
     + Mensaje de archivo leido con el contenido del archivo

### Archivo leido
Cop:
FileSystem -> Consola : Contenido de un archivo

**Formato**

    |contenidoDelArchivo|
