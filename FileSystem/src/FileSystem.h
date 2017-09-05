#include <socketConfig.h>
#include <commons/log.h>

//ESTRUCTURA ARCHIVO CONFIGURACION
typedef struct fileSystem_configuracion {
	char* IP_DATANODE;
	char* PUERTO_DATANODE;
} fileSystem_configuracion;

const char* path = "/home/utnso/Desktop/tp-2017-2c-Todo-ATR/FileSystem/configFileSystem.cfg";

fileSystem_configuracion get_configuracion() {
	printf("Levantando archivo de configuracion del proceso FileSystem\n");
	fileSystem_configuracion configuracion;
	// Obtiene el archivo de configuracion
	t_config* archivo_configuracion = config_create(path);
	configuracion.IP_DATANODE = get_campo_config_string(archivo_configuracion, "IP_DATANODE");
	configuracion.PUERTO_DATANODE = get_campo_config_string(archivo_configuracion, "PUERTO_DATANODE");
	return configuracion;
}
