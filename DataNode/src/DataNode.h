#include <socketConfig.h>
#include <commons/log.h>

//ESTRUCTURA ARCHIVO CONFIGURACION
typedef struct dataNode_configuracion {
	char* IP_FILESYSTEM;
	char* PUERTO_FILESYSTEM;
	char* NOMBRE_NODO;
	char* IP_NODO;
	char* PUERTO_WORKER;
	char* PUERTO_DATANODE;
	char* RUTA_DATABIN;
	int CANTIDAD_MB_DATABIN;

} dataNode_configuracion;

const char* path = "/home/utnso/Desktop/tp-2017-2c-Todo-ATR/DataNode/configNodo.cfg";

dataNode_configuracion get_configuracion() {
	printf("Levantando archivo de configuracion del proceso DataNode\n");
	dataNode_configuracion configuracion;
	// Obtiene el archivo de configuracion
	t_config* archivo_configuracion = config_create(path);
	configuracion.IP_FILESYSTEM = get_campo_config_string(archivo_configuracion, "IP_FILESYSTEM");
	configuracion.PUERTO_FILESYSTEM = get_campo_config_string(archivo_configuracion, "PUERTO_FILESYSTEM");
	configuracion.NOMBRE_NODO = get_campo_config_string(archivo_configuracion, "NOMBRE_NODO");
	configuracion.IP_NODO = get_campo_config_string(archivo_configuracion, "IP_NODO");
	configuracion.PUERTO_DATANODE = get_campo_config_string(archivo_configuracion, "PUERTO_DATANODE");
	configuracion.RUTA_DATABIN = get_campo_config_string(archivo_configuracion, "RUTA_DATABIN");
	configuracion.PUERTO_WORKER = get_campo_config_string(archivo_configuracion, "PUERTO_WORKER");
	configuracion.CANTIDAD_MB_DATABIN = get_campo_config_int(archivo_configuracion, "CANTIDAD_MB_DATABIN");
	return configuracion;
}

void leer_bloque (int, void*);
void escribir_bloque (int, void*);
