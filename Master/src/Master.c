/*
 ============================================================================
 Name        : Master.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "Master.h"
#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>
#include "socketConfig.h"

char* SCRIPT_TRANSF;
char* SCRIPT_REDUC;
char* ARCHIVO_ORIGEN;

int main(int argc, char** argv) {

	char* scriptTransf = argv[0];
	char* scriptReduc = argv[1];
    char* archivoOrigen= argv[2];
	char* archivoDestino=argv[3];
	SCRIPT_TRANSF=scriptTransf;
	SCRIPT_REDUC=scriptReduc;
	ARCHIVO_ORIGEN=archivoOrigen;

	t_log* logger;
	char* fileLog;
	fileLog = "MasterLogs.txt";

	printf("Inicializando proceso Master\n");
	logger = log_create(fileLog, "Master Logs", 0, 0);
	log_trace(logger, "Inicializando proceso Master");

	master_configuracion configuracion = get_configuracion();
	log_trace(logger, "Archivo de configuracion levantado");

	//Se conecta al YAMA
	un_socket yamaSocket = conectar_a(configuracion.IP_YAMA,configuracion.PUERTO_YAMA);
	realizar_handshake(yamaSocket, cop_handshake_master);
	//Master avisa a yama sobre que archivo del FS necesita ejecutar

	enviar(yamaSocket,cop_master_archivo_a_transformar,sizeof(char*)*strlen(archivoOrigen),archivoOrigen);

	log_trace(logger, "Recibi datos de workers de Yama");
	t_paquete* paqueteRecibido = recibir(yamaSocket);

	if(paqueteRecibido->codigo_operacion == -1){
		printf("Se cayo Yama, finaliza Master.\n");
		exit(-1);
	}
	t_archivoxnodo* archivoNodo= malloc(sizeof(t_archivoxnodo));
	archivoNodo->bloquesRelativos =  list_create();
	archivoNodo->workersAsignados= list_create();

	int i=0;
	int cantidadWorkers = 0;
	int desplazamiento = 0;
	memcpy(&cantidadWorkers, paqueteRecibido->data + desplazamiento, sizeof(int));
	desplazamiento+=sizeof(int);

	for(i=0;i<cantidadWorkers;i++){
		t_clock* worker = malloc(sizeof(t_clock));
		memcpy(worker->ip, paqueteRecibido->data + desplazamiento, 15);
		desplazamiento+= 15;
		memcpy(&worker->puerto, paqueteRecibido->data + desplazamiento, sizeof(int));
		desplazamiento+= sizeof(int);

		int cantidadBloques = 0;
		memcpy(&cantidadBloques, paqueteRecibido->data + desplazamiento, sizeof(int));

		int j=0;
		for(j=0;j<cantidadBloques;j++){
			t_infobloque* infoBloque = malloc(sizeof(t_infobloque));

			memcpy(&infoBloque->bloqueAbsoluto, paqueteRecibido->data + desplazamiento, sizeof(int));
			desplazamiento+= sizeof(int);
			memcpy(&infoBloque->finBloque, paqueteRecibido->data + desplazamiento, sizeof(int));
			desplazamiento+= sizeof(int);

			infoBloque->dirTemporal = malloc(11);
			memcpy(infoBloque->dirTemporal, paqueteRecibido->data + desplazamiento, 11);
			desplazamiento+= 11;

			// agregar a la lista de bloques list_add(archivoNodo->bloquesRelativos, worker);
			list_add(worker->bloques, infoBloque);
		}
		// agregar a la lista de workers list_add(archivoNodo->bloquesRelativos, worker);
		list_add(archivoNodo->workersAsignados, worker);
	}

	void iniciarHiloWorker(void* elem){
		t_parametrosHiloWorker* parametros = malloc(sizeof(t_parametrosHiloWorker));
		t_clock* infoWorker = malloc(sizeof(t_clock));
		infoWorker = elem;
		parametros->infoWorker = infoWorker;
		parametros->yama_socket = yamaSocket;
		pthread_create(NULL,NULL, hiloWorker, parametros);
	}

	list_iterate(archivoNodo->workersAsignados, iniciarHiloWorker);

	//	NO BORRAR SIRVE PARA PROBAR CONEXIONES AL WORKER
	//	un_socket w1 = conectar_a("127.0.0.1","5050");
	//
	//	un_socket w2 = conectar_a("127.0.0.1","5050");
	//
	//	un_socket w3 = conectar_a("127.0.0.1","5050");
	//
	//	realizar_handshake(w1, cop_handshake_master);
	//	realizar_handshake(w2, cop_handshake_master);
	//	realizar_handshake(w3, cop_handshake_master);

	//list_map(workers, );
	//agregandolos a las estructuras correspondientes

	//realizar_handshake(worker1, cop_handshake_master);

	//si falla hay que capturarlo y mandarlo a YAMA

	log_trace(logger, "Respondo a Yama estado de conexiones con workers");
	char* estado_worker1 = "Ok";
	enviar(yamaSocket, cop_master_estados_workers, sizeof(char*)*strlen(estado_worker1), estado_worker1);

	//hacer un if para saber si pasa a etapa de transformacion o si hubo error esperar nuevos workers

	return EXIT_SUCCESS;
}


void hiloWorker(void* parametros){
	t_parametrosHiloWorker* parametrosCasteado = (t_parametrosHiloWorker*) parametros;
	t_clock* worker=parametrosCasteado->infoWorker;
	un_socket yamaSocket = parametrosCasteado->yama_socket;
	int desplazamiento=0;
	FILE *fileIN;
	fileIN = fopen(SCRIPT_TRANSF, "rb");
	if(!fileIN){
		printf("No se puede abrir el archivo.\n");
		exit(-1);
	}

	struct stat st_script;
	stat(SCRIPT_TRANSF, &st_script);
	void* bufferScript = malloc (st_script.st_size);

	int MAX_LINE=4096;
	char singleline[MAX_LINE];
	while(fgets(singleline, MAX_LINE, fileIN) != NULL){
		strcat(bufferScript, singleline);
	}

	int cantElementos = list_size(worker->bloques);

	//Leer el archivo de script que corresponda segun la etapa (transformacion o reduccion)
	//Hacer un strlen+1 del buffer donde tenemos el contenido del archivo
	void* buffer= malloc(sizeof(int) + cantElementos * (sizeof(int)+ strlen(bufferScript)+1 + sizeof(int)+ 11+ sizeof(int)));

	void infoBloque(void* bloque){
		t_infobloque* infobloque=((t_infobloque*)bloque);

		struct stat st;

		memcpy(buffer+desplazamiento, &st.st_size, sizeof(int));
		desplazamiento+=sizeof(int);

		memcpy(buffer+desplazamiento, bufferScript, strlen(bufferScript)+1);
		desplazamiento+=strlen(bufferScript)+1;

		memcpy(buffer+desplazamiento, &infobloque->bloqueAbsoluto, sizeof(int));
		desplazamiento+=sizeof(int);

		memcpy(buffer+desplazamiento, &infobloque->dirTemporal, 11);
		desplazamiento+=11;

		memcpy(buffer+desplazamiento, &infobloque->finBloque, sizeof(int));
		desplazamiento+=sizeof(int);
	}
	list_iterate(((t_clock*)worker)->bloques, infoBloque);

	char* puerto = malloc(10);

	snprintf(puerto , 10, "%i", worker->puerto);
	un_socket workerSocket = conectar_a(worker->ip, puerto);
	realizar_handshake(workerSocket, cop_handshake_master);
	enviar(workerSocket,cop_worker_transformacion,desplazamiento,buffer);
	t_paquete* paqueteRecibido = recibir(workerSocket);

	char* mensaje = "";
	if(paqueteRecibido->data == NULL){
		printf("Se desconecto el nodo /n");
		mensaje= "ERROR: se desconecto el nodo";
	}
	else{
		mensaje= "Todo ok";
	}

	//envio yama lei
	int desplazamiento2 = 0;
	int longitudIdWorker = strlen(worker->worker_id);
	int longitudIdArchivo = strlen(ARCHIVO_ORIGEN) + 1;
	int longitudMensaje = strlen(mensaje) + 1;

	void* buffer2= malloc(sizeof(int) + longitudIdWorker + sizeof(int) + longitudIdArchivo + sizeof(t_estado_master) + sizeof(int) + longitudMensaje );

	t_estado_master* estado = finalizado;


	memcpy(buffer2+desplazamiento2, &longitudIdWorker, sizeof(int));
	desplazamiento2 += sizeof(int);
	memcpy(buffer2+desplazamiento2, worker->worker_id, longitudIdWorker);
	desplazamiento2 += longitudIdWorker;
	memcpy(buffer2+desplazamiento2, &longitudIdArchivo, sizeof(int));
	desplazamiento2 += sizeof(int);
	memcpy(buffer2+desplazamiento2, ARCHIVO_ORIGEN, longitudIdArchivo);
	desplazamiento2 += longitudIdArchivo;
	memcpy(buffer2+desplazamiento2, estado, sizeof(t_estado_master));
	desplazamiento2 += sizeof(t_estado_master);
	memcpy(buffer2+desplazamiento2, &longitudMensaje , sizeof(int));
	desplazamiento2 += sizeof(int);
	memcpy(buffer2+desplazamiento2, mensaje , longitudMensaje);
	desplazamiento2 += longitudMensaje;

	enviar(yamaSocket, cop_master_archivo_a_transformar,desplazamiento2,buffer2);
}

size_t cantidadCaracterEnString(const char *str, char token)
{
  size_t count = 0;
  while(*str != '\0')
  {
    count += *str++ == token;
  }
  return count;
}

char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}
