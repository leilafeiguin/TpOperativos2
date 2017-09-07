# Protocolo
## Mensajes

### |00000| Solicitud de procesamiento
Master -> YAMA : indica sobre que archivo desea operar

**Formato**

    |00000|nombreArchivo|

**Respuesta**
+ Caso: no se pudo acceder al archivo pedido
  * Mensaje de estado indicando error


+ Caso: exito
  1. N mensajes de informacion de workers
  2. Mensaje de estado indicando fin de lista

### |00001| Mensaje de estado
XXXX -> XXXX : Mensaje de Status

**Formato**

    |00001|mensaje|

### |00010| Informacion de workers (transformacion)
YAMA -> Master : informacion de los workers sobre los que se debera operar

**Formato**

    |00010|ip:puerto|bloque|bytesOcupados|nombreArchivoTemporal|
** Comportamiento : Master **

  + Espera un mensaje de error o n mensajes de informacion seguidos de un mensaje de estado con un fin de lista

### |00011| Orden de transformacion
Master -> Workers : orden de ejecucion de la transformacion

**Formato**

    |00011|bloque|bytesOcupados|nombreArchivoTemporal|
**Comportamiento : Worker**
  + Queda a la espera de n ordenes de ejecucion seguidas de un mensaje de envio de ejecutable

### |00100| Envio de ejecutable
Master -> Worker : archivo de ejecucion

**Formato**

    |00100|archivo|

### |00101| Informacion de workers (reduccion local)
YAMA -> Master : informacion de los workers sobre los que se debera operar

**Formato**

    |00101|ip:puerto|temporalDeTransformacion|temporalDeReduccionLocal|
**Comportamiento : Master**
  + Espera un mensaje de error o n mensajes de informacion seguidos de un mensaje de estado con un fin de lista


### |00110| Orden de reduccion local
Master -> Worker : orden de ejecucion de la reduccion local

**Formato**

    |00110|temporalDeTransformacion|temporalDeReduccionLocal|
**Comportamiento : Worker**
  + Queda a la espera de n ordenes de reduccion local seguidas de un mensaje de envio de ejecutable

### |00111| Informacion de workers (reduccion global)
YAMA -> Master : informacion de los workers sobre los que se debera operar

**Formato**

    |00111|ip:puerto|temporalReduccionLocal|temporalReduccionGlobal|Encargado|

**Comportamiento : Master**
  + Espera un mensaje de error o N mensajes de informacion seguidos de un mensaje de estado con un fin de lista


### |01000| Orden de reduccion global
Master -> Worker : orden de ejecucion de la reduccion global enviada al worker encargado

**Formato**

    |01000|ip:puerto|temporalDeReduccionLocal|temporalDeReduccionGlobal|

**Comportamiento : Worker**
  + Queda a la espera de n ordenes de reduccion global seguidas de un mensaje de estado con un fin de lista

### |01001| Informacion de worker (almacenado)
YAMA -> Master : informacion del worker encargado de almacenar sus resultados

**Formato**

    |01001|ip:puerto|temporalDeReduccionGlobal|

### |01010| Orden de almacenado
Master -> Worker : orden de almacenamiento enviada al worker encargado

**Formato**

    |01010|temporalDeReduccionGlobal|

**Comportamiento : Worker**
  + Queda a la espera de n ordenes de reduccion global seguidas de un mensaje de estado con un fin de lista

### |01011| Adquisicion de bloque
FileSystem -> DataNode : Devolverá el contenido del bloque solicitado almacenado en el Espacio de Datos

**Formato**

    |01011|numeroDeBloque|

### |01100| Bloque leido
DataNode -> FileSystem : Bloque de archivo

**Formato**

    |01100|contenidoDelBloque|

### |01101| Escritura de bloque
FileSystem -> DataNode : Grabará los datos enviados en el bloque solicitado del Espacio de Datos

**Formato**

    |01101|numeroDeBloque|contenidoDelBloque|

### |01110| Almacenado de archivo
{YAMA,Worker,Consola} -> FileSystem : Almacena el archivo

**Formato**

    |01110|rutaCompleta|nombreArchivo|tipo(texto o binario)|contenindo|

**Respuesta**

  + Mensaje de estado con exito o informacion de error

### |01111| Lectura de archivo
Consola -> FileSystem : Lee un archivo

**Formato**

    |01111|rutaCompleta|nombreArchivo|
**Respuesta**

  + Caso: no se pudo acceder al archivo pedido
    + Mensaje de estado indicando error
  + Caso: exito
     + Mensaje de archivo leido con el contenido del archivo

### |10000| Archivo leido
FileSystem -> Consola : Contenido de un archivo

**Formato**

    |10000|contenidoDelArchivo|
