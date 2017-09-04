#include <socketConfig.h>

//ESTRUCTURA ARCHIVO CONFIGURACION
typedef struct yama_configuracion {
	char* FS_IP;
	char* FS_PUERTO;
	int RETARDO_PLANIFICACION;
	char* ALGORITMO_BALANCEO;
} yama_configuracion;

const char* path = "/home/utnso/Desktop/tp-2017-2c-Todo-ATR/Yama/configYama.cfg";

yama_configuracion get_configuracion() {
	printf("Levantando archivo de configuracion del proceso Yama\n");
	yama_configuracion configuracion;
	// Obtiene el archivo de configuracion
	t_config* archivo_configuracion = config_create(path);
	configuracion.FS_IP = get_campo_config_string(archivo_configuracion, "FS_IP");
	configuracion.FS_PUERTO = get_campo_config_string(archivo_configuracion, "FS_PUERTO");
	configuracion.RETARDO_PLANIFICACION = get_campo_config_int(archivo_configuracion, "RETARDO_PLANIFICACION");
	configuracion.ALGORITMO_BALANCEO = get_campo_config_string(archivo_configuracion, "ALGORITMO_BALANCEO");
	return configuracion;
}
