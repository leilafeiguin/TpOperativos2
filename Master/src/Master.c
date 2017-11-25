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

int main(char* scriptTransf, char* scriptReduc, char* archivoOrigen, char* archivoDestino) {
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
		memcpy(worker->puerto, paqueteRecibido->data + desplazamiento, sizeof(int));
		desplazamiento+= sizeof(int);

		int cantidadBloques = 0;
		memcpy(&cantidadBloques, paqueteRecibido->data + desplazamiento, sizeof(int));

		int j=0;
		for(j=0;j<cantidadBloques;j++){
			t_infobloque* infoBloque = malloc(sizeof(t_infobloque));

			memcpy(infoBloque->bloqueAbsoluto, paqueteRecibido->data + desplazamiento, sizeof(int));
			desplazamiento+= sizeof(int);
			memcpy(infoBloque->finBloque, paqueteRecibido->data + desplazamiento, sizeof(int));
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
		pthread_create(NULL,NULL, hiloWorker, elem);
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

void hiloWorker(void* infoWorker){
	t_clock* worker=(t_clock*)infoWorker;
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
