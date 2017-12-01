#include <socketConfig.h>
#include <commons/log.h>
#include <time.h>
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

typedef enum t_estado_yama {
	enProceso = 1,
	error = 2,
	finalizado = 3
} t_estado_yama;

typedef struct t_tabla_planificacion{
	t_link_element* clock_actual; //t_clock*
	t_list* workers; //(t_clock)*
	char* archivo;
	t_archivoxnodo* archivoNodo;
}t_tabla_planificacion;

typedef struct t_job{
	char* worker_id;//nodo id
	char* ip;
	int puerto;
	int bloque;
	t_etapa etapa;
	char* temporalTransformacion;
	char* temporalReduccionLocal;
	char* temporalReduccionGlobal;
	t_estado_yama estado;
	t_tabla_planificacion* planificacion;
	time_t tiempoInicio;
	time_t tiempoFinal;
} t_job;

typedef struct t_estados{
	char* archivo;
	int socketMaster;
	t_list* contenido;
	int numeroFallos;
	time_t tiempoInicio;
} t_estados;



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

void* buscar_por_nodo (char* nodo, t_list* listaNodos);

void setearJob(t_job* nuevoJob, t_job datos);

t_job* crearJob(t_job datos);

int availability();

void planificarBloque(t_tabla_planificacion* tabla, int numeroBloque, t_archivoxnodo* archivo, t_estados* estadoxjob, char* workerIdCaido);

char* generarDirectorioTemporal();

int CalcularDisponibilidad(t_clock* worker, t_tabla_planificacion* tabla);

int CalcularWLMax(t_tabla_planificacion* tabla);

int CalcularWLWorker(t_clock* worker);

char *randstring(size_t length);

char *str_replace(char *orig, char *rep, char *with);
