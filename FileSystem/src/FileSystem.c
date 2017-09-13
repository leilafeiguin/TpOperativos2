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

	 fd_set master;    // master file descriptor list
	    fd_set read_fds;  // temp file descriptor list for select()
	    int fdmax;        // maximum file descriptor number

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
	    if ((rv = getaddrinfo(NULL, configuracion.PUERTO_FS, &hints, &ai)) != 0) {
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
	    fdmax = listener; // so far, it's this one

	pthread_t hiloFileSystem;
	pthread_create(&hiloFileSystem, NULL, hiloFileSystem_Consola,NULL);

	int socketActual;

	//CONEXIONES
	while(1){
		read_fds = master;
		select(fdmax+1, &read_fds, NULL, NULL, NULL);
		for(socketActual = 0; socketActual <= fdmax; socketActual++) {
				if (FD_ISSET(socketActual, &read_fds)) {
					if (socketActual == listener) { //es una conexion nueva
						newfd = aceptar_conexion(socketActual);
						t_paquete* handshake = recibir(socketActual);
						FD_SET(newfd, &master); //Agregar al master SET
						if (newfd > fdmax) {    //Update el Maximo
							fdmax = newfd;
						}
						log_trace(logger, "FileSystem recibio una nueva conexion");
						free(handshake);
					//No es una nueva conexion -> Recibo el paquete
					} else {
						t_paquete* paqueteRecibido = recibir(socketActual);
						switch(paqueteRecibido->codigo_operacion){ //revisar validaciones de habilitados
						case cop_handshake_yama:
							esperar_handshake(socketActual, paqueteRecibido, cop_handshake_yama);
						break;
						case cop_archivo_programa:
							log_trace(logger, paqueteRecibido->data);
						break;
						}
					}
				}
		}
	}
	return EXIT_SUCCESS;
}


void hiloFileSystem_Consola(void * unused){
	printf("Consola Iniciada. Ingrese una opcion \n");
	char * linea;
	while(1) {
		linea = readline(">");
		if (!linea) {
			break;
		}else{
			add_history(linea);
			if (strcmp(linea, "format") == 0){
				printf("Formatear el Filesystem\n");
				free(linea);
			}else if (strcmp(linea, "rm [path_archivo]") == 0){
				printf("Eliminar un Archivo/Directorio/Bloque. Si un directorio a eliminar no se encuentra vacío, la operación debe fallar. Además, si el bloque a eliminar fuera la última copia del mismo, se deberá abortar la operación informando lo sucedido.\n");
				char *parametro = linea;
				strsep(&parametro, " ");
				printf(parametro);
				free(linea);
			}else if (strcmp(linea, "rename [path_original] [nombre_final]") == 0){
				printf("Renombra un Archivo o Directorio\n");

				 char *linea2, *found;
				 char*parametros[2];
				 int i;
				 linea2 = strdup(linea);
				 found = strsep(&linea2," ");
				 found = strsep(&linea2," ");

				 for(i=0;found != NULL;i++){
					 printf("%s\n",found);
					 parametros[i] = found;
					 found = strsep(&linea2," ");
				 }

				free(linea);
			}else if (strcmp(linea, "mv [path_original] [path_final]") == 0){
				printf("Mueve un Archivo o Directorio\n");
				free(linea);
			}else if (strcmp(linea, "cat [path_archivo]") == 0){
				printf("Muestra el contenido del archivo como texto plano.\n");
				free(linea);
			}else if (strcmp(linea, "mkdir [path_dir]") == 0){
				printf("Crea un directorio. Si el directorio ya existe, el comando deberá informarlo.\n");
				free(linea);
			}else if (strcmp(linea, "cpfrom [path_archivo_origen] [directorio_yamafs]") == 0){
				printf("Copiar un archivo local al yamafs, siguiendo los lineamientos en la operaciòn Almacenar Archivo, de la Interfaz del FileSystem.\n");
				free(linea);
			}else if (strcmp(linea, "cpto [path_archivo_yamafs] [directorio_filesystem]") == 0){
				printf("Copiar un archivo local al yamafs\n");
				free(linea);
			}else if (strcmp(linea, "cpblock [path_archivo] [nro_bloque] [id_nodo]") == 0){
				printf("Crea una copia de un bloque de un archivo en el nodo dado.\n");
				free(linea);
			}else if (strcmp(linea, "md5 [path_archivo_yamafs]") == 0){
				printf("Solicitar el MD5 de un archivo en yamafs\n");
				free(linea);
			}else if (strcmp(linea, "ls [path_directorio]") == 0){
				printf("Lista los archivos de un directorio\n");
				free(linea);
			}else if (strcmp(linea, "info [path_archivo]") == 0){
				printf("Muestra toda la información del archivo, incluyendo tamaño, bloques, ubicación de los bloques, etc.\n");
				free(linea);
			}else {
				printf("Opcion no valida.\n");
				free(linea);
			}
		}
	}
}



