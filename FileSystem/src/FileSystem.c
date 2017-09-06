/*
 ============================================================================
 Name        : FileSystem.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "FileSystem.h"
//#include <readline/readline.h>
//#include <readline/history.h>

int main(void) {
	//Logger
	t_log* logger;
	char* fileLog;
	fileLog = "FileSystemLogs.txt";
	fclose(fopen(fileLog, "w"));

	printf("Inicializando proceso FileSystem\n");
	logger = log_create(fileLog, "FileSystem Logs", 0, 0);
	log_trace(logger, "Inicializando proceso FileSystem");

	fileSystem_configuracion configuracion = get_configuracion();
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
	direccionServidor.sin_port = htons("8000");//configuracion.PUERTO_FS);
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


	//CONEXIONES
	while(1){
		listaAux = listaOriginal;
		select(fd_max+1, &listaAux, NULL, NULL, NULL);
		for(socketActual = 0; socketActual <= fd_max; socketActual++) {
				if (FD_ISSET(socketActual, &listaAux)) {
					if (socketActual == socketServer) { //es una conexion nueva
						newfd = aceptar_conexion(socketActual);
						t_paquete* handshake = recibir(socketActual);
						if(esperar_handshake(socketActual, handshake, cop_handshake_datanode)){
							FD_SET(newfd, &listaOriginal); //Agregar al master SET
							if (newfd > fd_max) {    //Update el Maximo
								fd_max = newfd;
							}
							log_trace(logger, "FileSystem recibio un nuevo Nodo");
						} else if (esperar_handshake(socketActual, handshake, cop_handshake_yama)){
							//Comprobar estado
							if(estado != "Estable"){
								log_trace(logger, "Conexion de Yama rechazada, estado NO estable");
							}else if(existeYama == true){
								log_trace(logger, "Conecion de Yama rechazada, ya existe otro Yama");
							}else{
								//Acepto a Yama
								FD_SET(newfd, &listaOriginal); //Agregar al master SET
								if (newfd > fd_max) {    //Update el Maximo
									fd_max = newfd;
								}
								existeYama = true;
								log_trace(logger, "FileSystem se conecto con Yama");
								}
						}
						free(handshake);
					//No es una nueva conexion -> Recibo el paquete
					} else {
						t_paquete* paqueteRecibido = recibir(socketActual);
						switch(paqueteRecibido->codigo_operacion){ //revisar validaciones de habilitados
						}
					}
				}

		}
	}
	return EXIT_SUCCESS;
}


//void hiloFileSystem_Consola(){
//	char * linea;
//	  while(1) {
//		  linea = readline(">");
//		  if(linea)
//			  add_history(linea);
//		  if(!strncmp(linea, "exit", 4)) {
//			  free(linea);
//			  break;
//		  }
//		  printf("%s\n", linea);
//		  free(linea);
//	  }
//}



