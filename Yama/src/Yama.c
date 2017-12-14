/*
 ============================================================================
 Name        : Yama.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style

 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include "Yama.h"

t_list* tabla_estados;
yama_configuracion configuracion;
t_log* logger;
t_list* masters; //t_socket_archivo
int main(void) {
	imprimir("image.txt");
	masters= list_create();
	int socketFS;
	char* fileLog;
	fileLog = "YamaLogs.txt";
	tabla_estados= list_create();
	printf("Inicializando proceso Yama\n");
	logger = log_create(fileLog, "Yama Logs", 1, 0);
	log_trace(logger, "Inicializando proceso Yama");
    configuracion = get_configuracion();
	log_trace(logger, "Archivo de configuracion levantado");
	t_list* tablas_planificacion= list_create();
	fd_set master;    // master file descriptor list
	fd_set read_fds;  // temp file descriptor list for select()
	int fd_max;        // maximum file descriptor number
	int listener;     // listening socket descriptor
	int newfd;        // newly accept()ed socket descriptor
	struct sockaddr_storage remoteaddr; // client address
	socklen_t addrlen;
	char buf[256];    // buffer for client data
	int nbytes;
	char remoteIP[INET6_ADDRSTRLEN];
	int yes=1;        // for setsockopt() SO_REUSEADDR, below
	int i, j, rv;
	struct addrinfo hints, *ai, *p;
	FD_ZERO(&master);    // clear the master and temp sets
	FD_ZERO(&read_fds);
	// get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(NULL, "9999", &hints, &ai)) != 0) {
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}
	for(p = ai; p != NULL; p = p->ai_next) {
		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0) {
			continue;
		}
		// lose the pesky "address already in use" error message
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}
			break;
		}
		// if we got here, it means we didn't get bound
		if (p == NULL) {
			fprintf(stderr, "selectserver: failed to bind\n");
			exit(2);
		}
		freeaddrinfo(ai); // all done with this
		// listen
		if (listen(listener, 10) == -1) {
			perror("listen");
			exit(3);
		}
		// add the listener to the master set
		FD_SET(listener, &master);
		// keep track of the biggest file descriptor
		fd_max = listener; // so far, it's this one

	 	//Iniciar hilo consola
		pthread_t hiloFileSystem;
		//pthread_create(&hiloFileSystem, NULL, hiloFileSystem_Consola);

		//Se conecta al FileSystem
		un_socket fileSystemSocket = conectar_a(configuracion.IP_FS,configuracion.PUERTO_FS);
		FD_SET(fileSystemSocket, &master);
		if(fileSystemSocket > listener){
			fd_max = fileSystemSocket;
		}else{
			fd_max = listener;
		}
		socketFS = fileSystemSocket;
		realizar_handshake(fileSystemSocket, cop_handshake_yama);
		//Falta pedir la info de los workers conectados todo mati e aca hay que hacer un recibir
		//Que pasa si le rechazan la conexion.
		int socketActual;
		int socketMaster;
		//CONEXIONES
		while(1){
			if (signal(SIGUSR1, sig_handler) == SIG_ERR){};
			read_fds = master;
			select(fd_max+1, &read_fds, NULL, NULL, NULL);
			for(socketActual = 0; socketActual <= fd_max; socketActual++) {
					if (FD_ISSET(socketActual, &read_fds)) {
						if (socketActual == listener) { //es una conexion nueva
							newfd = aceptar_conexion(socketActual);
							t_paquete* handshake = recibir(socketActual);
							FD_SET(newfd, &master); //Agregar al master SET
							if (newfd > fd_max) {    //Update el Maximo
								fd_max = newfd;
							}
							log_trace(logger, "Yama recibio una nueva conexion");
							free(handshake);
						//No es una nueva conexion -> Recibo el paquete
						} else {
							t_paquete* paqueteRecibido = recibir(socketActual);
							switch(paqueteRecibido->codigo_operacion){ //revisar validaciones de habilitados
							case cop_handshake_master:
								esperar_handshake(socketActual, paqueteRecibido, cop_handshake_master);
								t_socket_archivo* socketArchivo = malloc(sizeof(t_socket_archivo));
								socketArchivo->socket = socketActual;
								list_add(masters, socketArchivo);
							break;
							case cop_archivo_programa:
								enviar(fileSystemSocket, cop_archivo_programa,paqueteRecibido->tamanio ,paqueteRecibido->data);
								//recibir un archivo
							break;
							case cop_yama_info_fs:
							{
								socketFS = socketActual;
								t_tabla_planificacion* tabla= malloc(sizeof(t_tabla_planificacion));

								tabla->workers = list_create();

								//Deserializacion
								t_archivoxnodo* archivoNodo=malloc(sizeof(t_archivoxnodo));
								archivoNodo->bloquesRelativos =  list_create();
								archivoNodo->nodos =  list_create();
								archivoNodo->workersAsignados= list_create();

								int longitudNombre = 0;
								int desplazamiento = 0;
								memcpy(&longitudNombre, paqueteRecibido->data + desplazamiento, sizeof(int));
								desplazamiento+=sizeof(int);
								archivoNodo->pathArchivo= malloc(longitudNombre);

								memcpy(archivoNodo->pathArchivo, paqueteRecibido->data + desplazamiento, longitudNombre);
								desplazamiento+=longitudNombre;

								tabla->archivo = string_duplicate(archivoNodo->pathArchivo);
								//Lista bloques relativos
								int cantidadElementosBloques = 0;
								memcpy(&cantidadElementosBloques ,paqueteRecibido->data + desplazamiento,sizeof(int));
								desplazamiento+= sizeof(int);

								bool esElArchivo(void* elem){
									return string_equals_ignore_case(((t_socket_archivo*)elem)->archivo,archivoNodo->pathArchivo);
								}
								t_socket_archivo* socketArchivo = list_find(masters, esElArchivo);
								if(cantidadElementosBloques == -2){
									//no encontro el archivo
									list_destroy(tabla->workers);
									list_destroy(archivoNodo->bloquesRelativos);
									list_destroy(archivoNodo->nodos);
									list_destroy(archivoNodo->workersAsignados);
									free(archivoNodo);
									free(tabla->archivo);
									free(tabla);
									enviar(socketArchivo->socket,-2,NULL,NULL);
								}else{
									list_add(tablas_planificacion, tabla);
									for(i=0;i<cantidadElementosBloques;i++){
										int* bloque = malloc(sizeof(int));
										memcpy(bloque, paqueteRecibido->data + desplazamiento, sizeof(int));
										desplazamiento+= sizeof(int);
										list_add(archivoNodo->bloquesRelativos, bloque);
									}

									//Lista nodos (t_nodoxbloques)
									int cantidadElementosNodos = 0;
									memcpy(&cantidadElementosNodos ,paqueteRecibido->data + desplazamiento,sizeof(int));
									desplazamiento+= sizeof(int);

									for(i=0;i<cantidadElementosNodos;i++){
										t_nodoxbloques* nodoBloques = malloc(sizeof(t_nodoxbloques));
										nodoBloques->bloques = list_create();
										int longitudNombreNodo = 0;
										memcpy(&longitudNombreNodo, paqueteRecibido->data + desplazamiento, sizeof(int));
										desplazamiento+=sizeof(int);

										char* idNodo= malloc(longitudNombreNodo);
										memcpy(idNodo ,paqueteRecibido->data + desplazamiento,longitudNombreNodo);
										desplazamiento+= longitudNombreNodo;
										nodoBloques->idNodo = idNodo;

										int longitudIp;
										memcpy(&longitudIp, paqueteRecibido->data + desplazamiento, sizeof(int));
										desplazamiento += sizeof(int);

										nodoBloques->ip =  malloc(longitudIp);
										memcpy(nodoBloques->ip, paqueteRecibido->data + desplazamiento, longitudIp);
										desplazamiento += longitudIp;

										memcpy(&nodoBloques->puerto, paqueteRecibido->data + desplazamiento, sizeof(int));
										desplazamiento += sizeof(int);
										//desplazamiento += sizeof(int);//me vuelvo a desplazar por el tamanio ya que lo ignoro

										//cantidad elementos lista bloques (t_infobloque)
										int cantidadElementos = 0;
										memcpy(&cantidadElementos ,paqueteRecibido->data + desplazamiento,sizeof(int));
										desplazamiento+= sizeof(int);
										int j=0;
										for(j=0;j<cantidadElementos;j++){
											t_infobloque* infoBloque = malloc(sizeof(t_infobloque));
											int bloqueAbsoluto = 0;
											memcpy(&bloqueAbsoluto, paqueteRecibido->data + desplazamiento, sizeof(int));
											desplazamiento+=sizeof(int);
											infoBloque->bloqueAbsoluto = bloqueAbsoluto;

											int bloqueRelativo = 0;
											memcpy(&bloqueRelativo, paqueteRecibido->data + desplazamiento, sizeof(int));
											desplazamiento+=sizeof(int);
											infoBloque->bloqueRelativo = bloqueRelativo;

											int finBloque = 0;
											memcpy(&finBloque, paqueteRecibido->data + desplazamiento, sizeof(int));
											desplazamiento+=sizeof(int);
											infoBloque->finBloque = finBloque;
											list_add(nodoBloques->bloques, infoBloque);
										}
										list_add(archivoNodo->nodos  ,nodoBloques);
									}

									void armarListaWorker(void* elem){
										t_nodoxbloques* nodoxbloque = ((t_nodoxbloques*)elem);
										t_clock* clock= malloc(sizeof(t_clock));
										clock->disponibilidad = configuracion.DISPONIBILIDAD_BASE;
										clock->worker_id = string_duplicate(nodoxbloque->idNodo);
										clock->ip = string_duplicate(nodoxbloque->ip);
										clock->puerto = nodoxbloque->puerto;
									 	clock->bloques = list_create();
										list_add(tabla->workers, clock );
									}

									list_iterate(archivoNodo->nodos, armarListaWorker);
									t_estados* estadosxjob=malloc(sizeof(t_estados));
									estadosxjob->archivo= string_duplicate(archivoNodo->pathArchivo);
									estadosxjob->socketMaster = socketActual;
									estadosxjob->contenido = list_create();
									tabla->archivoNodo=archivoNodo;
									tabla->clock_actual = tabla->workers->head;
									//Evalua y planifica en base al archivo que tiene que transaformar
									void planificarBloques(void* bloque){
										int* nroBloque = (int*)bloque;
										planificarBloque(tabla , *nroBloque, archivoNodo,estadosxjob, NULL);
										usleep(configuracion.RETARDO_PLANIFICACION * 1000);
									}
									list_add(tabla_estados, estadosxjob);
									list_iterate(archivoNodo->bloquesRelativos, planificarBloques);
									time(&estadosxjob->tiempoInicio);
									//Devuelve lista con los workers
									//Ahora lo debe sacar de archivoNodo workersAsignados
									int cantidadBloquesTotales =list_size(archivoNodo->bloquesRelativos);
									int cantidadWorkers = list_size(archivoNodo->workersAsignados);
									desplazamiento=0;

									int tamanioWorkers=0;
									void calcularTamanioWorkers(void* elem){
										 tamanioWorkers += (sizeof(int) + strlen(((t_clock*)elem)->ip)  + sizeof(int) + sizeof(int));
									}

									list_iterate(archivoNodo->workersAsignados, calcularTamanioWorkers);

									void* buffer= malloc(sizeof(int) + tamanioWorkers +cantidadBloquesTotales*(sizeof(int) + sizeof(int)+11));
									memcpy(buffer, &cantidadWorkers, sizeof(int)); //int que dice cuantos nodos hay en la lista
									desplazamiento+=sizeof(int);
									void datosWorker(void* worker){

										int longitudIP=strlen(((t_clock*)worker)->ip)+1;
										memcpy(buffer+desplazamiento, &longitudIP ,sizeof(int));
										desplazamiento+=sizeof(int);
										//por cada nodo mando IP (15)
										memcpy(buffer+desplazamiento, ((t_clock*)worker)->ip,longitudIP );
										desplazamiento+=longitudIP;

										//puerto (4)
										memcpy(buffer+desplazamiento, &((t_clock*)worker)->puerto,sizeof(int));
										desplazamiento+=sizeof(int);

										int cantidadBloques=list_size(((t_clock*)worker)->bloques);
										memcpy(buffer+desplazamiento,&cantidadBloques ,sizeof(int));
										desplazamiento+=sizeof(int);

										void datosBloques(void* bloque){
											//por cada bloque a pedir a cada worker mando
											t_infobloque* infobloque=((t_infobloque*)bloque);
											//int bloque absoluto (donde esta dentro del nodo)
											memcpy(buffer+desplazamiento, &infobloque->bloqueAbsoluto, sizeof(int));//todo ver seba como refactorizamos esto
											desplazamiento+=sizeof(int);

											//int fin de bloque
											memcpy(buffer+desplazamiento, &infobloque->finBloque, sizeof(int));
											desplazamiento+=sizeof(int);

											char* dirTemp=generarDirectorioTemporal();
											memcpy(buffer+desplazamiento,  dirTemp, 11);
											desplazamiento+=11;
										}
										list_iterate(((t_clock*)worker)->bloques, datosBloques);
									}
									list_iterate(archivoNodo->workersAsignados, datosWorker);
									enviar(socketArchivo->socket,cop_yama_lista_de_workers,desplazamiento,buffer);
								}
							}
							break;
							case cop_master_archivo_a_transformar:
							{
								log_trace(logger, "Recibi nuevo pedido de transformacion de un Master sobre X archivo");
								//Debe pedir al FS la composicion de bloques del archivo (por nodo)
								char* pathArchivo= malloc(paqueteRecibido->tamanio);
								memcpy(pathArchivo, paqueteRecibido->data, paqueteRecibido->tamanio);
								enviar(socketFS,cop_yama_info_fs,strlen(pathArchivo)+1,pathArchivo);

								void setearArchivo(void* elem){
									if(((t_socket_archivo*)elem)->socket == socketActual){
										((t_socket_archivo*)elem)->archivo = str_replace(pathArchivo, "yamafs://", "");
									}
								}

								list_iterate(masters, setearArchivo);
								free(pathArchivo);
							}
							break;
							case cop_master_estados_workers:
							{
								log_trace(logger, "Recibi estado de conexion de worker para proceso X");
								int desplazamiento = 0;
								int longitudIdWorker = 0;

								memcpy(&longitudIdWorker, paqueteRecibido->data + desplazamiento, sizeof(int));
								desplazamiento+=sizeof(int);
								char* idWorker = malloc(longitudIdWorker);

								memcpy(idWorker, paqueteRecibido->data + desplazamiento, longitudIdWorker);
								desplazamiento+=longitudIdWorker;

								int longitudIdArchivo = 0;
								memcpy(&longitudIdArchivo, paqueteRecibido->data + desplazamiento, sizeof(int));
								desplazamiento+=sizeof(int);
								char* idArchivo = malloc(longitudIdArchivo);

								memcpy(idArchivo, paqueteRecibido->data + desplazamiento, longitudIdArchivo);
								desplazamiento+=longitudIdArchivo;

								t_estado_yama estadoWorker;

								memcpy(&estadoWorker, paqueteRecibido->data + desplazamiento, sizeof(t_estado_yama));
								desplazamiento+=sizeof(t_estado_yama);

								int longitudMensaje = 0;
								memcpy(&longitudMensaje, paqueteRecibido->data + desplazamiento, sizeof(int));
								desplazamiento+=sizeof(int);
								char* mensaje = malloc(longitudMensaje);

								memcpy(mensaje, paqueteRecibido->data + desplazamiento, longitudMensaje);
								desplazamiento+=longitudMensaje;
								bool buscarXArchivoYMaster(void* elem){
									return string_equals_ignore_case(((t_estados*)elem)->archivo, idArchivo) && ((t_estados*)elem)->socketMaster == socketActual;
								}
								t_estados* estado=list_find(tabla_estados, buscarXArchivoYMaster);

								bool buscarWorker(void* elem){
									t_job* job= (t_job*)elem;
									return string_equals_ignore_case(job->worker_id, idWorker);
								}
								t_list* jobsModificados=list_filter(estado->contenido, buscarWorker);

								//Evaluar mensaje para saber si se cayeron nodos.
								if(estadoWorker == finalizado){
									t_list* nuevosJobs= list_create();
									t_request_reduccion_local* request = malloc(sizeof(t_request_reduccion_local));
									request->temporalesTransformacion = list_create();
									request->temporalReduccionLocal = string_from_format("/tmp/%i-%s-%s", socketActual, str_replace(idArchivo, "/","-"), randstring(5));
									void actualizarEstado(void* elem){
										t_job* job=(t_job*)elem;
										request->ip=job->ip;
										request->puerto = job->puerto;
										job->estado= finalizado;
										t_job* nuevoJob=malloc(sizeof(t_job));
										nuevoJob->bloque = job->bloque;
										nuevoJob->estado = enProceso;
										nuevoJob->planificacion = job->planificacion;
										nuevoJob->temporalTransformacion = job->temporalTransformacion;
										nuevoJob->temporalReduccionLocal = job->temporalReduccionLocal;
										nuevoJob->temporalReduccionGlobal = job->temporalReduccionGlobal;
										nuevoJob->worker_id = job->worker_id;
										time(&job->tiempoFinal);
										time(&nuevoJob->tiempoInicio);
										switch(job->etapa){
											case transformacion:
												nuevoJob->etapa = reduccionLocal;
												nuevoJob->temporalReduccionLocal = string_duplicate(request->temporalReduccionLocal);
												list_add(request->temporalesTransformacion, job->temporalTransformacion);
												break;
											case reduccionLocal:
												nuevoJob->etapa = reduccionGlobal;
												break;
											case reduccionGlobal:
												nuevoJob->etapa = almacenamientoFinal;
												break;
											case almacenamientoFinal:
												//Termino
												nuevoJob->etapa = almacenamientoFinal;
												break;
										}
										list_add(nuevosJobs, nuevoJob);
									}
									list_iterate(jobsModificados, actualizarEstado);
									list_add_all(nuevosJobs, estado->contenido);

									//Serializar y enviar request
									int cantidadElementosTemporales = list_size(request->temporalesTransformacion);
									int longitudTemporales = 0;
									for(i=0;i<cantidadElementosTemporales;i++){
										longitudTemporales += strlen(list_get(request->temporalesTransformacion,i))+1;
									}
									int longitudIdWorker;
									t_clock* worker;
									int desplazamientoRequest = 0;
									void* bufferRequest = malloc(sizeof(int) + strlen(request->ip)+1 + sizeof(int) + sizeof(int) + longitudTemporales + sizeof(int) + strlen(request->temporalReduccionLocal)+1+sizeof(int)+strlen(worker->worker_id)+1);
									//	longitudIp		ip						puerto		cantElementos	longitudTemporales	longitudTemp	temp
									int longitudIp = strlen(request->ip)+1;
									memcpy(bufferRequest, &longitudIp, sizeof(int));
									desplazamientoRequest+=sizeof(int);

									memcpy(bufferRequest, request->ip, longitudIp); //int que dice cuantos nodos hay en la lista
									desplazamientoRequest+=longitudIp;

									memcpy(bufferRequest, &request->puerto, sizeof(int));
									desplazamientoRequest+=sizeof(int);

									memcpy(bufferRequest, &cantidadElementosTemporales, sizeof(int));
									desplazamientoRequest+= sizeof(int);

									for(i=0;i<cantidadElementosTemporales;i++){
										int longitudTemporal = strlen(list_get(request->temporalesTransformacion,i))+1;
										char* temporal = malloc(longitudTemporal);
										memcpy(bufferRequest, &longitudTemporal, sizeof(int));
										memcpy(bufferRequest, temporal, sizeof(int));
										desplazamientoRequest+=longitudTemporal;
									}

									int longitudTemporalReduccionLocal = strlen(request->temporalReduccionLocal)+1;
									memcpy(bufferRequest, &longitudTemporalReduccionLocal, sizeof(int));
									desplazamientoRequest+=sizeof(int);

									memcpy(bufferRequest, request->temporalReduccionLocal, longitudTemporalReduccionLocal); //int que dice cuantos nodos hay en la lista
									desplazamientoRequest+=longitudTemporalReduccionLocal;

									memcpy(bufferRequest,&longitudIdWorker,sizeof(int));
									desplazamiento+=sizeof(int);

									memcpy(bufferRequest,worker->worker_id,longitudIdWorker);
									desplazamiento+= longitudIdWorker;

									enviar(socketActual,cop_yama_inicio_reduccion_local,desplazamiento,bufferRequest);
								}else{
									t_job* primerJob= list_get(jobsModificados,0);
									t_tabla_planificacion* tabla=primerJob->planificacion;
									t_archivoxnodo* archivoNodo=tabla->archivoNodo;
									list_clean(archivoNodo->workersAsignados);

									//Replanificar
									void replanificarBloques(void* elem){
										t_job* job = (t_job*)elem;
										job->estado = error;
										estado->numeroFallos++;
										planificarBloque(tabla , job->bloque, archivoNodo,estado, idWorker);
										usleep(configuracion.RETARDO_PLANIFICACION);
									}
									list_iterate(jobsModificados, replanificarBloques);
									int cantidadBloquesTotales =list_size(jobsModificados);
									int cantidadWorkers = list_size(archivoNodo->workersAsignados);
									desplazamiento=0;
									void* buffer= malloc(sizeof(int) + cantidadWorkers*(15+sizeof(int)+sizeof(int))+cantidadBloquesTotales*(sizeof(int) + sizeof(int)+15));
									memcpy(buffer, &cantidadWorkers, sizeof(int)); //int que dice cuantos nodos hay en la lista
									desplazamiento+=sizeof(int);

									void datosWorker(void* worker){
										//por cada nodo mando IP (15)
										memcpy(buffer+desplazamiento, ((t_clock*)worker)->ip, 15);
										desplazamiento+=15;

										//puerto (4)
										memcpy(buffer+desplazamiento, &((t_clock*)worker)->puerto,sizeof(int));
										desplazamiento+=sizeof(int);

										int cantidadBloques=list_size(((t_clock*)worker)->bloques);
										memcpy(buffer+desplazamiento,&cantidadBloques ,sizeof(int));
										desplazamiento+=sizeof(int);

										void datosBloques(void* bloque){
											//por cada bloque a pedir a cada worker mando

											t_infobloque* infobloque=((t_infobloque*)bloque);
											//int bloque absoluto (donde esta dentro del nodo)
											memcpy(buffer+desplazamiento, &infobloque->bloqueAbsoluto, sizeof(int));//todo ver seba como refactorizamos esto
											desplazamiento+=sizeof(int);

											memcpy(buffer+desplazamiento, &infobloque->finBloque, sizeof(int));
											desplazamiento+=sizeof(int);

											char* dirTemp=generarDirectorioTemporal();
											memcpy(buffer+desplazamiento,  dirTemp, strlen(dirTemp)+1);
											desplazamiento+=strlen(dirTemp)+1;
										}
										list_iterate(((t_clock*)worker)->bloques, datosBloques);
									}
									list_iterate(archivoNodo->workersAsignados, datosWorker);
									enviar(socketActual,cop_yama_lista_de_workers,desplazamiento,buffer);
								}
							}
								break;
							case cop_master_estado_reduccion_local:
							{
								//Recibo de Master
								int longitudIdWorker;
								char* workerId;
								int longitudIdArchivo;
								char* idArchivo;
								t_estado_yama estado;
								int longitudMensaje;
								char* mensaje;

								int desplazamientoRecibido = 0;
								memcpy(&longitudIdWorker, paqueteRecibido->data + desplazamientoRecibido, sizeof(int));
								desplazamientoRecibido+=sizeof(int);
								workerId = malloc(longitudIdWorker);

								memcpy(workerId, paqueteRecibido->data + desplazamientoRecibido, longitudIdWorker);
								desplazamientoRecibido+=longitudIdWorker;

								memcpy(&longitudIdArchivo, paqueteRecibido->data + desplazamientoRecibido, sizeof(int));
								desplazamientoRecibido+=sizeof(int);
								idArchivo = malloc(longitudIdArchivo);

								memcpy(idArchivo, paqueteRecibido->data + desplazamientoRecibido, longitudIdArchivo);
								desplazamientoRecibido+=longitudIdArchivo;

								memcpy(&estado, paqueteRecibido->data + desplazamientoRecibido, sizeof(int));
								desplazamientoRecibido+=sizeof(int);

								memcpy(&longitudMensaje, paqueteRecibido->data + desplazamientoRecibido, sizeof(int));
								desplazamientoRecibido+=sizeof(int);
								mensaje = malloc(longitudMensaje);

								memcpy(mensaje, paqueteRecibido->data + desplazamientoRecibido, longitudMensaje);
								desplazamientoRecibido+=longitudMensaje;

								bool buscarXArchivoYMaster(void* elem){
									return string_equals_ignore_case(((t_estados*)elem)->archivo, idArchivo) &&
											((t_estados*)elem)->socketMaster == socketActual;
								}
								t_estados* estados = list_find(tabla_estados, buscarXArchivoYMaster);

								bool esEtapaReduccionLocal(void* elem){
									return ((t_job*)elem)->etapa == reduccionLocal;
								}
								t_list* estadosReduccionLocal = list_filter(estados->contenido, esEtapaReduccionLocal);

								void actualizarTablaEstados(void* elem){
									((t_job*)elem)->estado = estado;
								}
								list_iterate(estadosReduccionLocal, actualizarTablaEstados);

								bool estaTerminado(void* elem){
									return ((t_job*)elem)->estado == finalizado;
								}
								if(list_all_satisfy(estadosReduccionLocal, estaTerminado)){
									t_list* listaWorkerBloques = list_create(); //t_workerBloques
									void llenarCantBloques(void* elem){
										void llenarCantBloquesWorker(void* elemAux){
											bool buscarXWorkerId(void* elemWorker)
											{
												return string_equals_ignore_case(((t_workerBloques*)elemWorker)->idWorker,((t_clock*)elemAux)->worker_id);
											}
											t_workerBloques* workerBloques = list_find(listaWorkerBloques, buscarXWorkerId);

											if(workerBloques == NULL){
												workerBloques= malloc(sizeof(t_workerBloques));
												workerBloques->idWorker = ((t_clock*)elemAux)->worker_id;
												workerBloques->ip = ((t_clock*)elemAux)->ip;
												workerBloques->puerto = ((t_clock*)elemAux)->puerto;
											}
											workerBloques->cantBloques += list_size(((t_clock*)elemAux)->bloques);
											list_add(listaWorkerBloques, workerBloques);
										}
										list_iterate(((t_tabla_planificacion*)elem)->workers, llenarCantBloquesWorker);
									}
									list_iterate(tablas_planificacion, llenarCantBloques);

									//Busco el minimo
									int min = 99999999999;
									char* ipEncargado = NULL;
									int puertoEncargado = NULL;
									char* idEncargado = NULL;

									void buscarMinimo(void* elemMin){
										if(((t_workerBloques*)elemMin)->cantBloques < min){
											min = ((t_workerBloques*)elemMin)->cantBloques;
											idEncargado = ((t_workerBloques*)elemMin)->idWorker;
											ipEncargado = ((t_workerBloques*)elemMin)->ip;
											puertoEncargado = ((t_workerBloques*)elemMin)->puerto;
										}
									}
									list_iterate(listaWorkerBloques, buscarMinimo);

									void* transformarWorkersJob(void* elem){
										t_job* job=(t_job*) elem;
										bool buscarXWorkerId(void* elemWorker)
										{
											return string_equals_ignore_case(((t_workerBloques*)elemWorker)->idWorker,job->worker_id);
										}
										t_workerBloques* worker= list_find(listaWorkerBloques, buscarXWorkerId);
										worker->archivoReduccionLocal = string_duplicate(job->temporalReduccionLocal);
										return worker;
									}
									t_list* workersGlobal=list_map(estados->contenido, transformarWorkersJob);
									char* temporalReduccionGlobal = generarDirectorioTemporal();
									int cantWorkersGlobal = list_size(workersGlobal);
									int sizeLista = 0;
									int i = 0;
									for(; i<cantWorkersGlobal;i++){
										t_workerBloques* workerGlobal = list_get(workersGlobal, i);
										sizeLista += sizeof(int) + strlen(workerGlobal->ip)+1 + sizeof(int) + sizeof(int) + strlen(workerGlobal->archivoReduccionLocal)+1 + sizeof(int) + strlen(workerGlobal->idWorker)+1;
												//		longitudIp		ip						puerto		longitudArchivo		archivo										longitudId
									}

									int desplazamientoRG = 0;
									void* bufferRG = malloc(sizeof(int) + strlen(ipEncargado)+1 + sizeof(int) + sizeof(int) + strlen(idEncargado)+1 + sizeof(int) + sizeLista);
														//	longitudIp		ip						puerto		longitudId		idEncargado				cantWorkers
									int longitudIp = strlen(ipEncargado)+1;
									memcpy(bufferRG, &longitudIp, sizeof(int));
									desplazamientoRG+=sizeof(int);

									memcpy(bufferRG, ipEncargado, longitudIp);
									desplazamientoRG+=longitudIp;

									memcpy(bufferRG, &puertoEncargado, sizeof(int));
									desplazamientoRG+=sizeof(int);

									int longitudId = strlen(idEncargado)+1;
									memcpy(bufferRG, &longitudId, sizeof(int));
									desplazamientoRG+=sizeof(int);

									memcpy(bufferRG, idEncargado, longitudId);
									desplazamientoRG+=longitudId;

									memcpy(bufferRG, &cantWorkersGlobal, sizeof(int));
									desplazamientoRG+=sizeof(int);

									int j = 0;
									for(; j<cantWorkersGlobal;j++){
										t_workerBloques* workerGlobal = list_get(workersGlobal, j);

										int longitudIpW = strlen(workerGlobal->ip)+1;
										memcpy(bufferRG, &longitudIpW, sizeof(int));
										desplazamientoRG+=sizeof(int);

										memcpy(bufferRG, workerGlobal->ip, longitudIpW);
										desplazamientoRG+=longitudIpW;

										memcpy(bufferRG, &workerGlobal->puerto, sizeof(int));
										desplazamientoRG+=sizeof(int);

										int longitudArchivo = strlen(workerGlobal->archivoReduccionLocal)+1;
										memcpy(bufferRG, &longitudArchivo, sizeof(int));
										desplazamientoRG+=sizeof(int);

										memcpy(bufferRG, workerGlobal->archivoReduccionLocal, longitudArchivo);
										desplazamientoRG+=longitudArchivo;

										int longitudIdW = strlen(workerGlobal->idWorker)+1;
										memcpy(bufferRG, &longitudIdW, sizeof(int));
										desplazamientoRG+=sizeof(int);

										memcpy(bufferRG, workerGlobal->idWorker, longitudArchivo);
										desplazamientoRG+=longitudIdW;
									}
									int longitudTemporalReduccionGlobal = strlen(temporalReduccionGlobal)+1;
									memcpy(bufferRG, &longitudTemporalReduccionGlobal, sizeof(int));
									desplazamientoRG+=sizeof(int);

									memcpy(bufferRG, temporalReduccionGlobal, longitudTemporalReduccionGlobal);
									desplazamientoRG+=longitudTemporalReduccionGlobal;
									enviar(socketActual,cop_yama_inicio_reduccion_global,desplazamientoRG,bufferRG);
								}
							}
								break;
							case cop_master_finalizado:
							{
								int cantidadArchivo;
								void* nombreArchivo;
								int tamanioArchivo;
								void* archivo;
								int desplazamiento=0;

								memcpy(&cantidadArchivo, paqueteRecibido->data + desplazamiento, sizeof(int));
								desplazamiento += sizeof(int);
								archivo= malloc(cantidadArchivo);

								memcpy(nombreArchivo, paqueteRecibido->data + desplazamiento, cantidadArchivo);
								desplazamiento += cantidadArchivo;

								memcpy(&tamanioArchivo, paqueteRecibido->data + desplazamiento, sizeof(int));
								desplazamiento += sizeof(int);
								archivo = malloc(tamanioArchivo);

								memcpy(archivo, paqueteRecibido->data + desplazamiento, cantidadArchivo);
								desplazamiento += tamanioArchivo;
								//deserializar lo que manda master y serializar primero socket actual (socket master) y
								void* buffer = malloc(sizeof(int) + sizeof(int) + cantidadArchivo + sizeof(int) + tamanioArchivo);
								int desplazamiento2 = 0;

								memcpy(&buffer + desplazamiento2, socketActual , sizeof(int));
								desplazamiento2 += sizeof(int);

								memcpy(&buffer + desplazamiento2, cantidadArchivo , sizeof(int));
								desplazamiento2 += sizeof(int);

								memcpy(&buffer + desplazamiento2, nombreArchivo, cantidadArchivo);
								desplazamiento2 += cantidadArchivo;

								memcpy(&buffer + desplazamiento2, tamanioArchivo , sizeof(int));
								desplazamiento2 += sizeof(int);

								memcpy(&buffer + desplazamiento2, archivo, tamanioArchivo);
								desplazamiento2 += tamanioArchivo;

								enviar(socketFS,cop_yama_finalizado, desplazamiento2, buffer);
							}
								break;
								case -1:
								{
									if(socketActual == socketFS){
										printf("Se cayo FS, finaliza Yama.\n");
										exit(-1);
									}else {

										bool buscarXSocket(void* elem){
											return ((t_socket_archivo*)elem)->socket == socketActual;
										}
										if(list_any_satisfy(masters, buscarXSocket)){

											bool buscarXSocket(void* elem){
												return ((t_socket_archivo*)elem)->socket == socketActual;
											}

											void destruir_socket_archivo(void* elem){
												free(((t_socket_archivo*)elem)->archivo);
												free(elem);
											}

											list_remove_and_destroy_by_condition(masters, buscarXSocket,destruir_socket_archivo);
											printf("Se cayo un Master. \n");
											t_tabla_planificacion* tabla;

											bool hayQueMarcarComoError(void* elem){
												return ((t_estados*)elem)->socketMaster == socketActual;
											}
											t_estados* jobsFinalizados = list_find(tabla_estados, hayQueMarcarComoError);

											void marcarComoError(void* elem){
												((t_job*)elem)->estado = error;
												tabla=((t_job*)elem)->planificacion;
											}

											if(jobsFinalizados != NULL){
												list_iterate(jobsFinalizados->contenido, marcarComoError);

												void eliminarJobsDelMaster(void* elem){

													t_clock* clock = (t_clock*)elem;
													free(clock->ip);
													free(clock->worker_id);

													void destruirBloques(void* elemAux){
														free(((t_infobloque*)elemAux)->dirTemporal);
														free(((t_infobloque*)elemAux));
													}

													list_destroy_and_destroy_elements(clock->bloques, destruirBloques);
													free(clock);
												}

												list_destroy_and_destroy_elements(tabla->workers, eliminarJobsDelMaster);
												free(tabla->archivo);

												void destruirBloques(void* elem){
													free(elem);
												}

												list_destroy_and_destroy_elements((tabla->archivoNodo->bloquesRelativos), destruirBloques);
												free(tabla->archivoNodo->pathArchivo);

												void destruirNodos (void* elem){
													free(((t_infobloque*)elem)->dirTemporal);
													free((t_infobloque*)elem);
												}

												list_destroy_and_destroy_elements(tabla->archivoNodo->nodos, destruirNodos);
												free(tabla->archivoNodo);
												free(tabla);

											}

											close(socketActual);
											FD_CLR(socketActual, &master);

										}
									}
								}
								break;
								case cop_yama_almacenado:
								{
									int longitudNombre;
									char* nombreArchivo;
									char* archivo;
									int socket;
									int desplazamiento=0;
									bool estado;

									memcpy(&socket,paqueteRecibido->data + desplazamiento ,sizeof(int));
									desplazamiento+=sizeof(int);

									memcpy(&longitudNombre, paqueteRecibido->data + desplazamiento ,sizeof(int));
									desplazamiento+=sizeof(int);
									nombreArchivo = malloc(longitudNombre);

									memcpy(nombreArchivo,paqueteRecibido->data + desplazamiento,longitudNombre);
									desplazamiento+=longitudNombre;

									memcpy(&estado,paqueteRecibido->data + desplazamiento,sizeof(bool));
									desplazamiento+=sizeof(bool);

									bool buscarXArchivoYMaster(void* elem){
										return string_equals_ignore_case(((t_estados*)elem)->archivo, archivo) &&
												((t_estados*)elem)->socketMaster == socketActual;
									}
									t_estados* estados = list_find(tabla_estados, buscarXArchivoYMaster);
									time_t tiempoFinal=time(NULL);

									bool buscarXSocket(void* elem){
										return ((t_socket_archivo*)elem)->socket == socketActual;
									}

									void destruir_socket_archivo(void* elem){
										free(((t_socket_archivo*)elem)->archivo);
										free(elem);
									}

									list_remove_and_destroy_by_condition(masters, buscarXSocket,destruir_socket_archivo);
									//Tiempo total de Ejecución del Job.
									double tiempoTotal = difftime(tiempoFinal, estados->tiempoInicio);

									//Tiempo de ejecucion en cada etapa.
									double tiempoTransformacion=0;
									double tiempoRL=0;
									double tiempoRG=0;

									t_tabla_planificacion* tabla;
									void calcularTiempos(void* elem){
										t_job* job = (t_job*)elem;
										if(job->estado == finalizado)
										{
											double diff= difftime(job->tiempoFinal,job->tiempoInicio);
											if(job->etapa == transformacion)
												tiempoTransformacion += diff;
											if(job->etapa == reduccionLocal)
												tiempoRL += diff;
											if(job->etapa == reduccionGlobal)
												tiempoRG += diff;
										}
										tabla = job->planificacion;
									}
									int cantidadBloques=list_size(tabla->archivoNodo->bloquesRelativos);
									//cant tareas en transf= cantidad bloques
									int cantTareasTransformacion = cantidadBloques;
									//cant tareas en reduc  = cant bloques
									int cantTareasRL = cantidadBloques;
									//cant tareas reduc glob = 1
									int cantTareasRG = 1;
									//Cant maxima de tareas en paralelo
									bool tieneAlMenosUnBloque(void* elem){
										return list_size(((t_clock*)elem)->bloques) >1;
									}
									int cantidadWorkersConAlMenosUnWorker= list_count_satisfying(tabla->workers, tieneAlMenosUnBloque);
									int cantMaxEnParalelo = cantidadWorkersConAlMenosUnWorker;
									//Cantidad de fallos obtenidos en la realización de un Job.
									int cantidadFallos = estados->numeroFallos;
									//orden
	//								double tiempoTotal;
	//								double tiempoTransformacion;
	//								double tiempoRL;
	//								double tiempoRG;
	//								int cantTareasTransformacion;
	//								int cantTareasRL;
	//								int cantTareasRG;
	//								int cantidadFallos;
	//								int cantMaxEnParalelo;

									void* buffer = malloc(sizeof(double) * 4 + sizeof(int) *5);
									desplazamiento=0;

									memcpy(buffer+desplazamiento, &tiempoTotal, sizeof(double));
									desplazamiento =+ sizeof(double);

									memcpy(buffer+desplazamiento, &tiempoTransformacion, sizeof(double));
									desplazamiento =+ sizeof(double);

									memcpy(buffer+desplazamiento, &tiempoRL, sizeof(double));
									desplazamiento =+ sizeof(double);

									memcpy(buffer+desplazamiento, &tiempoRG, sizeof(double));
									desplazamiento =+ sizeof(double);

									memcpy(buffer+desplazamiento, &cantTareasTransformacion, sizeof(int));
									desplazamiento =+ sizeof(int);

									memcpy(buffer+desplazamiento, &cantTareasRL, sizeof(int));
									desplazamiento =+ sizeof(int);

									memcpy(buffer+desplazamiento, &cantTareasRG, sizeof(int));
									desplazamiento =+ sizeof(int);

									memcpy(buffer+desplazamiento, &cantidadFallos, sizeof(int));
									desplazamiento =+ sizeof(int);

									memcpy(buffer+desplazamiento, &cantMaxEnParalelo, sizeof(int));
									desplazamiento =+ sizeof(int);

									enviar(socket, cop_yama_metricas, desplazamiento, buffer);
								}
							}
						}
					}
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
		//    ins points to the next occurrence of 500rep in orig
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

char* generarDirectorioTemporal(){
	char* dirTemp=string_new();
	string_append(&dirTemp, "./tm/");
	string_append(&dirTemp, randstring(5));//todo buscar una funcion que te genere X cantidad de caracteres aleatorios
	return dirTemp;
}

char *randstring(size_t length) {
    static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-#'?!";
    char *randomString = NULL;
    if (length){
        randomString = malloc(sizeof(char) * (length +1));
        if (randomString){
        	int n;
            for ( n = 0;n < length;n++) {
                int key = rand() % (int)(sizeof(charset) -1);
                randomString[n] = charset[key];
            }
            randomString[length] = '\0';
        }
    }
    return randomString;
}

void sig_handler(int signo){
    if (signo == SIGUSR1){
        printf("Se recibio SIGUSR1\n");
    	log_trace(logger, "Se recibio SIGUSR1");
    	configuracion = get_configuracion();
    	printf("Se cargo nuevamente el archivo de configuracion\n");
    	log_trace(logger, "Se cargo nuevamente el archivo de configuracion");
    }
}

void planificarBloque(t_tabla_planificacion* tabla, int numeroBloque, t_archivoxnodo* archivo, t_estados* estadoxjob, char* workerIdCaido){
	int* numBloqueParaLista = malloc(sizeof(int));
	*numBloqueParaLista=numeroBloque;
	if(workerIdCaido != NULL){
		bool buscarNodoCaido(void* elem){
			return string_equals_ignore_case(workerIdCaido, ((t_nodoxbloques*)elem)->idNodo);
		}
		list_remove_by_condition(archivo->nodos, buscarNodoCaido);
	}

	bool existeBloqueEnWorker(void* elem){
		return numeroBloque == ((t_infobloque*)elem)->bloqueRelativo;
	}

	bool workerContieneBloque(void* elem){
		return string_equals_ignore_case(((t_clock*)tabla->clock_actual->data)->worker_id , ((t_nodoxbloques*)elem)->idNodo) &&
				list_any_satisfy(((t_nodoxbloques*)elem)->bloques, existeBloqueEnWorker);
	}

	void asignarBloqueAWorker(t_clock* worker,int* numeroBloque, t_archivoxnodo* archivo){
		bool buscarNodoWorker(void*elem){
				return string_equals_ignore_case(worker->worker_id , ((t_nodoxbloques*)elem)->idNodo);
		}
		t_nodoxbloques* nodoWorker=list_find(archivo->nodos, buscarNodoWorker);
		t_infobloque* infoBloque = list_find(nodoWorker->bloques,existeBloqueEnWorker);

		worker->disponibilidad--;
		t_job* jobBloque=malloc(sizeof(t_job));
		jobBloque->bloque= *numeroBloque;
		jobBloque->estado = enProceso;
		jobBloque->etapa = transformacion;
		jobBloque->ip=string_duplicate(worker->ip);
		jobBloque->puerto = worker->puerto;
		jobBloque->worker_id= string_duplicate(worker->worker_id);
		jobBloque->temporalTransformacion = generarDirectorioTemporal();
		time(&jobBloque->tiempoInicio);
		list_add(estadoxjob->contenido, jobBloque);
		list_add(worker->bloques, infoBloque);
		bool existeWorkerAsignado(void* elem){
			return string_equals_ignore_case(((t_clock*)elem)->worker_id , worker->worker_id);
		}
		if(!list_any_satisfy(archivo->workersAsignados, existeWorkerAsignado))
		{
			list_add(archivo->workersAsignados, worker);
		}
	}

	if(list_any_satisfy(archivo->nodos, workerContieneBloque)){
		if(CalcularDisponibilidad(((t_clock*)tabla->clock_actual->data), tabla)> 0)
		{
			asignarBloqueAWorker(((t_clock*)tabla->clock_actual->data), numBloqueParaLista, archivo);
			tabla->clock_actual = tabla->clock_actual->next;
			if(tabla->clock_actual == NULL)
				tabla->clock_actual = tabla->workers->head;
			if(CalcularDisponibilidad(((t_clock*)tabla->clock_actual->data),tabla) ==0)
			{
				((t_clock*)tabla->clock_actual->data)->disponibilidad= configuracion.DISPONIBILIDAD_BASE;
				//Vuelve a mover el clock
				tabla->clock_actual = tabla->clock_actual->next;
				if(tabla->clock_actual == NULL)
					tabla->clock_actual = tabla->workers->head;
			}
		}
		else {
			//ACA NO DEBERIA ENTRAR NUNCA.
			printf("el worker apuntado por el clock actual posee el bloque pero no tiene disponibilidad.\n");
		}
	}
	else{ //el worker apuntado por el clock actual no posee ese bloque
		t_link_element* elementoActual= tabla->clock_actual->next;
		if(elementoActual == NULL)
			elementoActual = tabla->workers->head;
		t_link_element* elementoOriginal = tabla->clock_actual;
		bool encontro=false;
		while(!encontro)
		{
			char* nombreWorker= ((t_clock*)elementoActual->data)->worker_id;
			bool workerContieneBloqueByWorkerId(void* elem){
				return string_equals_ignore_case( nombreWorker, ((t_nodoxbloques*)elem)->idNodo) &&
				list_any_satisfy(((t_nodoxbloques*)elem)->bloques, existeBloqueEnWorker);
			}
			if(list_any_satisfy(archivo->nodos, workerContieneBloqueByWorkerId) && ((t_clock*)elementoActual->data)->disponibilidad >0)
			{
				encontro=true;
				asignarBloqueAWorker(((t_clock*)elementoActual->data), numBloqueParaLista, archivo);
			}
			else{
				elementoActual = elementoActual->next;
				if(elementoActual == NULL)
					elementoActual = tabla->workers->head;

				if(string_equals_ignore_case(((t_clock*)elementoActual->data)->worker_id, ((t_clock*)elementoOriginal->data)->worker_id))
				{
					void sumarDisponibilidadBase(void* elem){
						((t_clock*)elem)->disponibilidad+= configuracion.DISPONIBILIDAD_BASE;
					}
					list_iterate(tabla->workers,sumarDisponibilidadBase);
				}
			}
		}
	}
}

int CalcularDisponibilidad(t_clock* worker, t_tabla_planificacion* tabla){
	if(string_equals_ignore_case(configuracion.ALGORITMO_BALANCEO, "CLOCK")){
		return worker->disponibilidad;
	}
	else {
		return worker->disponibilidad + CalcularWLMax(tabla) - CalcularWLWorker(worker);
	}
}

int CalcularWLMax(t_tabla_planificacion* tabla){
	int wlMax=0;
	void SumarWL(void* elem){
		wlMax+=CalcularWLWorker((t_clock*)elem);
	}
	list_iterate(tabla->workers, SumarWL);
	return wlMax;
}

int CalcularWLWorker(t_clock* worker){
	return list_size(worker->bloques);
}


