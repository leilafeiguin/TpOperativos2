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
#include "Yama.h"


int main(void) {
	t_log* logger;
	char* fileLog;
	fileLog = "YamaLogs.txt";

	printf("Inicializando proceso Yama\n");
	logger = log_create(fileLog, "Yama Logs", 0, 0);
	log_trace(logger, "Inicializando proceso Yama");

	yama_configuracion configuracion = get_configuracion();
	log_trace(logger, "Archivo de configuracion levantado");


	//Estructuras para el Select
		fd_set listaOriginal;
		fd_set listaAux;
		FD_ZERO(&listaOriginal);
		FD_ZERO(&listaAux);
		int fd_max = 1;
		int newfd;
		struct sockaddr_storage remoteaddr; // client address
		socklen_t addrlen;
		int nbytes;
		char remoteIP[INET6_ADDRSTRLEN];
		int yes=1;        // for setsockopt() SO_REUSEADDR, below
		int socketActual, j, rv;
		struct addrinfo hints, *ai, *p;
		struct sockaddr_in direccionServidor;
		direccionServidor.sin_family = AF_INET;
		direccionServidor.sin_addr.s_addr = inet_addr("127.0.0.1");
		direccionServidor.sin_port = htons("6666");//configuracion.PUERTO_FS);
		un_socket socketServer = socket(AF_INET, SOCK_STREAM, 0);
		setsockopt(socketServer, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		int error=0;
		error=bind(socketServer, (struct sockaddr *) &direccionServidor, sizeof(direccionServidor));
		if(error <0)
			perror("error en bind");
		error=listen(socketServer, 256);
		if(error <0)
			perror("error en listen");

		FD_SET(socketServer,&listaOriginal);


	 	//Iniciar hilo consola

		pthread_t hiloFileSystem;
		//pthread_create(&hiloFileSystem, NULL, hiloFileSystem_Consola);

		//Se conecta al FileSystem
		un_socket fileSystemSocket = conectar_a(configuracion.IP_FS,configuracion.PUERTO_FS);
		FD_SET(fileSystemSocket, &listaOriginal);
		if(fileSystemSocket > socketServer){
			fd_max = fileSystemSocket;
		}else{
			fd_max = socketServer;
		}
		realizar_handshake(fileSystemSocket, cop_handshake_yama);
		//Que pasa si le rechazan la conexion.

		//CONEXIONES
		while(1){
			listaAux = listaOriginal;
			select(fd_max+1, &listaAux, NULL, NULL, NULL);
			for(socketActual = 0; socketActual <= fd_max; socketActual++) {
					if (FD_ISSET(socketActual, &listaAux)) {
						if (socketActual == socketServer) { //es una conexion nueva
							newfd = aceptar_conexion(socketActual);
							t_paquete* handshake = recibir(socketActual);
							FD_SET(newfd, &listaOriginal); //Agregar al master SET
							if (newfd > fd_max) {    //Update el Maximo
								fd_max = newfd;
							}
							log_trace(logger, "Yama recibio una nueva conexion");
							free(handshake);
						//No es una nueva conexion -> Recibo el paquete
						} else {
							t_paquete* paqueteRecibido = recibir(socketActual);
							switch(paqueteRecibido->codigo_operacion){ //revisar validaciones de habilitados
							case cop_archivo_programa:
								enviar(fileSystemSocket, cop_archivo_programa,sizeof(paqueteRecibido->data) ,paqueteRecibido->data);
								//recibir un archivo
							break;
							}
						}
					}

			}
		}

	return EXIT_SUCCESS;
}
