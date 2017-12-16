#include <socketConfig.h>
#include <commons/log.h>

//ESTRUCTURA ARCHIVO CONFIGURACION
typedef struct worker_configuracion {
	char* IP_FILESYSTEM;
	char* PUERTO_FILESYSTEM;
	char* NOMBRE_NODO;
	char* IP_NODO;
	char* PUERTO_WORKER;
	char* PUERTO_DATANODE;
	char* RUTA_DATABIN;
} worker_configuracion;

typedef struct t_archivoRG{
	char* ip;
	int puerto;
	char* archivo;
	char* id;
}t_archivoRG;

typedef struct t_retorno_bloque{
	char* bloque;
	int tamanio;
}t_retorno_bloque;

const char* path = "/home/utnso/Desktop/tp-2017-2c-Todo-ATR/DataNode/configNodo.cfg";

worker_configuracion get_configuracion() {
	printf("Levantando archivo de configuracion del proceso Worker\n");
	worker_configuracion configuracion;
	// Obtiene el archivo de configuracion
	t_config* archivo_configuracion = config_create(path);
	configuracion.IP_FILESYSTEM = get_campo_config_string(archivo_configuracion, "IP_FILESYSTEM");
	configuracion.PUERTO_FILESYSTEM = get_campo_config_string(archivo_configuracion, "PUERTO_FILESYSTEM");
	configuracion.NOMBRE_NODO = get_campo_config_string(archivo_configuracion, "NOMBRE_NODO");
	configuracion.IP_NODO = get_campo_config_string(archivo_configuracion, "IP_NODO");
	configuracion.PUERTO_WORKER = get_campo_config_string(archivo_configuracion, "PUERTO_WORKER");
	configuracion.RUTA_DATABIN = get_campo_config_string(archivo_configuracion, "RUTA_DATABIN");
	return configuracion;
}

bool apareo (char* paths [], int,char* nombre_ordenado);
void transformacion(char* , char*, char*);
t_retorno_bloque* obtenerBloque(int numeroBloque, int tamanioBloque);
char* generarDirectorioTemporal();

char *randstring(size_t length);
void vaciarCarpeta(char*);
void crear_subcarpeta(char*);
