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

un_socket yamaSocket;
t_log* logger;
char* SCRIPT_TRANSF;
char* SCRIPT_REDUC;
char* ARCHIVO_ORIGEN;
char* PATH_ARCHIVO_ORIGEN;
int main(int argc, char** argv) {
	imprimir("/home/utnso/Desktop/tp-2017-2c-Todo-ATR/Libraries/master.txt");

	char* scriptTransf = "transformador.py";
	char* scriptReduc = "reductor.py";
    char* archivoOrigen = "yamafs://nombres.csv";
	char* archivoDestino = "yamafs://hellou.csv";


//	char* scriptTransf = argv[0];
//	char* scriptReduc = argv[1];
//    char* archivoOrigen = argv[2];
//	char* archivoDestino = argv[3];

	SCRIPT_TRANSF=scriptTransf;
	SCRIPT_REDUC=scriptReduc;
	ARCHIVO_ORIGEN=archivoOrigen;
	PATH_ARCHIVO_ORIGEN= str_replace(ARCHIVO_ORIGEN, "yamafs://", "");
	char* fileLog;
	fileLog = "MasterLogs.txt";
	logger = log_create(fileLog, "Master Logs", 1, 0);
	log_trace(logger, "Inicializando proceso Master \n");

	master_configuracion configuracion = get_configuracion();
	log_trace(logger, "Archivo de configuracion levantado. \n");

	//Se conecta al YAMA
	yamaSocket = conectar_a(configuracion.IP_YAMA,configuracion.PUERTO_YAMA);
	realizar_handshake(yamaSocket, cop_handshake_master);
	//Master avisa a yama sobre que archivo del FS necesita ejecutar
	enviar(yamaSocket,cop_master_archivo_a_transformar,strlen(archivoOrigen)+1,archivoOrigen);
	log_trace(logger, "Recibi datos de Yama \n");
	while(1)
	{
		t_paquete* paqueteRecibido = recibir(yamaSocket);
			if(paqueteRecibido->codigo_operacion == -1){
				log_trace(logger,"Se cayo Yama, finaliza Master.\n");
				exit(-1);
			}else if(paqueteRecibido->codigo_operacion == -2){
				log_trace(logger,"El path ingresado no existe en el FS.\n");
				exit(-2);
			}else if(paqueteRecibido->codigo_operacion == cop_yama_lista_de_workers){
				t_archivoxnodo* archivoNodo= malloc(sizeof(t_archivoxnodo));
				archivoNodo->bloquesRelativos =  list_create();
				archivoNodo->workersAsignados= list_create();
				int cantidadWorkers = 0;
				int desplazamiento = 0;
				memcpy(&cantidadWorkers, paqueteRecibido->data + desplazamiento, sizeof(int));
				desplazamiento+=sizeof(int);
				int i=0;
				for(;i<cantidadWorkers;i++){
					t_clock* worker = malloc(sizeof(t_clock));

					worker->bloques = list_create();

					int longitudWorkerId;
					memcpy(&longitudWorkerId, paqueteRecibido->data + desplazamiento, sizeof(int));
					desplazamiento+= sizeof(int);

					worker->worker_id = malloc(longitudWorkerId);

					memcpy(worker->worker_id, paqueteRecibido->data + desplazamiento, longitudWorkerId);
					desplazamiento+= longitudWorkerId;

					int longitudIP;
					memcpy(&longitudIP, paqueteRecibido->data + desplazamiento, sizeof(int));
					desplazamiento+= sizeof(int);

					worker->ip = malloc(longitudIP);

					memcpy(worker->ip, paqueteRecibido->data + desplazamiento, longitudIP);
					desplazamiento+= longitudIP;

					memcpy(&worker->puerto, paqueteRecibido->data + desplazamiento, sizeof(int));
					desplazamiento+= sizeof(int);

					int cantidadBloques = 0;
					memcpy(&cantidadBloques, paqueteRecibido->data + desplazamiento, sizeof(int));
					desplazamiento+= sizeof(int);

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
					t_clock* infoWorker = elem;
					parametros->infoWorker = infoWorker;
					parametros->yama_socket = yamaSocket;
					pthread_t pth;
					pthread_create(&pth,NULL, hiloWorker, parametros);
				}
				list_iterate(archivoNodo->workersAsignados, iniciarHiloWorker);
			}else if(paqueteRecibido->codigo_operacion == cop_yama_inicio_reduccion_local){
				t_hilo_reduccion_local* hilo_reduc_local = malloc(sizeof(t_hilo_reduccion_local));
				hilo_reduc_local->yamaSocket = yamaSocket;
				hilo_reduc_local->listaTemp = list_create();
				int longitudIdWorker;
				int cantidadDeElementos;
				int longitudIp = 0;
				int desplazamiento = 0;

				memcpy(&longitudIp, paqueteRecibido->data + desplazamiento, sizeof(int));
				desplazamiento+=sizeof(int);

				hilo_reduc_local->ip =malloc(longitudIp+1);
				memcpy(hilo_reduc_local->ip, paqueteRecibido->data + desplazamiento, longitudIp);
				desplazamiento+=longitudIp;

				memcpy(&hilo_reduc_local->puerto, paqueteRecibido->data + desplazamiento, sizeof(int));
				desplazamiento+=sizeof(int);

				memcpy(&cantidadDeElementos, paqueteRecibido->data + desplazamiento, sizeof(int));
				desplazamiento+=sizeof(int);

				t_serializacionTemporal* serializacion;
				int i;
				for(i=0;i<cantidadDeElementos;i++){
					serializacion = malloc(sizeof(t_serializacionTemporal));//consultar
					memcpy(&serializacion->cantidadTemporal, paqueteRecibido->data + desplazamiento, sizeof(int));
					desplazamiento+= sizeof(int);
					serializacion->temporal = malloc(serializacion->cantidadTemporal);

					memcpy(serializacion->temporal, paqueteRecibido->data + desplazamiento, serializacion->cantidadTemporal);
					desplazamiento+= serializacion->cantidadTemporal;
					list_add(hilo_reduc_local->listaTemp, serializacion);
				}
				int cantidadTempDestino = 0;
				memcpy(&cantidadTempDestino, paqueteRecibido->data + desplazamiento, sizeof(int));
				desplazamiento+= sizeof(int);
				hilo_reduc_local->tempDestino =malloc(cantidadTempDestino+1);

				memcpy(hilo_reduc_local->tempDestino, paqueteRecibido->data + desplazamiento, cantidadTempDestino);
				desplazamiento+= cantidadTempDestino;

				memcpy(&longitudIdWorker, paqueteRecibido->data + desplazamiento, sizeof(int));
				desplazamiento +=sizeof(int);

				hilo_reduc_local->idWorker = malloc(longitudIdWorker+1);
				memcpy(hilo_reduc_local->idWorker, paqueteRecibido->data +desplazamiento, longitudIdWorker);
				desplazamiento +=longitudIdWorker;
				pthread_t pth_reduc;
				pthread_create(&pth_reduc,NULL,hilo_reduccion_local,hilo_reduc_local);
			}else if(paqueteRecibido->codigo_operacion == cop_yama_inicio_reduccion_global){
				pthread_t pth_glob;
				pthread_create(&pth_glob,NULL, hiloReduccionGlobal, paqueteRecibido);
			}
	}

	return EXIT_SUCCESS;
}

char *str_replace(char *orig, char *rep, char *with) {
		char *result; // the return string
		char *ins;    // the next insert point
		char *tmp;    // varies
		int len_rep;  // length of rep (the string to remove)
		int len_with; // length of with (the string to replace rep with)
		int len_front; // distance between rep and end of last rep
		int count;    // number of replacements
		// sanity checks and initialization
		if (!orig || !rep)
			return NULL;
		len_rep = strlen(rep);
		if (len_rep == 0)
			return NULL; // empty rep causes infinite loop during count
		if (!with)
			with = "";
		len_with = strlen(with);
		// count the number of replacements needed
		ins = orig;
		for (count = 0; (tmp = strstr(ins, rep)); ++count) {
			ins = tmp + len_rep;
		}
		tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);
		if (!result)
			return NULL;
		// first time through the loop, all the variable are set correctly
		// from here on,
		//    tmp points to the end of the result string
		//    ins points to the next occurrence of rep in orig
		//    orig points to the remainder of orig after "end of rep"
		while (count--) {
			ins = strstr(orig, rep);
			len_front = ins - orig;
			tmp = strncpy(tmp, orig, len_front) + len_front;
			tmp = strcpy(tmp, with) + len_with;
			orig += len_front + len_rep; // move to next "end of rep"
		}
		strcpy(tmp, orig);
		return result;
	}

void hiloReduccionGlobal(void* parametros){
	char* ipEncargado;
	int puertoEncargado;
	char* idEncargado;
	char* redGlobal;
	int longitudIp;
	int longitudId;
	int longitudRedGlobal;
	int cantidadWorkers;
	t_list* workers = list_create();
	int desplazamiento = 0;

	memcpy(&longitudIp, parametros+ desplazamiento, sizeof(int));
	desplazamiento+=sizeof(int);
	ipEncargado=malloc(longitudIp);

	memcpy(ipEncargado, parametros+ desplazamiento, longitudIp);
	desplazamiento+=longitudIp;

	memcpy(&puertoEncargado, parametros+ desplazamiento, sizeof(int));
	desplazamiento+=sizeof(int);

	memcpy(&longitudId, parametros+ desplazamiento, sizeof(int));
	desplazamiento+=sizeof(int);
    idEncargado=malloc(longitudId);

	memcpy(idEncargado, parametros+ desplazamiento, longitudId);
	desplazamiento+=longitudId;

	memcpy(&cantidadWorkers, parametros+ desplazamiento, sizeof(int));
	desplazamiento+=sizeof(int);

	int i;
	for(i=0;i<cantidadWorkers;i++){
		t_workerBloques* worker = malloc(sizeof(t_workerBloques));
		int cantidadIp;
		int cantidadRedLocal;
		int cantidadId;

		memcpy(&cantidadIp, parametros+ desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);

		memcpy(worker->ip, parametros+ desplazamiento, cantidadIp);
		desplazamiento+=cantidadIp;

		memcpy(&worker->puerto, parametros+ desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);

		memcpy(&cantidadRedLocal, parametros+ desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);

		memcpy(worker->archivoReduccionLocal, parametros+ desplazamiento, cantidadRedLocal);
		desplazamiento+=cantidadRedLocal;

		memcpy(&cantidadId, parametros+ desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);

		memcpy(worker->idWorker, parametros+ desplazamiento, cantidadId);
		desplazamiento+=cantidadId;

		list_add(workers, worker);
	}
	memcpy(&longitudRedGlobal, parametros+ desplazamiento, sizeof(int));
	desplazamiento+=sizeof(int);
	redGlobal=malloc(longitudRedGlobal);

	memcpy(redGlobal, parametros+ desplazamiento, longitudRedGlobal);
	desplazamiento+=longitudRedGlobal;

	int sizeLista=0;
	int j;
	for(j=0; j<cantidadWorkers;j++){
		t_workerBloques* workerGlobal = malloc(sizeof(t_workerBloques));
		workers = list_get(workers, j);
		sizeLista += sizeof(int) + strlen(workerGlobal->ip)+1 + sizeof(int) + sizeof(int) + strlen(workerGlobal->archivoReduccionLocal)+1 + sizeof(int) + strlen(workerGlobal->idWorker)+1;
	}
	void* bufferRG= malloc(sizeof(int) + sizeLista);
	int desplazamientoRG=0;
	memcpy(bufferRG+desplazamientoRG, &cantidadWorkers, sizeof(int));
	desplazamientoRG+=sizeof(int);
	int k;
	for(k=0; j<cantidadWorkers;k++){
		t_workerBloques* workerGlobal = malloc(sizeof(t_workerBloques));
		workerGlobal = list_get(workers, k);

		int longitudIpW = strlen(workerGlobal->ip)+1;
		memcpy(bufferRG+desplazamientoRG, &longitudIpW, sizeof(int));
		desplazamientoRG+=sizeof(int);

		memcpy(bufferRG+desplazamientoRG, workerGlobal->ip, longitudIpW);
		desplazamientoRG+=longitudIpW;

		memcpy(bufferRG+desplazamientoRG, &workerGlobal->puerto, sizeof(int));
		desplazamientoRG+=sizeof(int);

		int longitudArchivo = strlen(workerGlobal->archivoReduccionLocal)+1;
		memcpy(bufferRG+desplazamientoRG, &longitudArchivo, sizeof(int));
		desplazamientoRG+=sizeof(int);

		memcpy(bufferRG+desplazamientoRG, workerGlobal->archivoReduccionLocal, longitudArchivo);
		desplazamientoRG+=longitudArchivo;

		int longitudIdW = strlen(workerGlobal->idWorker)+1;
		memcpy(bufferRG+desplazamientoRG, &longitudIdW, sizeof(int));
		desplazamientoRG+=sizeof(int);

		memcpy(bufferRG+desplazamientoRG, workerGlobal->idWorker, longitudArchivo);
		desplazamientoRG+=longitudIdW;
	}
	memcpy(bufferRG+desplazamientoRG, &longitudRedGlobal, sizeof(int));
	desplazamientoRG+=sizeof(int);

	memcpy(bufferRG+desplazamientoRG, redGlobal, longitudRedGlobal);
	desplazamientoRG+=longitudRedGlobal;

	un_socket socketRedGlobal = conectar_a(ipEncargado,string_from_format("%i",puertoEncargado));
	realizar_handshake(socketRedGlobal, cop_handshake_master);
	enviar(socketRedGlobal,cop_worker_reduccionGlobal,desplazamiento,bufferRG);
	t_paquete* paquete_recibido = recibir(socketRedGlobal);
	enviar(yamaSocket,cop_master_finalizado,paquete_recibido->tamanio,paquete_recibido->data);
}

void hiloWorker(void* parametros){
	t_parametrosHiloWorker* parametrosCasteado = (t_parametrosHiloWorker*) parametros;
	t_clock* worker=parametrosCasteado->infoWorker;
	un_socket yamaSocket = parametrosCasteado->yama_socket;
	int desplazamiento=0;
	FILE *fileIN;
	fileIN = fopen(SCRIPT_TRANSF, "rb");
	if(!fileIN){
		log_trace(logger, "No se puede abrir el archivo.\n");
		exit(-1);
	}
	struct stat st_script;
	stat(SCRIPT_TRANSF, &st_script);
	void* bufferScript = malloc (st_script.st_size+1);
	int MAX_LINE=4096;
	char singleline[MAX_LINE];
	while(fgets(singleline, MAX_LINE, fileIN) != NULL){
		strcat(bufferScript, singleline);
	}
	int cantElementos = list_size(worker->bloques);

	//Leer el archivo de script que corresponda segun la etapa (transformacion o reduccion)
	//Hacer un strlen+1 del buffer donde tenemos el contenido del archivo
	void* buffer= malloc(sizeof(int) + cantElementos * (sizeof(int)+ strlen(bufferScript)+1 + sizeof(int)+ 11+ sizeof(int)));

	memcpy(buffer+desplazamiento, &cantElementos, sizeof(int));
	desplazamiento+=sizeof(int);
	void infoBloque(void* bloque){
		t_infobloque* infobloque=((t_infobloque*)bloque);

		int longitudScript=strlen(bufferScript)+1;
		memcpy(buffer+desplazamiento, &longitudScript, sizeof(int));
		desplazamiento+=sizeof(int);

		memcpy(buffer+desplazamiento, bufferScript, longitudScript);
		desplazamiento+=longitudScript;

		memcpy(buffer+desplazamiento, &infobloque->bloqueAbsoluto, sizeof(int));
		desplazamiento+=sizeof(int);

		memcpy(buffer+desplazamiento, infobloque->dirTemporal, 11);
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
	t_estado_master estado ;
	if(paqueteRecibido->codigo_operacion == -1){
		log_trace(logger, "Se desconecto el nodo .\n");
		mensaje= "ERROR: se desconecto el nodo";
		estado=error;
	}
	else{
		mensaje= "Todo ok";
		estado=finalizado;
	}
	int desplazamiento2 = 0;
	int longitudIdWorker = strlen(worker->worker_id) +1;
	int longitudIdArchivo = strlen(PATH_ARCHIVO_ORIGEN) + 1;
	int longitudMensaje = strlen(mensaje) + 1;
	void* estadoWorker= malloc(sizeof(int) + longitudIdWorker + sizeof(int) + longitudIdArchivo + sizeof(t_estado_master) + sizeof(int) + longitudMensaje );

	memcpy(estadoWorker+desplazamiento2, &longitudIdWorker, sizeof(int));
	desplazamiento2 += sizeof(int);

	memcpy(estadoWorker+desplazamiento2, worker->worker_id, longitudIdWorker);
	desplazamiento2 += longitudIdWorker;

	memcpy(estadoWorker+desplazamiento2, &longitudIdArchivo, sizeof(int));
	desplazamiento2 += sizeof(int);

	memcpy(estadoWorker+desplazamiento2, PATH_ARCHIVO_ORIGEN, longitudIdArchivo);
	desplazamiento2 += longitudIdArchivo;

	memcpy(estadoWorker+desplazamiento2, &estado, sizeof(t_estado_master));
	desplazamiento2 += sizeof(t_estado_master);

	memcpy(estadoWorker+desplazamiento2, &longitudMensaje , sizeof(int));
	desplazamiento2 += sizeof(int);

	memcpy(estadoWorker+desplazamiento2, mensaje , longitudMensaje);
	desplazamiento2 += longitudMensaje;
	printf("Envio a yama estado del worker %s", worker->worker_id);
	enviar(yamaSocket, cop_master_estados_workers,desplazamiento2,estadoWorker);
	free(estadoWorker);
}

void hilo_reduccion_local(void* parametros){
	t_hilo_reduccion_local* estructura = (t_hilo_reduccion_local*) parametros;
	un_socket socketWorker;
	char* puertoWorker = string_from_format("%i",estructura->puerto);
	socketWorker = conectar_a(estructura->ip, puertoWorker);
	int desplazamiento = 0;
	int suma= 0;
	void sumadorElementos(t_serializacionTemporal* Elem){
		suma += Elem->cantidadTemporal;
		return;
	}
	list_iterate(estructura->listaTemp, (void*)sumadorElementos);
	int cantidadDeElementos = list_size(estructura->listaTemp);
	FILE *fileIN;
	fileIN = fopen(SCRIPT_REDUC, "rb");
	if(!fileIN){
		log_trace(logger, "No se puede abrir el archivo.\n");
		exit(-1);
	}
	struct stat st_script;
	stat(SCRIPT_REDUC, &st_script);
	void* bufferScript = malloc (st_script.st_size+1);
	int MAX_LINE=4096;
	char singleline[MAX_LINE];
	while(fgets(singleline, MAX_LINE, fileIN) != NULL){
		strcat(bufferScript, singleline);
	}

	int longitudScript=strlen(bufferScript)+1;

	int tamanioData = sizeof(int)+suma + (cantidadDeElementos*sizeof(int))+sizeof(int) + strlen(estructura->tempDestino) +sizeof(int)+strlen(estructura->idWorker)+sizeof(int)+longitudScript;
	char*buffer = malloc(tamanioData);
	memcpy(buffer,&cantidadDeElementos,sizeof(int));
	desplazamiento +=sizeof(int);
	int i = 0;
	for(;i<cantidadDeElementos;i++){
		int aux = ((t_serializacionTemporal*)list_get(estructura->listaTemp,i))->cantidadTemporal;
		memcpy(buffer+desplazamiento, &aux,sizeof(int));
		desplazamiento += sizeof(int);

		memcpy(buffer+desplazamiento, ((t_serializacionTemporal*)list_get(estructura->listaTemp,i))->temporal,aux);
		desplazamiento +=aux;
	}
	int cantidadTempDestino = strlen(estructura->tempDestino);
	memcpy(buffer+desplazamiento,&cantidadTempDestino,sizeof(int));
	desplazamiento +=sizeof(int);

	memcpy(buffer+desplazamiento,estructura->tempDestino,cantidadTempDestino);
	desplazamiento +=cantidadTempDestino;

	int longitudIdWorker2 = strlen(estructura->idWorker);
	memcpy(buffer+desplazamiento, &longitudIdWorker2, sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(buffer+desplazamiento, estructura->idWorker, longitudIdWorker2);
	desplazamiento += longitudIdWorker2;

	memcpy(buffer+desplazamiento, &longitudScript, sizeof(int));
	desplazamiento+=sizeof(int);

	memcpy(buffer+desplazamiento, bufferScript, longitudScript);
	desplazamiento+=longitudScript;

	enviar(socketWorker, cop_worker_reduccionLocal,tamanioData,buffer);


	free(buffer);
	t_paquete* paqueteRecibido;
	paqueteRecibido = recibir(socketWorker);
	//Deserializacion
	int longIp;
	char*ipW;
	int puerto;
	bool resultado;
	int longIdWorker;
	char* idWorker;
	desplazamiento = 0;

	memcpy(&longIp,paqueteRecibido->data+desplazamiento,sizeof(int));
	desplazamiento += sizeof(int);
	ipW = malloc(longIp);

	memcpy(ipW,paqueteRecibido->data +desplazamiento,longIp);
	desplazamiento+=longIp;

	memcpy(&puerto,paqueteRecibido->data+desplazamiento,sizeof(int));
	desplazamiento+=sizeof(int);

	memcpy(&resultado,paqueteRecibido->data+desplazamiento,sizeof(bool));
	desplazamiento+=sizeof(bool);

	memcpy(&longIdWorker,paqueteRecibido->data+desplazamiento,sizeof(int));
	desplazamiento+=sizeof(int);
	idWorker=malloc(longIdWorker);

	memcpy(idWorker,paqueteRecibido->data+desplazamiento,longIdWorker);
	desplazamiento+=longIdWorker;

	//Serializacion
	int longitudIdWorker;
	char* worker_id;
	int	 longitudIdArchivo;
	char*	archivo;
	t_estado_master 	estado;
	int		longitudMensaje;
	char* 	mensaje;
	char* buffer1 =malloc(longitudIdWorker + sizeof(int) + longitudIdArchivo + sizeof(t_estado_master) + sizeof(int) + longitudMensaje);
	desplazamiento = 0;

	memcpy(buffer1+desplazamiento,&longitudIdWorker,sizeof(int));
	desplazamiento += sizeof(int);
	worker_id=malloc(longitudIdWorker);

	memcpy(buffer1+desplazamiento,estructura->idWorker, longitudIdWorker);
	desplazamiento += longitudIdWorker;

	memcpy(buffer1+desplazamiento, &longitudIdArchivo, sizeof(int));
	desplazamiento+=sizeof(int);
	archivo=malloc(longitudIdArchivo);

	memcpy(buffer1+desplazamiento, archivo,longitudIdArchivo);
	desplazamiento+=longitudIdArchivo;

	memcpy(buffer1+desplazamiento, &estado,sizeof(t_estado_master));
	desplazamiento +=sizeof(t_estado_master);

	memcpy(buffer1+desplazamiento, &longitudMensaje,sizeof(int));
	desplazamiento +=sizeof(int);

	mensaje=malloc(longitudMensaje);
	memcpy(buffer1+desplazamiento,mensaje,longitudMensaje);
	desplazamiento += longitudMensaje;
	enviar(estructura->yamaSocket,cop_master_estado_reduccion_local,desplazamiento,buffer1);
	free(buffer1);
	free(worker_id);
	free(archivo);
	free(mensaje);
	free(puertoWorker);
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
