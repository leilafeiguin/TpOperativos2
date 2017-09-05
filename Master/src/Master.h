#include <socketConfig.h>
#include <commons/log.h>

//ESTRUCTURA ARCHIVO CONFIGURACION
typedef struct master_configuracion {
	char* IP_YAMA;
	char* PUERTO_YAMA;
} master_configuracion;

const char* path = "/home/utnso/Desktop/tp-2017-2c-Todo-ATR/Master/configMaster.cfg";

master_configuracion get_configuracion() {
	printf("Levantando archivo de configuracion del proceso Worker\n");
	master_configuracion configuracion;
	// Obtiene el archivo de configuracion
	t_config* archivo_configuracion = config_create(path);
	configuracion.IP_YAMA = get_campo_config_string(archivo_configuracion, "IP_YAMA");
	configuracion.PUERTO_YAMA = get_campo_config_string(archivo_configuracion, "PUERTO_YAMA");
	return configuracion;
}
