#include <socketConfig.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <assert.h>


//ESTRUCTURA ARCHIVO CONFIGURACION
typedef struct master_configuracion {
	char* IP_YAMA;
	char* PUERTO_YAMA;
	char* PATH_ARCHIVO;
} master_configuracion;

const char* path = "/home/utnso/Desktop/tp-2017-2c-Todo-ATR/Master/configMaster.cfg";

master_configuracion get_configuracion() {
	printf("Levantando archivo de configuracion del proceso Worker\n");
	master_configuracion configuracion;
	// Obtiene el archivo de configuracion
	t_config* archivo_configuracion = config_create(path);
	configuracion.IP_YAMA = get_campo_config_string(archivo_configuracion, "IP_YAMA");
	configuracion.PUERTO_YAMA = get_campo_config_string(archivo_configuracion, "PUERTO_YAMA");
	configuracion.PATH_ARCHIVO = get_campo_config_string(archivo_configuracion, "PATH_ARCHIVO");
	return configuracion;
}

typedef struct t_worker{
	bool estado;
	char* puerto;
	char* IP;
	int socket;
}t_worker;

t_list* workers;

size_t cantidadCaracterEnString(const char *str, char token);
char** str_split(char* a_str, const char a_delim);
void iniciarHiloWorker(void* elem);
void hiloWorker(void* infoWorker);
