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
		realizar_handshake(fileSystemSocket, cop_handshake_yama);
		//Que pasa si le rechazan la conexion.
		int socketActual;
		//CONEXIONES
		while(1){
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
							break;
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
