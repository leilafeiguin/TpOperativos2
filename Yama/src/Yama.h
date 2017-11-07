#include <socketConfig.h>
#include <commons/log.h>
#include <commons/collections/list.h>

//ESTRUCTURA ARCHIVO CONFIGURACION
typedef struct yama_configuracion {
	char* IP_FS;
	char* PUERTO_FS;
	int RETARDO_PLANIFICACION;
	char* ALGORITMO_BALANCEO;
	int DISPONIBILIDAD_BASE;
} yama_configuracion;

typedef enum t_etapa {
	transformacion = 1,
	reduccionLocal = 2,
	reduccionGlobal = 3,
	almacenamientoFinal = 4
} t_etapa;

typedef enum t_estado {
	enProceso = 1,
	error = 2,
	finalizado = 3
} t_estado;

typedef struct t_job{
	int master;
	int nodo;
	int bloque;
	t_etapa etapa;
	int cantidadTemporal;
	char* temporal;
	t_estado estado;
} t_job;

typedef struct t_estados{
	int job;
	t_list* contenido;
} t_estados;

typedef struct t_clock{
	int disponibilidad;
	char* worker_id;
	char* ip;
	int puerto;
	t_list* bloques; //(int)
} t_clock;

typedef struct t_tabla_planificacion{
	t_link_element* clock_actual; //t_clock*
	t_list* workers; //(t_clock)*
}t_tabla_planificacion;


const char* path = "/home/utnso/Desktop/tp-2017-2c-Todo-ATR/Yama/configYama.cfg";

yama_configuracion get_configuracion() {
	printf("Levantando archivo de configuracion del proceso Yama\n");
	yama_configuracion configuracion;
	// Obtiene el archivo de configuracion
	t_config* archivo_configuracion = config_create(path);
	configuracion.IP_FS = get_campo_config_string(archivo_configuracion, "IP_FS");
	configuracion.PUERTO_FS = get_campo_config_string(archivo_configuracion, "PUERTO_FS");
	configuracion.RETARDO_PLANIFICACION = get_campo_config_int(archivo_configuracion, "RETARDO_PLANIFICACION");
	configuracion.ALGORITMO_BALANCEO = get_campo_config_string(archivo_configuracion, "ALGORITMO_BALANCEO");
	configuracion.DISPONIBILIDAD_BASE = get_campo_config_int(archivo_configuracion, "DISPONIBILIDAD_BASE");
	return configuracion;
}

void procesar_job(int job, t_job datos);

void sig_handler(int);

void* buscar_por_jobid(int jobId);

void* buscar_por_nodo (int nodo, t_list* listaNodos);

void setearJob(t_job* nuevoJob, t_job datos);

t_job* crearJob(t_job datos);

int availability();

void planificarBloque(t_tabla_planificacion tabla, int numeroBloque, t_archivoxnodo* bloquesxnodo);

char* generarDirectorioTemporal();
