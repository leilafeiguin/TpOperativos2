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
#include <string.h>
#include <assert.h>
#include "FileSystem.h"

int main(void) {

//	void asignarNombre(char list[], char* asignar){
//		int i = 0;
//		while(i<strlen(asignar)){
//			list[i] = asignar[i];
//			i++;
//		}
//	};
//
//	tablaDeDirectorios[0].index = 0;
//	asignarNombre(tablaDeDirectorios[0].nombre,"root");
//	tablaDeDirectorios[0].padre = -1;

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

char** validaCantParametrosComando(char* comando, int cantParametros){
	int i = 0;
	char** parametros;
	parametros = str_split(comando, ' ');
	for (i = 1; *(parametros + i); i++)
	{
		printf("parametros= %s \n", *(parametros + i));
	}
	free(parametros);
	if(i != cantParametros+1){
		printf("rm necesita %i parametro/s. \n", cantParametros);
		return 0;
	}else{
		printf("Cantidad de parametros correcta. \n");
		return parametros;
	}
	return 0;
}

void hiloFileSystem_Consola(void * unused){
	printf("Consola Iniciada. Ingrese una opcion \n");
	char * linea;
	char* primeraPalabra;
	char* context;
	int i = 0;
	while(1) {
		linea = readline(">");
		if (!linea) {
			break;
		}else{
			add_history(linea);
			char** parametros;
			int lineaLength = strlen(linea);
			char *lineaCopia = (char*) calloc(lineaLength + 1, sizeof(char));
			strncpy(lineaCopia, linea, lineaLength);
			primeraPalabra = strtok_r (lineaCopia, " ", &context);

			if (strcmp(linea, "format") == 0){
				printf("Formatear el Filesystem\n");
				free(linea);
			}else if (strcmp(primeraPalabra, "rm") == 0){
				printf("Eliminar un Archivo/Directorio/Bloque. Si un directorio a eliminar no se encuentra vacío, la operación debe fallar. Además, si el bloque a eliminar fuera la última copia del mismo, se deberá abortar la operación informando lo sucedido.\n");
				parametros = validaCantParametrosComando(linea, 1);
				free(linea);
			}else if (strcmp(primeraPalabra, "rename") == 0){
				printf("Renombra un Archivo o Directorio\n");
				parametros = validaCantParametrosComando(linea, 2);
				free(linea);
			}else if (strcmp(primeraPalabra, "mv") == 0){
				printf("Mueve un Archivo o Directorio\n");
				parametros = validaCantParametrosComando(linea, 2);
				free(linea);
			}else if (strcmp(primeraPalabra, "cat") == 0){
				printf("Muestra el contenido del archivo como texto plano.\n");
				parametros = validaCantParametrosComando(linea, 1);
				free(linea);
			}else if (strcmp(primeraPalabra, "mkdir") == 0){
				printf("Crea un directorio. Si el directorio ya existe, el comando deberá informarlo.\n");
				parametros = validaCantParametrosComando(linea, 1);
				free(linea);
			}else if (strcmp(primeraPalabra, "cpfrom") == 0){
				printf("Copiar un archivo local al yamafs, siguiendo los lineamientos en la operaciòn Almacenar Archivo, de la Interfaz del FileSystem.\n");
				parametros = validaCantParametrosComando(linea, 2);
				free(linea);
			}else if (strcmp(primeraPalabra, "cpto") == 0){
				printf("Copiar un archivo local al yamafs\n");
				parametros = validaCantParametrosComando(linea, 2);
				free(linea);
			}else if (strcmp(primeraPalabra, "cpblock") == 0){
				printf("Crea una copia de un bloque de un archivo en el nodo dado.\n");
				parametros = validaCantParametrosComando(linea, 3);
				free(linea);
			}else if (strcmp(primeraPalabra, "md5") == 0){
				printf("Solicitar el MD5 de un archivo en yamafs\n");
				parametros = validaCantParametrosComando(linea, 1);
				free(linea);
			}else if (strcmp(primeraPalabra, "ls") == 0){
				printf("Lista los archivos de un directorio\n");
				parametros = validaCantParametrosComando(linea, 1);
				free(linea);
			}else if (strcmp(primeraPalabra, "info") == 0){
				printf("Muestra toda la información del archivo, incluyendo tamaño, bloques, ubicación de los bloques, etc.\n");
				parametros = validaCantParametrosComando(linea, 1);
				free(linea);
			}else {
				printf("Opcion no valida.\n");
				free(linea);
			}
		}
	}
}

// SIN TERMINAR

//t_bitmap leerBitmap(char* unPath){
//	t_bitmap bitmap;
//	t_bitmap* siguiente;
//	bitmap.siguiente = NULL;
//	FILE *fp = fopen (unPath, "rb");
//	int i=0;
//	fseek(fp, 0, SEEK_END);
//	int tamanio = sizeof(char) * ftell(fp);
//	fseek(fp, 0, SEEK_SET);
//	char* bytes = malloc(tamanio);
//	fread(bytes, tamanio, 1, fp);
//	fclose (fp);
//	while(i<tamanio){
//		bitmap.estado = bytes[i];
//		bitmap.siguiente;
//		i++;
//	};
//	free(bytes);
//	return bitmap;
//};


