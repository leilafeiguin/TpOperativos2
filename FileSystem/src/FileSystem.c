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
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

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

	//Inciializacion de estructuras
	int x;
	for(x=0; x<100;x++){
		tablaDeDirectorios[x] = malloc(sizeof(struct t_directory));
		tablaDeDirectorios[x]->index = -2;
		sprintf(tablaDeDirectorios[x]->nombre, "");
		tablaDeDirectorios[x]->padre = 0;
	}

	if( access("directorios.txt", F_OK) != -1 ){
	    // El archivo existe, recupero la informacion
	} else {
	    // El archivo no existe
	}
	actualizarArchivoDeDirectorios();

	fileSystem.ListaNodos = list_create(); //Se le deben cargar estructuras de tipo t_nodo
	fileSystem.libre = 0;
	fileSystem.listaArchivos = list_create(); //Se le deben cargar estructuras de tipo t_archivo
	fileSystem.tamanio = 0;

	// ----------------------------------------------
	// ----------------------------------------------
	// ----------------------------------------------
	// ----------------------------------------------
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
	for(p = ai; p != NULL; p = p->ai_next){
		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0){
			continue;
		}
		// lose the pesky "address already in use" error message
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0){
			close(listener);
			continue;
		}
		break;
	}
	// if we got here, it means we didn't get bound
	if (p == NULL){
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

	// ----------------------------------------------
	// ----------------------------------------------
	// ----------------------------------------------
	// ----------------------------------------------
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
							{
								esperar_handshake(socketActual, paqueteRecibido, cop_handshake_yama);

								int tamanioTotal=0;
								void contabilizarTamanio(void* elemento){
									tamanioTotal+= (strlen(((t_nodo*)elemento)->ip)+1 + 3* sizeof(int));
								};


								int cantidadElementos=list_size(fileSystem.ListaNodos);
								list_iterate(fileSystem.ListaNodos, contabilizarTamanio );

								int desplazamiento=0;
								void* buffer = malloc(sizeof(int) + tamanioTotal);
								memcpy(buffer, &cantidadElementos, sizeof(int));
								desplazamiento+= sizeof(int);

								void copiarABuffer(void* elemento){
									int longitudIp=strlen(((t_nodo*)elemento)->ip)+1;
									int longitudNombre = strlen(((t_nodo*)elemento)->nroNodo )+1;

									memcpy(buffer+desplazamiento,  longitudIp, sizeof(int));
									desplazamiento+= sizeof(int);
									memcpy(buffer+desplazamiento, ((t_nodo*)elemento)->ip, longitudIp);
									desplazamiento+= longitudIp;
									memcpy(buffer+ desplazamiento, &((t_nodo*)elemento)->puertoWorker, sizeof(int));
									desplazamiento+= sizeof(int);
									memcpy(buffer+ desplazamiento, &((t_nodo*)elemento)->tamanio, sizeof(int));
									desplazamiento+= sizeof(int);
									memcpy(buffer+ desplazamiento, &longitudNombre, sizeof(int));
									desplazamiento+= sizeof(int);
									memcpy(buffer+ desplazamiento, ((t_nodo*)elemento)->nroNodo,longitudNombre);
									desplazamiento+= longitudNombre;

								};

								list_iterate(fileSystem.ListaNodos,copiarABuffer);

								enviar(socketActual, cop_datanode_info,desplazamiento, buffer);
							}

							//enviar(socketActual, cop_datanode_info, sizeof(char*) t_datanode_info, );
							//todo mati e armar un paquete con t_datanode_info_list lista de t_nodos y enviarlo
						break;
						case cop_archivo_programa:
							log_trace(logger, paqueteRecibido->data);
						break;
						case cop_handshake_datanode:
							esperar_handshake(socketActual, paqueteRecibido, cop_handshake_datanode);
							//Falta la consideracion si se levanta de un estado anterior
							break;
						case cop_datanode_info:
						{
							t_paquete_datanode_info_list* infoNodo = malloc(sizeof(t_paquete_datanode_info_list));
							int longitudIp;
							int longitudNombre;
							int desplazamiento=0;
							memcpy(&longitudIp, paqueteRecibido->data + desplazamiento, sizeof(int));
							desplazamiento+=sizeof(int);
							infoNodo->ip= malloc(longitudIp);

							memcpy(infoNodo->ip, paqueteRecibido->data + desplazamiento, longitudIp);
							desplazamiento+=longitudIp;

							memcpy(&infoNodo->puertoWorker, paqueteRecibido->data + desplazamiento, sizeof(int));
							desplazamiento+=sizeof(int);

							memcpy(&infoNodo->tamanio, paqueteRecibido->data + desplazamiento, sizeof(int));
							desplazamiento+=sizeof(int);

							memcpy(&longitudNombre, paqueteRecibido->data + desplazamiento, sizeof(int));
							desplazamiento+=sizeof(int);
							infoNodo->nombreNodo= malloc(longitudNombre);

							memcpy(infoNodo->nombreNodo, paqueteRecibido->data + desplazamiento, longitudNombre);
							desplazamiento+=longitudNombre;



							t_bitarray* unBitmap;
							char* data[] = {00000000000000000000};
							unBitmap = bitarray_create(data, 3);
							t_nodo* unNodo = nodo_create(infoNodo->nombreNodo, false, unBitmap, socketActual , infoNodo->ip, infoNodo->puertoWorker, infoNodo->tamanio);
							list_add(fileSystem.ListaNodos, unNodo);
						}
							//falta agregar otras estructuras administrativas
						break;
						}
					}
				}
		}
	}
	return EXIT_SUCCESS;
}

// CP_TO --> igual que CP FROM PERO CAMBIA ORIGEN Y DESITNO
void CP_TO (char* origen, char*destino){
	//desarrolar
}




//CPFROM
void CP_FROM(char* origen, char* destino){
	int cantidadBloques=0;
	char** bloques = LeerArchivo(origen, &cantidadBloques);
	char* path= malloc(255);
	char* file= malloc(255);
	destino= str_replace(destino, "yamafs://","");
    split_path_file(&path, &file, destino);
    char** listaDirectorios=string_split(path, "/");

    int j=0;
    int padre=0;
    int cantidadDirectorios = countOccurrences(path, "/")+1;
    for(;j<cantidadDirectorios;j++){
    	t_directory* directorio= buscarDirectorio(padre, listaDirectorios[cantidadDirectorios]);
    	if(directorio != NULL)
    		directorio= crearDirectorio(padre, listaDirectorios[cantidadDirectorios]) ;

    	padre= directorio->index;
    }

	int i=0;
	for(;i<cantidadBloques;i++){
		t_nodoasignado* respuesta = escribir_bloque(bloques[i]);
		t_nodo* nodo1= buscar_nodo(respuesta->nodo1);
		nodo1->libre--;
		if(nodo1->libre ==0)
			nodo1->ocupado=true;

		bitarray_set_bit(nodo1->bitmap,respuesta->bloque1 );


		t_nodo* nodo2= buscar_nodo(respuesta->nodo2);
		nodo2->libre--;
		if(nodo2->libre ==0)
			nodo2->ocupado=true;

		bitarray_set_bit(nodo2->bitmap,respuesta->bloque2 );


	}

	//todo tabla de dir
	//todo tabla archivos

}

t_directory* crearDirectorio(int padre, char* nombre){
	int x;
	for(x=0; x<100;x++){

		if(tablaDeDirectorios[x]->index == -2)
		{
			//tablaDeDirectorios[x]->nombre= malloc(strlen(nombre)+1);
			strcpy(tablaDeDirectorios[x]->nombre, nombre);
			tablaDeDirectorios[x]->padre=padre;
			tablaDeDirectorios[x]->index= x;
			return tablaDeDirectorios[x];
		}
	}

		 return NULL;
}

t_directory* buscarDirectorio(int padre, char* nombre){
	int x;
		for(x=0; x<100;x++){

			if(tablaDeDirectorios[x]->padre == padre && string_equals_ignore_case(tablaDeDirectorios[x] ->nombre, nombre))
				return tablaDeDirectorios[x];
		}

	 return NULL;
}

int countOccurrences(char * str, char * toSearch)
{
    int i, j, found, count;
    int stringLen, searchLen;

    stringLen = strlen(str);      // length of string
    searchLen = strlen(toSearch); // length of word to be searched

    count = 0;

    for(i=0; i <= stringLen-searchLen; i++)
    {
        /* Match word with string */
        found = 1;
        for(j=0; j<searchLen; j++)
        {
            if(str[i + j] != toSearch[j])
            {
                found = 0;
                break;
            }
        }

        if(found == 1)
        {
            count++;
        }
    }

    return count;
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
    for (count = 0; tmp = strstr(ins, rep); ++count) {
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

void split_path_file(char** p, char** f, char *pf) {
    char *slash = pf, *next;
    while ((next = strpbrk(slash + 1, "\\/"))) slash = next;
    if (pf != slash) slash++;
    *p = strndup(pf, slash - pf);
    *f = strdup(slash);
}

char** LeerArchivo(char* archivo, int* cantbloques){
	struct stat sb;
	int fd = open(archivo, O_RDONLY);
	fstat(fd, &sb);
	void* archivoMapeado = mmap(NULL,sb.st_size,PROT_READ | PROT_WRITE,  MAP_SHARED,fd,0);
	int cantidadBloques = sb.st_size / 1024*1024;
	if(sb.st_size % (1024 *1024) != 0)
		cantidadBloques++;
	char** listaBloques = malloc(cantidadBloques);
	int tamanio=0;
	int i=0;
	for(; i< cantidadBloques ; i++) //falta completar casos de archivos de texto
	{
		int tamanioBloque = 1024*1024;
		if(sb.st_size - tamanio < tamanioBloque)
			tamanioBloque = sb.st_size-tamanio;
		(*listaBloques[i]) = malloc(tamanioBloque);
		memcpy((*listaBloques[i]), archivoMapeado+tamanio, tamanioBloque);
		tamanio+=(tamanioBloque);
	}
	*cantbloques=cantidadBloques;
	return listaBloques;
}

char** str_split(char* a_str, const char a_delim){
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;
    /* Count how many elements will be extracted. */
    while (*tmp){
        if (a_delim == *tmp){
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
    if (result){
        size_t idx  = 0;
        char* token = strtok(a_str, delim);
        while (token){
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
	for (i = 1; *(parametros + i); i++){
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

void formatearFileSystem(){
	int i;
	list_destroy(fileSystem.ListaNodos);
	t_list* nodos = list_create();
	fileSystem.ListaNodos = nodos;

	for(i=0;i<sizeof(tablaDeDirectorios)/sizeof(t_directory);i++){
		tablaDeDirectorios[i]->padre = -1;
	}
	printf("Filesystem formateado.\n");
}






void cp_block(char* path, int numeroBloque, char* nombreNodo){
	t_nodo* nodoDestino= buscar_nodo(nombreNodo);
	if(nodoDestino == NULL || nodoDestino->ocupado)
	{
		printf("no existe el nodo %s o está ocupado" , nombreNodo);
		return;

	}

	bool buscarArchivoPorPath(void* elem){
		return string_equals_ignore_case(((t_archivo*)elem)->path,path);
	};

	t_archivo* archivoEncontrado=list_find(fileSystem.listaArchivos,buscarArchivoPorPath);
	if(archivoEncontrado == NULL)
	{
		printf("El path %s no se encuentra en el FS", path);
		return;
	}

	bool buscarBloquePorNumero(void* elem){
		return ((t_bloque*)elem)->nroBloque == numeroBloque;
	}

	t_bloque* bloque= list_find(archivoEncontrado->bloques, buscarBloquePorNumero);

	if(bloque == NULL)
	{
		printf("El bloque %i no se encuentra en el FS", numeroBloque);
		return;
	}

	ubicacionBloque* ubicacionOrigen = bloque->copia1;

	if( ubicacionOrigen == NULL)
	{
		ubicacionOrigen = bloque->copia2;
	}

	if(ubicacionOrigen != NULL)
	{

		t_nodo* nodoOrigen= buscar_nodo(ubicacionOrigen->nroNodo);
		void* contenido=getbloque(ubicacionOrigen->nroBloque, nodoOrigen);
		int numBloqueDestino = buscarBloque(nodoDestino);
		nodoDestino->libre--;
		if(nodoDestino->libre ==0)
			nodoDestino->ocupado=true;
		bitarray_set_bit(nodoDestino->bitmap,numBloqueDestino);
		enviar_bloque_a_escribir(numBloqueDestino, contenido, nodoDestino);

		ubicacionBloque* bloqueAsignado=malloc(sizeof(ubicacionBloque));
		bloqueAsignado->nroBloque =numBloqueDestino;
		bloqueAsignado->nroNodo = malloc(strlen(nombreNodo)+1);
		strcpy(bloqueAsignado->nroNodo, nombreNodo);
		if(bloque->copia1==NULL)
			bloque->copia1 = bloqueAsignado;
		else
			bloque->copia2 = bloqueAsignado;
	}
	else
	{
		printf("No tiene ninguna copia para tomar de origen");
		return;
	}
}

void cat(char* path){
	bool buscarArchivoPorPath(void* elem){
		return string_equals_ignore_case(((t_archivo*)elem)->path,path);
	};

	t_archivo* archivoEncontrado=list_find(fileSystem.listaArchivos,buscarArchivoPorPath);

	if(archivoEncontrado == NULL)
	{
		printf("El path %s no se encuentra en el FS", path);
		return;
	}

	void imprimirPorPantallaContenidoBloque(void* elem){
			t_bloque* bloque= (t_bloque*)elem;


			t_nodo* nodo=NULL;
			if(bloque->copia1 != NULL)
				nodo=buscar_nodo(bloque->copia1->nroNodo);
			if(nodo != NULL)
			{
				void* contenido=getbloque(bloque->copia1->nroBloque, nodo);
				if(archivoEncontrado->tipoArchivo == BINARIO)
				{

				}
				else{
					char* buffer = malloc(bloque->finBloque);
					memcpy(buffer, contenido, bloque->finBloque);
					printf(buffer);
				}
				//imprimir contenido

				return;
			}
			else{
				if(bloque->copia2 != NULL)
					nodo=buscar_nodo(bloque->copia2->nroNodo);

				if(nodo== NULL)
				{
					char* nombre1="";
					char* nombre2="";
					if(bloque->copia1 != NULL)
						nombre1=bloque->copia1->nroNodo;

					if(bloque->copia2 != NULL)
						nombre2=bloque->copia2->nroNodo;

					printf("No se encontro el bloque %i en el nodo %s ni en el nodo %s", bloque->nroBloque,nombre1, nombre2);
					return;
				}

				void* contenido=getbloque(bloque->copia2->nroBloque, nodo);
				if(archivoEncontrado->tipoArchivo == BINARIO)
				{

				}
				else{
					char* buffer = malloc(bloque->finBloque);
					memcpy(buffer, contenido, bloque->finBloque);
					printf("%s", buffer);
				}

				return;
			}

		}

	list_iterate(archivoEncontrado->bloques,imprimirPorPantallaContenidoBloque);
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
				formatearFileSystem();
				actualizarArchivoDeDirectorios();
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
				cat(parametros[0]);
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

t_nodo* buscar_nodo (char* nombreNodo){
	bool buscarNodo(void* elemento){
		return string_equals_ignore_case(((t_nodo*)elemento)->nroNodo, nombreNodo);
	}
	return list_find(fileSystem.ListaNodos, buscarNodo);
}

t_nodo* buscar_nodo_libre (int nodoAnterior){
	bool buscarLibre(void* elemento){
		return !((t_nodo*)elemento)->ocupado && (nodoAnterior ==0 || ((t_nodo*)elemento)->nroNodo!=nodoAnterior);
	}
	return list_find(fileSystem.ListaNodos, buscarLibre);
}


int buscarBloque(t_nodo* nodo){
	int tamanio= bitarray_get_max_bit (nodo->bitmap);
	int i = 0;
	for (; i< tamanio; i++){
		bool ocupado = bitarray_test_bit (nodo->bitmap, i);
		if (!ocupado)
			return i;
	}
	return -1;
}

void* getbloque (int numBloque, t_nodo* nodo){
	t_getbloque* bloque = malloc(sizeof(t_getbloque));
	bloque->numero_bloque= numBloque;
	void* buffer = malloc(sizeof(int));
	int desplazamiento=0;
	memcpy(buffer, &bloque->numero_bloque, sizeof(int));
	desplazamiento+= sizeof(int);
	enviar(nodo->socket, cop_datanode_get_bloque,desplazamiento, buffer);
	t_paquete* paquete=recibir(nodo->socket);
	return paquete->data;

}

void enviar_bloque_a_escribir (int numBloque, void* contenido, t_nodo* nodo){
	t_setbloque* bloque = malloc(sizeof(t_setbloque));
	bloque->numero_bloque = numBloque;
	bloque->datos_bloque = malloc(1024*1024);
	memcpy(bloque->datos_bloque, contenido, 1024*1024);
	void* buffer = malloc(sizeof(int) + 1024*1024);//numero bloque
	int desplazamiento=0;
	memcpy(buffer, &bloque->numero_bloque, sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(buffer + desplazamiento,  bloque->datos_bloque, 1024*1024);//datos->bloque
	desplazamiento+= 1024*1024;
	enviar(nodo->socket, cop_datanode_setbloque,desplazamiento, bloque);
}

t_nodoasignado* escribir_bloque (void* bloque){
	t_nodo* nodolibre =	buscar_nodo_libre (0);
	int numBloque = buscarBloque(nodolibre);
	enviar_bloque_a_escribir(numBloque,bloque,nodolibre );

	nodolibre =	buscar_nodo_libre (nodolibre->nroNodo);
	numBloque = buscarBloque(nodolibre);
	enviar_bloque_a_escribir(numBloque,bloque,nodolibre );

	return NULL;//todo mati gg aca crea la estructura nodo asignado y asignale los valores que corresponden.
}

void actualizarArchivoDeDirectorios(){
	crear_subcarpeta("metadata");
	FILE * file= fopen("metadata/directorios.txt", "w");
	int i;
	if (file != NULL) {
		fprintf(file, "%i,%s,%i" ,tablaDeDirectorios[0]->index,tablaDeDirectorios[0]->nombre,tablaDeDirectorios[0]->padre);
		for(i=1;i<100;i++){
			if(tablaDeDirectorios[i]->index != 0){
				fprintf(file, "/%i,%s,%i" ,tablaDeDirectorios[i]->index,tablaDeDirectorios[i]->nombre,tablaDeDirectorios[i]->padre);
			}
		}
		fclose(file);
		}
}

void crear_subcarpeta(char* nombre){
	struct stat st = {0};
	if (stat(nombre, &st) == -1) {
	    mkdir(nombre, 0700);
	}
}

//Revisar
void actualizarBitmap(t_nodo unNodo){
	crear_subcarpeta("metadata/bitmaps/");
	char* aux;
	aux = "";
	strcat(aux, "metadata/bitmaps/");
	strcat(aux, unNodo.nroNodo);
	strcat(aux, ".dat");
	FILE * file= fopen(aux, "wb");
	if (file != NULL) {
		fwrite(unNodo.bitmap,sizeof(unNodo.bitmap),1,file);
		fclose(file);
		}
}

void actualizarArchivoTablaNodos(){
	crear_subcarpeta("metadata");
	FILE * file= fopen("metadata/nodos.bin", "wb");
	if (file != NULL) {
		int libreTotal;
		libreTotal = 0;
		int tamanioTotal;
		tamanioTotal = 0;
		int cantidadNodos;
		cantidadNodos = list_count_satisfying(fileSystem.ListaNodos ,true);
		int libresPorNodo[cantidadNodos];
		int tamanioPorNodo[cantidadNodos];
		char* nodos[cantidadNodos];
		int i;
		for(i=0;i<cantidadNodos;i++){
			t_nodo* aux;
			aux = list_get(fileSystem.ListaNodos,i);
			libreTotal += aux->libre;
			tamanioTotal += aux->tamanio;
			libresPorNodo[i] = aux->libre;
			tamanioPorNodo[i] = aux->tamanio;
			nodos[i] = aux->nroNodo;
		}
		fprintf(file, "TAMANIO=%i\nLIBRE=%i\nNODOS=[" ,tamanioTotal,libreTotal);
		for(i=0;i<cantidadNodos-1;i++){
			fprintf(file, "%s,", nodos[i]);
		}
		fprintf(file,"%s]\n",nodos[cantidadNodos]);
		for(i=0;i<cantidadNodos;i++){
			fprintf(file,"%sTotal=%i\n%sLibre%i\n",nodos[i],tamanioPorNodo[i],nodos[i],libresPorNodo[i]);
		}
		fclose(file);
		}
}

