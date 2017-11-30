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
#include <pthread.h>
#include <dirent.h>


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

	//Incializacion de estructuras
	int x = 0;
	tablaDeDirectorios[x] = malloc(sizeof(struct t_directory));
	tablaDeDirectorios[x]->index = 0;
	tablaDeDirectorios[x]->nombre = "root";
	tablaDeDirectorios[x]->padre = -1;
	for(x = 1; x < 100; x++) {
		tablaDeDirectorios[x] = malloc(sizeof(struct t_directory));
		tablaDeDirectorios[x]->index = -2;
		tablaDeDirectorios[x]->nombre = "";
		tablaDeDirectorios[x]->padre = 0;
	}
	fileSystem.ListaNodos = list_create(); //Se le deben cargar estructuras de tipo t_nodo
	fileSystem.libre = 0;
	fileSystem.listaArchivos = list_create(); //Se le deben cargar estructuras de tipo t_archivo
	fileSystem.tamanio = 0;

	cargarDirectoriosDesdeArchivo();
	DIR* dir = opendir("/metadata/archivos/");
	if (dir){
		cargarArchivos();
		closedir(dir);
	}
	// ----------------------------------------------
	// ----------------------------------------------
	// ----------------------------------------------
	// ----------------------------------------------

	int listener;     // listening socket descriptor

	int yes = 1;        // for setsockopt() SO_REUSEADDR, below
	int rv;
	struct addrinfo hints, *ai, *p;

	// get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(NULL, configuracion.PUERTO_FS, &hints, &ai)) != 0) {
		fprintf(stderr, "selectserver: %s", gai_strerror(rv));
		exit(1);
	}
	for (p = ai; p != NULL; p = p->ai_next) {
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
		fprintf(stderr, "selectserver: failed to bind");
		exit(2);
	}
	freeaddrinfo(ai); // all done with this
	// listen
	if (listen(listener, 10) == -1) {
		perror("listen");
		exit(3);
	}
	//--------------
	// ----------------------------------------------
	pthread_t hiloFileSystem;
	pthread_create(&hiloFileSystem, NULL, hiloFileSystem_Consola, NULL);
	int socketYama=0;

	while (socketYama==0) {
		int socketNuevo = accept(listener, NULL, NULL);
		t_paquete* paqueteRecibido = recibir(socketNuevo);
		switch (paqueteRecibido->codigo_operacion) { //revisar validaciones de habilitados
		case cop_handshake_yama: {
			esperar_handshake(socketNuevo, paqueteRecibido, cop_handshake_yama);
			socketYama = socketNuevo;
		}

			//enviar(socketActual, cop_datanode_info, sizeof(char*) t_datanode_info, );
			//todo mati e armar un paquete con t_datanode_info_list lista de t_nodos y enviarlo
			break;

		case cop_handshake_datanode: {
			esperar_handshake(socketNuevo, paqueteRecibido,
					cop_handshake_datanode);
			printf("Se conecto un nodo\n");
			cargarArchivoTablaNodos();
			cargarArchivos();

			t_paquete* paqueteRecibido = recibir(socketNuevo);
			//deserializa
			t_paquete_datanode_info_list* infoNodo = malloc(
					sizeof(t_paquete_datanode_info_list));
			int longitudIp;
			int longitudNombre;
			int desplazamiento = 0;
			memcpy(&longitudIp, paqueteRecibido->data + desplazamiento,
					sizeof(int));
			desplazamiento += sizeof(int);
			infoNodo->ip = malloc(longitudIp);

			memcpy(infoNodo->ip, paqueteRecibido->data + desplazamiento,
					longitudIp);
			desplazamiento += longitudIp;

			memcpy(&infoNodo->puertoWorker,
					paqueteRecibido->data + desplazamiento, sizeof(int));
			desplazamiento += sizeof(int);

			memcpy(&infoNodo->tamanio, paqueteRecibido->data + desplazamiento,
					sizeof(int));
			desplazamiento += sizeof(int);

			memcpy(&longitudNombre, paqueteRecibido->data + desplazamiento,
					sizeof(int));
			desplazamiento += sizeof(int);
			infoNodo->nombreNodo = malloc(longitudNombre);

			memcpy(infoNodo->nombreNodo, paqueteRecibido->data + desplazamiento,
					longitudNombre);
			desplazamiento += longitudNombre;

			int cantidad = (infoNodo->tamanio / (1024 * 1024)) / 8;
			if ((infoNodo->tamanio / (1024 * 1024)) % 8 != 0)
				cantidad++;

			char data[cantidad];
			t_bitarray* unBitmap;
			int i = 0;
			for (; i < cantidad; i++) {
				data[i] = 0b00000000;
			}

			unBitmap = bitarray_create(data, cantidad);

			bool esElNodo(t_nodo* nodo){
				if(strcmp(nodo->nroNodo,infoNodo->nombreNodo)==0){
					return true;
				}else{
					return false;;
				}
			}
			if(list_any_satisfy(fileSystem.ListaNodos,(void*)esElNodo)){
				t_nodo* unNodo = list_find(fileSystem.ListaNodos,(void*)esElNodo);
				unNodo->ip = malloc(strlen(infoNodo->ip)+1);
				memcpy(unNodo->ip, infoNodo->ip, strlen(infoNodo->ip)+1);
				unNodo->socket = socketNuevo;
				unNodo->puertoWorker = infoNodo->puertoWorker;
				cargarBitmapDesdeArchivo(unNodo);
			}else{
				t_nodo* unNodo = nodo_create(infoNodo->nombreNodo, false, unBitmap,
						socketNuevo, infoNodo->ip, infoNodo->puertoWorker,
						infoNodo->tamanio, (infoNodo->tamanio / (1024 * 1024)));
				list_add(fileSystem.ListaNodos, unNodo);
			}
		}
			//falta agregar otras estructuras administrativas
			break;
		case -1: {
			if (socketNuevo == socketYama) {
				printf("Se cayo Yama, finaliza FS.\n");
				exit(-1);
			} else {
				printf(
						"Se cayo un DataNode, se elimina de la lista de nodos.\n");
				bool eliminarNodoXSocket(void* elem) {
					return (((t_nodo*) elem)->socket == socketNuevo);
				}

				t_nodo* nodo = list_remove_by_condition(fileSystem.ListaNodos,
						eliminarNodoXSocket);

				free(nodo);
				}
			}
			break;
		}
	}

		//CONEXIONES
		while (1) {

			t_paquete* paqueteRecibido = recibir(socketYama);
			switch (paqueteRecibido->codigo_operacion) { //revisar validaciones de habilitados

			case cop_yama_info_fs:
			{
				char* pathArchivo = (char*) paqueteRecibido->data;
				pathArchivo = str_replace(pathArchivo, "yamafs://", "");
				bool buscarArchivoPorPath(void* elem) {
					return string_equals_ignore_case(((t_archivo*) elem)->path,
							pathArchivo);
				};

				t_archivo* archivoEncontrado = list_find(
						fileSystem.listaArchivos, buscarArchivoPorPath);
				if (archivoEncontrado != NULL) {
					t_archivoxnodo* archivoxnodo = malloc(
							sizeof(t_archivoxnodo));
					archivoxnodo->pathArchivo = pathArchivo;
					archivoxnodo->nodos = list_create();
					archivoxnodo->bloquesRelativos = list_create();

					void recopilarInfoArchivo(void* elem) {
						t_bloque* bloque = (t_bloque*) elem;

						list_add(archivoxnodo->bloquesRelativos,
								&bloque->nroBloque);

						if (bloque->copia1 != NULL) {
							recopilarInfoCopia(bloque->copia1, archivoxnodo,
									bloque);
						}

						if (bloque->copia2 != NULL) {
							recopilarInfoCopia(bloque->copia2, archivoxnodo,
									bloque);
						}
					}

					list_iterate(archivoEncontrado->bloques,
							recopilarInfoArchivo);

					int tamanioTotalBloques = 0;
					void contabilizarTamanioBloques(void* elemento) {
						tamanioTotalBloques += sizeof(int);
					}
					int cantidadElementosBloques = list_size(
							archivoxnodo->bloquesRelativos);
					list_iterate(archivoxnodo->bloquesRelativos,
							contabilizarTamanioBloques);

					int tamaniototalNodos = 0;
					void contabilizarTamanioNodo(void* elemento) {
						t_nodoxbloques* nodo = ((t_nodoxbloques*) elemento);

						tamaniototalNodos +=
								(       sizeof(int)
										+strlen(((t_nodoxbloques*) elemento)->idNodo)+ 1 //Id Nodo
										+ sizeof(int)
										+strlen(((t_nodoxbloques*) elemento)->ip)+ 1 //IP
										+ sizeof(int) //Puerto
										+ sizeof(int)/*cant bloques*/
										+ list_size(nodo->bloques) * 3
												* sizeof(int)/*cada bloque tiene 3 int*/);
					}
					;

					int cantidadElementosNodos = list_size(archivoxnodo->nodos);
					list_iterate(archivoxnodo->nodos, contabilizarTamanioNodo);

					//Serializacion
					int desplazamiento = 0;
					int longitudNombre = strlen(pathArchivo) + 1;

					void* buffer = malloc(
							sizeof(int) + longitudNombre + tamanioTotalBloques
									+ tamaniototalNodos);

					memcpy(buffer + desplazamiento, &longitudNombre,
							sizeof(int));
					desplazamiento += sizeof(int);
					memcpy(buffer + desplazamiento, archivoxnodo->pathArchivo,
							longitudNombre);
					desplazamiento += longitudNombre;

					//Lista bloques relativos
					memcpy(buffer + desplazamiento, &cantidadElementosBloques,
							sizeof(int));
					desplazamiento += sizeof(int);

					void copiarABufferBloques(void* elemento) {
						memcpy(buffer + desplazamiento, elemento, sizeof(int));
						desplazamiento += sizeof(int);
					}
					;

					list_iterate(archivoxnodo->bloquesRelativos,
							copiarABufferBloques);

					//Lista nodos
					memcpy(buffer + desplazamiento, &cantidadElementosNodos,
							sizeof(int));
					desplazamiento += sizeof(int);

					void copiarABufferNodos(void* elemento) {
						int longitudNombreNodo = strlen(((t_nodoxbloques*) elemento)->idNodo) + 1;
						memcpy(buffer + desplazamiento,	&longitudNombreNodo,sizeof(int));
						desplazamiento += sizeof(int);

						memcpy(buffer + desplazamiento,	((t_nodoxbloques*) elemento)->idNodo,longitudNombreNodo);
						desplazamiento += longitudNombreNodo;

						int longitudIP = strlen(((t_nodoxbloques*) elemento)->ip) + 1;

						memcpy(buffer + desplazamiento,	&longitudIP,sizeof(int));
						desplazamiento += sizeof(int);

						memcpy(buffer + desplazamiento,	(((t_nodoxbloques*) elemento)->ip),longitudIP);
						desplazamiento += longitudIP;


						memcpy(buffer + desplazamiento,	((t_nodoxbloques*) elemento)->puerto,sizeof(int));
						desplazamiento += sizeof(int);

						int cantidadelementos = list_size(
								((t_nodoxbloques*) elemento)->bloques);
						memcpy(buffer + desplazamiento, &cantidadelementos,
								sizeof(int));
						desplazamiento += sizeof(int);

						void copiarABufferNodosBloques(void* nodobloq) {

							t_infobloque* infobloque =
									((t_infobloque*) nodobloq);
							memcpy(buffer + desplazamiento,
									&infobloque->bloqueAbsoluto, sizeof(int));
							desplazamiento += sizeof(int);

							memcpy(buffer + desplazamiento,
									&infobloque->bloqueRelativo, sizeof(int));
							desplazamiento += sizeof(int);

							memcpy(buffer + desplazamiento,
									&infobloque->finBloque, sizeof(int));
							desplazamiento += sizeof(int);

						}
						list_iterate(((t_nodoxbloques*) elemento)->bloques,
								copiarABufferNodosBloques);
					}

					list_iterate(archivoxnodo->nodos, copiarABufferNodos);
					enviar(socketYama, cop_yama_info_fs, desplazamiento,
							buffer);
				} else {
					//todo handlear error
				}
			case cop_yama_finalizado:
			{
				int longitudNombre;
				int longitudArchivo;
				int desplazamiento=0;

				memcpy(&longitudNombre,paqueteRecibido->data,sizeof(int));
				desplazamiento+=sizeof(int);
				char* nombreArchivo = malloc(longitudNombre);
				memcpy(nombreArchivo,paqueteRecibido->data+desplazamiento,longitudNombre);
				desplazamiento+=longitudNombre;
				memcpy(&longitudArchivo,paqueteRecibido->data+desplazamiento,sizeof(int));
				desplazamiento+=sizeof(int);

				FILE* fp = fopen("archivoTemp", "w");
				if(fp){
					fwrite(paqueteRecibido->data+desplazamiento,longitudArchivo,1,fp);
					fclose(fp);
				}
				CP_FROM("archivoTemp",nombreArchivo,0);
				remove("archivoTemp");
				free(nombreArchivo);
			}
			}
				break;
			case -1: {
				printf("Se cayo Yama, finaliza FS.\n");
				exit(-1);
			}
				break;

			}
		}
		return EXIT_SUCCESS;
	}

	void recopilarInfoCopia(ubicacionBloque* copia,
			t_archivoxnodo* archivoxnodo, t_bloque* bloque) {
		t_nodoxbloques* nodo;

		bool buscarNodo(void* elem) {
			return string_equals_ignore_case(((t_nodoxbloques*) elem)->idNodo,
					bloque->copia1->nroNodo);
		}
		nodo = list_find(archivoxnodo->nodos, buscarNodo);
		if (nodo == NULL) {
			nodo = malloc(sizeof(t_nodoxbloques));
			nodo->bloques = list_create();
			nodo->idNodo = copia->nroNodo;
			nodo->ip = string_duplicate(copia->ip);
			nodo->puerto = copia->puerto;
			list_add(archivoxnodo->nodos, nodo);
		}

		t_infobloque* infobloque = malloc(sizeof(t_infobloque));

		infobloque->bloqueRelativo = bloque->nroBloque;
		infobloque->finBloque = bloque->finBloque;
		infobloque->bloqueAbsoluto = bloque->copia1->nroBloque;

		list_add(nodo->bloques, infobloque);
	}

	void CP_TO(char* origen, char*destino) {
		origen = str_replace(origen, "yamafs://", "");
		//buscar el archivo de origen en la tabla de archivos
		bool buscarArchivoPorPath(void* elem) {
			return string_equals_ignore_case(((t_archivo*) elem)->path, origen);
		}
		;

		t_archivo* archivoEncontrado = list_find(fileSystem.listaArchivos,
				buscarArchivoPorPath);

		if (archivoEncontrado == NULL) {
			printf("El path %s no se encuentra en el FS\n", origen);
			return;
		}

		FILE* fd = fopen(destino, "w");

		//crear el archivo de destino si no existe (si existe pisarlo)

		void escribirContenidoBloqueaArchivoDestino(void* elem) {
			t_bloque* bloque = (t_bloque*) elem;

			//pedir los bloques del archivo (idem CAT)

			t_nodo* nodo = NULL;
			if (bloque->copia1 != NULL)
				nodo = buscar_nodo(bloque->copia1->nroNodo);
			if (nodo != NULL) {
				void* contenido = getbloque(bloque->copia1->nroBloque, nodo);
				fwrite(contenido, bloque->finBloque, 1, fd);
				free(contenido);
				return;
			}
			else{
				if (bloque->copia2 != NULL){
					nodo = buscar_nodo(bloque->copia2->nroNodo);
				}

				if (nodo == NULL) {
					char* nombre1 = "";
					char* nombre2 = "";
					if (bloque->copia1 != NULL)
						nombre1 = bloque->copia1->nroNodo;

					if (bloque->copia2 != NULL)
						nombre2 = bloque->copia2->nroNodo;

					printf(
							"No se encontro el bloque %i en el nodo %s ni en el nodo %s\n",
							bloque->nroBloque, nombre1, nombre2);
					return;
				}

				void* contenido = getbloque(bloque->copia2->nroBloque, nodo);
				fwrite(contenido, bloque->finBloque, 1, fd);
				free(contenido);
				return;
			}

		}

		list_iterate(archivoEncontrado->bloques,
				escribirContenidoBloqueaArchivoDestino);

		fclose(fd);

		//por cada bloque voy haciendo un fwrite
	}

	void CP_FROM(char* origen, char* destino, t_tipo_archivo tipoArchivo) {
		struct stat st;
		stat(origen, &st);
		if(st.st_size == 0){
			printf("El archivo esta vacio.\n");
		}else{
			t_archivo_partido* archivoPartido = LeerArchivo(origen, tipoArchivo);
			t_archivo* nuevoArchivo = malloc(sizeof(t_archivo));
			char* path = malloc(255);
			char* file = malloc(255);
			destino = str_replace(destino, "yamafs://", "");
			split_path_file(&path, &file, destino);
			char** listaDirectorios = string_split(path, "/");

			int j = 0;
			int padre = 0;
			int cantidadDirectorios = countOccurrences(path, "/");
			if (cantidadDirectorios > 0) {
				for (; j < cantidadDirectorios; j++) { //Todo corregir, si no encuentra el directorio la funcion tiene que fallar
					t_directory* directorio = buscarDirectorio(padre,
							listaDirectorios[j]);
					padre = directorio->index;
				}
			}

			nuevoArchivo->indiceDirectorioPadre = padre;
			nuevoArchivo->nombre = file;
			nuevoArchivo->path = destino;
			nuevoArchivo->bloques = list_create();
			nuevoArchivo->tamanio = 0;
			nuevoArchivo->tipoArchivo = tipoArchivo;
			int i = 0;
			if(!elArchivoPuedeSercargado(archivoPartido->cantidadBloques)){
				printf("No hay suficiente espacio en los nodos para almacenar el archivo.\n");
				return;
			}
			for (; i < archivoPartido->cantidadBloques; i++) {
				t_nodoasignado* respuesta = escribir_bloque(list_get(archivoPartido->bloquesPartidos, i));

				t_bloque* unBloqueAux = malloc(sizeof(t_bloque));
				unBloqueAux->nroBloque = i;
				t_bloque_particion* bloqueParticion = list_get(archivoPartido->bloquesPartidos, i);
				unBloqueAux->finBloque = bloqueParticion->ultimoByteValido;
				nuevoArchivo->tamanio += unBloqueAux->finBloque;

				t_nodo* nodo1 = buscar_nodo(respuesta->nodo1);
				unBloqueAux->copia1 = malloc(sizeof(ubicacionBloque));
				unBloqueAux->copia1->nroNodo = nodo1->nroNodo;
				unBloqueAux->copia1->nroBloque = respuesta->bloque1;

				nodo1->libre--;
				if (nodo1->libre == 0)
					nodo1->ocupado = true;

				bitarray_set_bit(nodo1->bitmap, respuesta->bloque1);
				actualizarBitmap(nodo1);

				t_nodo* nodo2 = buscar_nodo(respuesta->nodo2);
				unBloqueAux->copia2 = malloc(sizeof(ubicacionBloque));
				unBloqueAux->copia2->nroNodo = nodo2->nroNodo;
				unBloqueAux->copia2->nroBloque = respuesta->bloque2;

				nodo2->libre--;
				if (nodo2->libre == 0)
					nodo2->ocupado = true;

				bitarray_set_bit(nodo2->bitmap, respuesta->bloque2);
				actualizarBitmap(nodo2);

				list_add(nuevoArchivo->bloques, unBloqueAux);
			}
			list_add(fileSystem.listaArchivos, nuevoArchivo);
			free(path);

		}

	}

	t_directory* crearDirectorio(int padre, char* nombre) {
		int x;
		for (x = 0; x < 100; x++) {

			if (tablaDeDirectorios[x]->index == -2) {
				//tablaDeDirectorios[x]->nombre= malloc(strlen(nombre)+1);
				strcpy(tablaDeDirectorios[x]->nombre, nombre);
				tablaDeDirectorios[x]->padre = padre;
				tablaDeDirectorios[x]->index = x;
				return tablaDeDirectorios[x];
			}
		}

		return NULL;
	}

	t_directory* buscarDirectorio(int padre, char* nombre) {
		int x;
		for (x = 0; x < 100; x++) {

			if (tablaDeDirectorios[x]!=NULL && tablaDeDirectorios[x]->nombre!= NULL &&
					tablaDeDirectorios[x]->padre == padre && string_equals_ignore_case(tablaDeDirectorios[x]->nombre,nombre))
				return tablaDeDirectorios[x];
		}

		return NULL;
	}

	int countOccurrences(char * str, char * toSearch) {
		int i, j, found, count;
		int stringLen, searchLen;

		stringLen = strlen(str);      // length of string
		searchLen = strlen(toSearch); // length of word to be searched

		count = 0;

		for (i = 0; i <= stringLen - searchLen; i++) {
			/* Match word with string */
			found = 1;
			for (j = 0; j < searchLen; j++) {
				if (str[i + j] != toSearch[j]) {
					found = 0;
					break;
				}
			}

			if (found == 1) {
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

	void split_path_file(char** p, char** f, char *pf) {
		char *slash = pf, *next;
		while ((next = strpbrk(slash + 1, "\\/")))
			slash = next;
		if (pf != slash)
			slash++;
		*p = strndup(pf, slash - pf);
		*f = strdup(slash);
	}

	t_archivo_partido* LeerArchivo(char* archivo, t_tipo_archivo tipoArchivo) {
		struct stat sb;
		int fd = open(archivo, O_RDONLY);
		fstat(fd, &sb);
		void* archivoMapeado = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd,
				0);

		t_archivo_partido* archivoPartido = malloc(sizeof(t_archivo_partido));
		archivoPartido->bloquesPartidos = list_create();
		if (tipoArchivo == 1) {

			int cantidadBloques = sb.st_size / 1024 * 1024;
			if (sb.st_size % (1024 * 1024) != 0)
				cantidadBloques++;
			int tamanio = 0;
			int i = 0;
			for (; i < cantidadBloques; i++)
					{
				t_bloque_particion* bloquePartido = malloc(
						sizeof(t_bloque_particion));

				int tamanioBloque = 1024 * 1024;
				if (sb.st_size - tamanio < tamanioBloque)
					tamanioBloque = sb.st_size - tamanio;

				bloquePartido->ultimoByteValido = tamanioBloque;

				bloquePartido->contenido = malloc(tamanioBloque);
				memcpy(bloquePartido->contenido, archivoMapeado + tamanio,
						tamanioBloque);
				tamanio += (tamanioBloque);
				list_add(archivoPartido->bloquesPartidos, bloquePartido);
			}
			munmap(archivoMapeado, sb.st_size);
			archivoPartido->cantidadBloques = cantidadBloques;
			return archivoPartido;
		} else {
			char* contenidoArchivo = (char*) archivoMapeado;
			int cantidadBloques = 0;
			t_bloque_particion* bloquePartido = NULL;
			int archivo = 0;
			while(sb.st_size > archivo + 1024*1024){
				char* aux=malloc(1024*1024+1);
				memset(aux,'\0',1024*1024+1);
				memcpy(aux,contenidoArchivo + archivo,1024*1024);
				char* finBloque = strrchr(aux, '\n');
				int ultimoByteValido = finBloque-aux+1;
				//logica sobre esa parte
				bloquePartido = malloc(sizeof(t_bloque_particion));
				bloquePartido->contenido = malloc(ultimoByteValido);
				memcpy(bloquePartido->contenido, aux, ultimoByteValido);
				bloquePartido->ultimoByteValido = ultimoByteValido;

				list_add(archivoPartido->bloquesPartidos, bloquePartido);

				cantidadBloques++;
				archivo += ultimoByteValido;
				free(aux);
			}

			if(sb.st_size > archivo)
			{
				char* aux=malloc(sb.st_size - archivo);
				memcpy(aux,contenidoArchivo + archivo,sb.st_size - archivo);

				int ultimoByteValido = sb.st_size - archivo-1;
				//logica sobre esa parte
				bloquePartido = malloc(sizeof(t_bloque_particion));
				bloquePartido->contenido = malloc(ultimoByteValido+1);
				memcpy(bloquePartido->contenido, aux, ultimoByteValido+1);
				bloquePartido->contenido[ultimoByteValido]='\n';
				bloquePartido->ultimoByteValido = ultimoByteValido+1;

				list_add(archivoPartido->bloquesPartidos, bloquePartido);

				cantidadBloques++;
				free(aux);
			}
			munmap(archivoMapeado, sb.st_size);
			archivoPartido->cantidadBloques = cantidadBloques;
			return archivoPartido;
		}
	}

	char** str_split(char* a_str, const char a_delim) {
		char** result = 0;
		size_t count = 0;
		char* tmp = a_str;
		char* last_comma = 0;
		char delim[2];
		delim[0] = a_delim;
		delim[1] = 0;
		/* Count how many elements will be extracted. */
		while (*tmp) {
			if (a_delim == *tmp) {
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
		if (result) {
			size_t idx = 0;
			char* token = strtok(a_str, delim);
			while (token) {
				assert(idx < count);
				*(result + idx++) = strdup(token);
				token = strtok(0, delim);
			}
			assert(idx == count - 1);
			*(result + idx) = 0;
		}
		return result;
	}

	char** validaCantParametrosComando(char* comando, int cantParametros) {
		int i = 0;
		char** parametros;
		parametros = str_split(comando, ' ');
		for (i = 1; *(parametros + i); i++) {
			printf("parametros= %s \n", *(parametros + i));
		}

		if (i != cantParametros + 1) {
			printf("%s necesita %i parametro/s. \n", comando, cantParametros);
			return NULL;
		} else {
			printf("Cantidad de parametros correcta. \n");
			return parametros;
		}
		return NULL;
	}

	void validaCantParametrosComandoRemove(char* comando) {
		char** parametros;
		parametros = str_split(comando, ' ');
		int i = 1;
		for (; *(parametros + i); i++) {
			printf("parametros= %s \n", *(parametros + i));
		}

		if (i == 2) {
			printf("Cantidad de parametros correcta. Elimina archivo. \n");
			eliminar_archivo(parametros[1]);
		} else if(i == 3){
			if(string_equals_ignore_case(parametros[1], "-d")){
				printf("Cantidad de parametros correcta. Elimina directorio. \n");
				eliminar_directorio(parametros[2]);
			}
		}else if(i == 5){
			if(string_equals_ignore_case(parametros[1], "-b")){
				printf("Cantidad de parametros correcta. Elimina bloque. \n");
				eliminar_bloque(parametros[2], atoi(parametros[3]), atoi(parametros[4]));
			}
		}else{
			printf("Parametros incorrectos. \n");
		}
	}

	void formatearFileSystem() {
		int i;
		void liberar(void* elem) {
			free(elem);
		}
		list_destroy_and_destroy_elements(fileSystem.ListaNodos, liberar);
		list_destroy_and_destroy_elements(fileSystem.listaArchivos, liberar);
		t_list* nodos = list_create();
		fileSystem.ListaNodos = nodos;

		for (i = 0; i < sizeof(tablaDeDirectorios) / sizeof(t_directory); i++) {
			tablaDeDirectorios[i]->padre = -1;
		}
		printf("Filesystem formateado.\n");
	}

	void cp_block(char* path, int numeroBloque, char* nombreNodo) {
		t_nodo* nodoDestino = buscar_nodo(nombreNodo);
		if (nodoDestino == NULL || nodoDestino->ocupado) {
			printf("no existe el nodo %s o está ocupado. \n", nombreNodo);
			return;

		}

		bool buscarArchivoPorPath(void* elem) {
			return string_equals_ignore_case(((t_archivo*) elem)->path, path);
		}
		;

		t_archivo* archivoEncontrado = list_find(fileSystem.listaArchivos,
				buscarArchivoPorPath);
		if (archivoEncontrado == NULL) {
			printf("El path %s no se encuentra en el FS. \n", path);
			return;
		}

		bool buscarBloquePorNumero(void* elem) {
			return ((t_bloque*) elem)->nroBloque == numeroBloque;
		}

		t_bloque* bloque = list_find(archivoEncontrado->bloques,
				buscarBloquePorNumero);

		if (bloque == NULL) {
			printf("El bloque %i no se encuentra en el FS.\n", numeroBloque);
			return;
		}

		ubicacionBloque* ubicacionOrigen = bloque->copia1;

		if (ubicacionOrigen == NULL) {
			ubicacionOrigen = bloque->copia2;
		}

		if (ubicacionOrigen != NULL) {

			t_nodo* nodoOrigen = buscar_nodo(ubicacionOrigen->nroNodo);
			if(string_equals_ignore_case(ubicacionOrigen->nroNodo, nombreNodo) || (string_equals_ignore_case(bloque->copia2->nroNodo,nombreNodo)))
					{
				//error
				printf("Erorr, ya existe esa copia ./n");
					}
			void* contenido = getbloque(ubicacionOrigen->nroBloque, nodoOrigen);

			int numBloqueDestino = buscarBloque(nodoDestino);
			nodoDestino->libre--;
			if (nodoDestino->libre == 0)
				nodoDestino->ocupado = true;
			bitarray_set_bit(nodoDestino->bitmap, numBloqueDestino);
			enviar_bloque_a_escribir(numBloqueDestino, contenido, nodoDestino,
					1024 * 1024); //todo verificar si funciona ok con 1024*1024

			ubicacionBloque* bloqueAsignado = malloc(sizeof(ubicacionBloque));
			bloqueAsignado->nroBloque = numBloqueDestino;
			bloqueAsignado->nroNodo = malloc(strlen(nombreNodo) + 1);
			strcpy(bloqueAsignado->nroNodo, nombreNodo);
			if (bloque->copia1 == NULL)
				bloque->copia1 = bloqueAsignado;
			else
				bloque->copia2 = bloqueAsignado;
		} else {
			printf("No tiene ninguna copia para tomar de origen.\n");
			return;
		}
	}

	void YAMA_mkdir(char* path) {
		path = str_replace(path, "yamafs://", "");
		if (!string_ends_with(path, "/")) {
			string_append(&path, "/");
		}

		int cantidadDirectorios = countOccurrences(path, "/");
		char** directorios = string_split(path, "/");
		int i = 0;
		int indicePadre = 0;
		t_directory* directorioActual = NULL;
		if (cantidadDirectorios == 1) {
			indicePadre = 0;
		} else { //revisar para el caso que crean multiples directorios
			for (; i < cantidadDirectorios - 1; i++) {
				if (directorios[i] == NULL)
					break;
				directorioActual = buscarDirectorio(indicePadre,
						directorios[i]);
				if (directorioActual == NULL) {
					printf("El directorio %s no existe para el padre %i\n",
							directorios[i], indicePadre);
					return;
				}
				indicePadre = directorioActual->index;
			}
		}

		int libre = 0;
		while (tablaDeDirectorios[libre]->index != -2 && libre < 100) {
			libre++;
		}
		if (libre == 100) {
			printf("La tabla de directorios esta llena\n");
		} else {
			tablaDeDirectorios[libre]->index = libre;
			tablaDeDirectorios[libre]->nombre = directorios[cantidadDirectorios
					- 1];
			tablaDeDirectorios[libre]->padre = indicePadre;
			printf("Se creo el directorio\n");
		}
		return;
	}

	void ls(char*path) {
		path = str_replace(path, "yamafs://", "");
		if (!string_ends_with(path, "/")) {
			string_append(&path, "/");
		}

		int cantidadDirectorios = countOccurrences(path, "/");
		char** directorios = string_split(path, "/");
		int i = 0;
		int indicePadre = 0;
		t_directory* directorioActual = NULL;
		for (; i < cantidadDirectorios; i++) {
			if (directorios[i] == NULL)
				break;
			directorioActual = buscarDirectorio(indicePadre, directorios[i]);
			if (directorioActual == NULL) {
				printf("El directorio %s no existe para el padre %i\n",
						directorios[i], indicePadre);
				return;
			}
			indicePadre = directorioActual->index;
		}

		t_list* listaDirectorios = obtenerSubdirectorios(indicePadre);
		bool buscarArchivoPorPath(void* elem) {
			char* partial_string = str_replace(((t_archivo*) elem)->path, path,
					"");
			return string_equals_ignore_case(partial_string,
					((t_archivo*) elem)->nombre);
		}
		;

		t_list* listaArchivos = list_filter(fileSystem.listaArchivos,
				buscarArchivoPorPath);

		void imprimirDirectorios(t_directory* elem) {
			if (elem->nombre != NULL && elem->index != -2) {
				printf("Directorio: %s\n", elem->nombre);
			}
		}

		void imprimirArchivos(t_archivo* elem) {
			printf("Archivo: %s\n", elem->nombre);
		}
		if(list_size(listaDirectorios)!=0){
			list_iterate(listaDirectorios, (void*) imprimirDirectorios);
		}
		if(list_size(listaArchivos)!=0){
			list_iterate(listaArchivos, (void*) imprimirArchivos);
		}
	}

	t_list* obtenerSubdirectorios(int indicePadre) {
		int x;
		t_list* listaDirectorios = list_create();
		for (x = 0; x < 100; x++) {
			if (tablaDeDirectorios[x]->padre == indicePadre && tablaDeDirectorios[x]->index != -2){
				list_add(listaDirectorios, tablaDeDirectorios[x]);
			}
		}

		return listaDirectorios;

	}

	void calcular_md5(char* path) {
		path = str_replace(path, "yamafs://", "");
		bool buscarArchivoPorPath(void* elem) {
			return string_equals_ignore_case(((t_archivo*) elem)->path, path);
		}
		;

		t_archivo* archivoEncontrado = list_find(fileSystem.listaArchivos,
				buscarArchivoPorPath);

		if (archivoEncontrado == NULL) {
			printf("El path %s no se encuentra en el FS\n", path);
			return;
		}

		void* buffer = malloc(archivoEncontrado->tamanio);
		int desplazamiento = 0;
		void concatenarBloques(void* elem) {
			t_bloque* bloque = (t_bloque*) elem;

			t_nodo* nodo = NULL;
			if (bloque->copia1 != NULL)
				nodo = buscar_nodo(bloque->copia1->nroNodo);

			if (nodo != NULL) {
				void* contenido = getbloque(bloque->copia1->nroBloque, nodo);
				int longitudBloque;
				if (archivoEncontrado->tipoArchivo == BINARIO)
					longitudBloque = 1024 * 1024;
				else
					longitudBloque = bloque->finBloque;

				memcpy(buffer + desplazamiento, contenido, longitudBloque);
				desplazamiento += longitudBloque;
				return;
			} else {
				if (bloque->copia2 != NULL)
					nodo = buscar_nodo(bloque->copia2->nroNodo);

				if (nodo == NULL) {
					char* nombre1 = "";
					char* nombre2 = "";
					if (bloque->copia1 != NULL)
						nombre1 = bloque->copia1->nroNodo;

					if (bloque->copia2 != NULL)
						nombre2 = bloque->copia2->nroNodo;

					printf(
							"No se encontro el bloque %i en el nodo %s ni en el nodo %s\n",
							bloque->nroBloque, nombre1, nombre2);
					return;
				}

				void* contenido = getbloque(bloque->copia1->nroBloque, nodo);
				int longitudBloque;
				if (archivoEncontrado->tipoArchivo == BINARIO)
					longitudBloque = 1024 * 1024;
				else
					longitudBloque = bloque->finBloque;

				memcpy(buffer + desplazamiento, contenido, longitudBloque);
				desplazamiento += longitudBloque;
				return;

				return;
			}

		}

		list_iterate(archivoEncontrado->bloques, concatenarBloques);
		char* pathArchivo = "./temp-MD5SUM";
		FILE *fp = fopen(pathArchivo, "w+");

		fwrite(buffer, sizeof(char), archivoEncontrado->tamanio, fp);
		fclose(fp);
		char* comando =string_from_format("md5sum %s", pathArchivo);
		system(comando);
	}

	void cat(char* path) {
		path = str_replace(path, "yamafs://", "");
		bool buscarArchivoPorPath(void* elem) {
			return string_equals_ignore_case(((t_archivo*) elem)->path, path);
		}		;

		t_archivo* archivoEncontrado = list_find(fileSystem.listaArchivos,
				buscarArchivoPorPath);

		if (archivoEncontrado == NULL) {
			printf("El path %s no se encuentra en el FS\n", path);
			return;
		}

		void imprimirPorPantallaContenidoBloque(void* elem) {
			t_bloque* bloque = (t_bloque*) elem;

			t_nodo* nodo = NULL;
			if (bloque->copia1 != NULL)
				nodo = buscar_nodo(bloque->copia1->nroNodo);
			if (nodo != NULL) {
				void* contenido = getbloque(bloque->copia1->nroBloque, nodo);
				fwrite(contenido, bloque->finBloque, 1, stdout);
				return;
			} else {
				if (bloque->copia2 != NULL)
					nodo = buscar_nodo(bloque->copia2->nroNodo);

				if (nodo == NULL) {
					char* nombre1 = "";
					char* nombre2 = "";
					if (bloque->copia1 != NULL)
						nombre1 = bloque->copia1->nroNodo;

					if (bloque->copia2 != NULL)
						nombre2 = bloque->copia2->nroNodo;

					printf(
							"No se encontro el bloque %i en el nodo %s ni en el nodo %s\n",
							bloque->nroBloque, nombre1, nombre2);
					return;
				}

				void* contenido = getbloque(bloque->copia2->nroBloque, nodo);
				fwrite(contenido, bloque->finBloque, 1, stdout);
				return;
			}

		}

		list_iterate(archivoEncontrado->bloques,
				imprimirPorPantallaContenidoBloque);
	}

	void eliminar_archivo(char* path) {
		path=str_replace(path, "yamafs://","");
		bool buscarArchivoPorPath(void* elem) {
			return string_equals_ignore_case(((t_archivo*) elem)->path, path);
		};
		t_archivo* unArchivo = list_find(fileSystem.listaArchivos, buscarArchivoPorPath);
		if(unArchivo == NULL)
		{
			printf("El archivo %s no existe", path);
			return;
		}

		void liberarBloques(void* elem)
		{
			t_bloque* bloque=(t_bloque*)elem;
			if(bloque->copia1 != NULL){
				t_nodo* nodo=buscar_nodo(bloque->copia1->nroNodo);
				bitarray_clean_bit(nodo->bitmap,bloque->copia1->nroBloque);
				free(bloque->copia1);
			}

			if(bloque->copia2 != NULL){
				t_nodo* nodo=buscar_nodo(bloque->copia2->nroNodo);
				bitarray_clean_bit(nodo->bitmap,bloque->copia2->nroBloque);
				free(bloque->copia2);
			}
			free(bloque);
		}
		free(unArchivo->nombre);
		free(unArchivo->path);
		list_destroy_and_destroy_elements(unArchivo->bloques, liberarBloques);
		free(unArchivo);
		return;
	}

		void eliminar_directorio(char* path) {
				path=str_replace(path, "yamafs://","");
				if (!string_ends_with(path, "/"))
					string_append(&path, "/");
				int cantidadDirectorios=countOccurrences(path, "/");
				char** directorios = string_split(path, "/");
				t_directory* directorioAEliminar = NULL;
				int indicePadre=0;
				int i=0;
				for (; i < cantidadDirectorios; i++) {
					if (directorios[i] == NULL)
						break;
					directorioAEliminar = buscarDirectorio(indicePadre, directorios[i]);
					indicePadre = directorioAEliminar->padre;
				}
				bool buscarArchivoPorPath(void* elem) {
					return string_contains(path, (char*)elem);
				}
				if(!list_any_satisfy(fileSystem.listaArchivos, buscarArchivoPorPath)) {
					free(directorioAEliminar->nombre);
					directorioAEliminar->nombre = NULL;
					directorioAEliminar->padre=-2;
				}
				else {
					printf("No se puede eliminar el directorio porque no esta vacio \n");
				}
		}

		void eliminar_bloque(char* path, int nroBloque, int nroCopia){
			path=str_replace(path, "yamafs://","");
			bool buscarArchivoPorPath(void* elem) {
				return string_equals_ignore_case(((t_archivo*) elem)->path, path);
			};
			t_archivo* unArchivo = list_find(fileSystem.listaArchivos, buscarArchivoPorPath);
			if(unArchivo == NULL)
			{
				printf("El archivo %s no existe", path);
				return;
			}

			bool buscarBloque(void* elem) {
				t_bloque* bloque=(t_bloque*)elem;
				return bloque->nroBloque == nroBloque;
			}

			t_bloque* bloque=list_find(unArchivo->bloques, buscarBloque);
			if(bloque == NULL)
			{
				printf("No existe el numero de bloque \n");
			}
			if((bloque->copia1 == NULL && nroCopia == 2)|| (bloque->copia2 == NULL && nroCopia == 1)){
				printf("No se puede eliminar ya que solo posee una copia \n");
			}
			if(nroCopia == 1 && bloque->copia1 != NULL){
				t_nodo* nodo=buscar_nodo(bloque->copia1->nroNodo);
				bitarray_clean_bit(nodo->bitmap,bloque->copia1->nroBloque);
				free(bloque->copia1);
				bloque->copia1 = NULL;
				printf("Eliminando copia 1 \n");
			}
			else if(nroCopia == 2 && bloque->copia2 != NULL) {
				t_nodo* nodo=buscar_nodo(bloque->copia2->nroNodo);
				bitarray_clean_bit(nodo->bitmap,bloque->copia2->nroBloque);
				free(bloque->copia2);
				bloque->copia2 = NULL;
				printf("Eliminando copia 2 \n");
			}
	}

	void hiloFileSystem_Consola(void * unused) {
		printf("Consola Iniciada. Ingrese una opcion: \n");
		char * linea;
		char* primeraPalabra;
		char* context;

		while (1) {
			linea = readline(">");
			if (!linea || string_equals_ignore_case(linea, "")) {
				continue;
			} else {
				add_history(linea);
				char** parametros;
				int lineaLength = strlen(linea);
				char *lineaCopia = (char*) calloc(lineaLength + 1,
						sizeof(char));
				strncpy(lineaCopia, linea, lineaLength);
				primeraPalabra = strtok_r(lineaCopia, " ", &context);

				if (strcmp(linea, "format") == 0) {
					printf("Formatear el Filesystem\n");
					formatearFileSystem();
					free(linea);
				} else if (strcmp(primeraPalabra, "rm") == 0) {
					printf("Eliminar un Archivo/Directorio/Bloque. \n");
					validaCantParametrosComandoRemove(linea);
					free(linea);
				} else if (strcmp(primeraPalabra, "rename") == 0) {
					printf("Renombra un Archivo o Directorio\n");
					parametros = validaCantParametrosComando(linea, 3);
					if (parametros != NULL) {
						yama_rename(parametros[1], parametros[2], parametros[3]);
					} else {
						printf(
								"El rename debe recibir path_original nombre_final. \n");
					}

					free(linea);
				} else if (strcmp(primeraPalabra, "mv") == 0) {
					printf("Mueve un Archivo o Directorio\n");
					parametros = validaCantParametrosComando(linea, 3);
					if (parametros != NULL) {
						yama_mv(parametros[1], parametros[2],
								(char) parametros[3][0]);
					} else {
						printf(
								"El mv debe recibir path_origen path_destino y tipo ('a' o 'd')\n");
					}

					free(linea);
				} else if (strcmp(primeraPalabra, "cat") == 0) {
					printf(
							"Muestra el contenido del archivo como texto plano.\n");
					parametros = validaCantParametrosComando(linea, 1);
					if (parametros != NULL) {
						cat(parametros[1]);
					} else {
						printf("El cat debe recibir path archivo. \n");
					}

					free(linea);
				} else if (strcmp(primeraPalabra, "mkdir") == 0) {
					printf(
							"Crea un directorio. Si el directorio ya existe, el comando deberá informarlo.\n");
					parametros = validaCantParametrosComando(linea, 1);
					if (parametros != NULL) {
						YAMA_mkdir(parametros[1]);
					} else {
						printf(
								"El mkdir debe recibir el path del directorio. \n");
					}

					free(linea);
				} else if (strcmp(primeraPalabra, "cpfrom") == 0) {
					printf(
							"Copiar un archivo local al yamafs, siguiendo los lineamientos en la operaciòn Almacenar Archivo, de la Interfaz del FileSystem.\n");
					parametros = validaCantParametrosComando(linea, 3);
					if (parametros != NULL) {
						CP_FROM(parametros[1], parametros[2],
								atoi(parametros[3]));
					} else {
						printf(
								"El cpfrom debe recibir el path del archivo y el directorio de fs. \n");
					}

					free(linea);
				} else if (strcmp(primeraPalabra, "cpto") == 0) {
					printf("Copiar un archivo local al yamafs\n");
					parametros = validaCantParametrosComando(linea, 2);
					if (parametros != NULL) {
						CP_TO(parametros[1], parametros[2]);
					} else {
						printf(
								"El cpto debe recibir el path_archivo_yamafs y el directorio_filesystem. \n");
					}

					free(linea);
				} else if (strcmp(primeraPalabra, "cpblock") == 0) {
					printf(
							"Crea una copia de un bloque de un archivo en el nodo dado.\n");
					parametros = validaCantParametrosComando(linea, 3);
					if (parametros != NULL) {
						cp_block(parametros[1],atoi( parametros[2]),parametros[3]);
					} else {
						printf(
								"El cpblock debe recibir el path_archivo, el nro_bloque y el id_nodo. \n");
					}

					free(linea);
				} else if (strcmp(primeraPalabra, "md5") == 0) {
					printf("Solicitar el MD5 de un archivo en yamafs\n");
					parametros = validaCantParametrosComando(linea, 1);
					if (parametros != NULL) {
						calcular_md5(parametros[1]);
					} else {
						printf(
								"El mds5 debe recibir el path_archivo_yamafs. \n");
					}

					free(linea);
				} else if (strcmp(primeraPalabra, "ls") == 0) {
					printf("Lista los archivos de un directorio\n");
					parametros = validaCantParametrosComando(linea, 1);
					if (parametros != NULL) {
						ls(parametros[1]);

					} else {
						printf("El ls debe recibir el path_directorio. \n");
					}

					free(linea);
				} else if (strcmp(primeraPalabra, "info") == 0) {
					printf(
							"Muestra toda la información del archivo, incluyendo tamaño, bloques, ubicación de los bloques, etc.\n");
					parametros = validaCantParametrosComando(linea, 1);
					if (parametros != NULL) {
						info_archivo(parametros[1]);
					} else {
						printf("El info debe recibir el path_archivo. \n");
					}

					free(linea);
				} else {
					printf("Opcion no valida.\n");
					free(linea);
				}
				actualizarArchivoDeDirectorios();
				actualizarArchivoTablaNodos();
				actualizarTablaArchivos();
				free(lineaCopia);
				if (parametros != NULL)
					free(parametros);
			}
		}
	}

	t_nodo* buscar_nodo(char* nombreNodo) {
		bool buscarNodo(void* elemento) {
			return string_equals_ignore_case(((t_nodo*) elemento)->nroNodo,
					nombreNodo);
		}
		return list_find(fileSystem.ListaNodos, buscarNodo);
	}

	t_nodo* buscar_nodo_libre(char* nodoAnterior) {
		//todo hacer un random para que no asigne en el mismo orden
		bool buscarLibre(void* elemento) {
			return !((t_nodo*) elemento)->ocupado && (nodoAnterior == 0 || ((t_nodo*) elemento)->nroNodo != nodoAnterior);
		}

		t_list* nodosCandidatos=list_filter(fileSystem.ListaNodos, buscarLibre);

		int i=rand() % list_size(nodosCandidatos);
		return list_get(nodosCandidatos, i);

	}

	int buscarBloque(t_nodo* nodo) {
		int tamanio = nodo->cantidadBloques;
		int i = 0;
		for (; i < tamanio; i++) {
			bool ocupado = bitarray_test_bit(nodo->bitmap, i);
			if (!ocupado)
				return i;
		}
		return -1;
	}

	void* getbloque(int numBloque, t_nodo* nodo) {
		t_getbloque* bloque = malloc(sizeof(t_getbloque));
		bloque->numero_bloque = numBloque;
		void* buffer = malloc(sizeof(int));
		int desplazamiento = 0;
		memcpy(buffer, &bloque->numero_bloque, sizeof(int));
		desplazamiento += sizeof(int);
		enviar(nodo->socket, cop_datanode_get_bloque, desplazamiento, buffer);
		t_paquete* paquete = recibir(nodo->socket);
		return paquete->data;

	}

	void enviar_bloque_a_escribir(int numBloque, void* contenido, t_nodo* nodo,
			int ultimoByteValido) {
		t_setbloque* bloque = malloc(sizeof(t_setbloque));
		bloque->numero_bloque = numBloque;
		bloque->datos_bloque = malloc(1024 * 1024);
		memset(bloque->datos_bloque, '\0', 1024*1024);
		memcpy(bloque->datos_bloque, contenido, ultimoByteValido);
		void* buffer = malloc(sizeof(int) + 1024 * 1024); //numero bloque
		int desplazamiento = 0;
		memcpy(buffer, &bloque->numero_bloque, sizeof(int));
		desplazamiento += sizeof(int);
		memcpy(buffer + desplazamiento, bloque->datos_bloque, 1024 * 1024); //datos->bloque
		desplazamiento += 1024 * 1024;
		enviar(nodo->socket, cop_datanode_setbloque, desplazamiento, buffer);
	}

	t_nodoasignado* escribir_bloque(void* bloque) {
		t_nodoasignado* respuesta = malloc(sizeof(t_nodoasignado));
		t_bloque_particion* bloque_partido = (t_bloque_particion*) bloque;
		void* buffer = malloc(1024 * 1024);
		memcpy(buffer, bloque_partido->contenido,
				bloque_partido->ultimoByteValido);

		t_nodo* nodolibre = buscar_nodo_libre(0);
		if(nodolibre == NULL){
			printf("no hay nodo libre\n");
			return NULL;
		}

		int numBloque = buscarBloque(nodolibre);
		if(numBloque == -1){
			printf("no hay bloque libre\n");
			return NULL;
		}
		enviar_bloque_a_escribir(numBloque, buffer, nodolibre,
				bloque_partido->ultimoByteValido);
		respuesta->nodo1 = string_duplicate(nodolibre->nroNodo);
		respuesta->bloque1 = numBloque;

		nodolibre = buscar_nodo_libre(nodolibre->nroNodo);
		if(nodolibre == NULL){
			printf("no hay nodo libre\n");
			return NULL;
		}
		numBloque = buscarBloque(nodolibre);
		if(numBloque == -1){
			printf("no hay bloque libre\n");
			return NULL;
		}
		enviar_bloque_a_escribir(numBloque, buffer, nodolibre,
				bloque_partido->ultimoByteValido);
		respuesta->bloque2 = numBloque;
		respuesta->nodo2 = string_duplicate(nodolibre->nroNodo);

		return respuesta;
	}

	bool cargarDirectoriosDesdeArchivo(){
		if( access("metadata/directorios.txt", F_OK ) != -1 ) {
		    // file exists
			FILE * fp;
			char * line = NULL;
			size_t len = 0;
			ssize_t read;

			fp = fopen("metadata/directorios.txt", "r");
			if (fp == NULL){
				printf("No se pudo recuperar el estado anterior de la tabla de directorios.\n");
				return false;
			}
			int i = 0;
			while ((read = getline(&line, &len, fp)) != -1) {
				line = str_replace(line, "\n", "");
				char** linea = str_split(line,',');
				tablaDeDirectorios[i]->index = atoi(linea[0]);
				tablaDeDirectorios[i]->nombre = linea[1];
				tablaDeDirectorios[i]->padre = atoi(linea[2]);
				i++;
				free(line);
				line = malloc(200);
			}

			fclose(fp);
			if (line)
				free(line);
			return true;
		} else {
		    // file doesn't exist
			printf("No existe un estado anterior de directorios. \n");
			return false;
		}
	}

	void actualizarArchivoDeDirectorios() {
		DIR* dir = opendir("metadata");
		if (dir){
			crear_subcarpeta("metadata");
			closedir(dir);
		}
		FILE * file = fopen("metadata/directorios.txt", "w");
		int i;
		if (file != NULL) {
			fprintf(file, "%i,%s,%i", tablaDeDirectorios[0]->index,
					tablaDeDirectorios[0]->nombre,
					tablaDeDirectorios[0]->padre);
			for (i = 1; i < 100; i++) {
				if (tablaDeDirectorios[i]->index != -2) {
					fprintf(file, "\n%i,%s,%i", tablaDeDirectorios[i]->index,
							tablaDeDirectorios[i]->nombre,
							tablaDeDirectorios[i]->padre);
				}
			}
			fclose(file);
		}
	}

	void crear_subcarpeta(char* nombre) {
		struct stat st = { 0 };
		if (stat(nombre, &st) == -1) {
			mkdir(nombre, 0700);
		}
	}

	void cargarBitmapDesdeArchivo(t_nodo* unNodo){
		char* aux = malloc(200);
		memset(aux, '\0', 200);
		aux = string_from_format("metadata/bitmaps/%s.dat",unNodo->nroNodo);

		FILE * file = fopen(aux, "r");
		if (file != NULL) {

			int cantidad = (unNodo->tamanio / (1024 * 1024)) / 8;
			if ((unNodo->tamanio / (1024 * 1024)) % 8 != 0){
				cantidad++;
			}
			char* bitarray=malloc(cantidad);
			memset(bitarray,'\0',cantidad);
			fread(bitarray, cantidad,1, file);
			unNodo->bitmap = bitarray_create(bitarray,cantidad);

			fclose(file);
		}
		free(aux);
	}

	void actualizarBitmap(t_nodo* unNodo) {
		crear_subcarpeta("metadata");
		crear_subcarpeta("metadata/bitmaps/");

		char* aux = malloc(200);
		memset(aux, '\0', 200);
		aux = string_from_format("metadata/bitmaps/%s.dat",unNodo->nroNodo);
		FILE * file = fopen(aux, "w");
		if (file != NULL) {
			fwrite(unNodo->bitmap->bitarray, strlen(unNodo->bitmap->bitarray),
					1, file);
			fclose(file);
		}
		free(aux);
	}

	void cargarArchivoTablaNodos(){
		FILE * file = fopen("metadata/nodos.bin", "r");
		if (file != NULL) {
			char * line = NULL;
			size_t len = 0;
			ssize_t read;
			int i = 0;
			int cantidadNodos=0;
			char** nodos;
			bool esElNodoByName(t_nodo* unNodo){
				if(strcmp(unNodo->nroNodo,nodos[i])==0){
					return true;
				}else{
					return false;
				}
			}
			while((read = getline(&line, &len, file)) != -1){
				line = str_replace(line,"\n","");
				switch(i){
				case 0:
					fileSystem.tamanio = atoi(str_replace(line,"TAMANIO=",""));
					free(line);
					line = malloc(200);
					break;
				case 1:
					fileSystem.libre = atoi(str_replace(line, "LIBRE=",""));
					free(line);
					line = malloc(200);
					break;
				case 2:
					line = str_replace(line,"NODOS=[","");
					line = str_replace(line,"]","");
					cantidadNodos = countOccurrences(line,",")+1;
					nodos = str_split(line,',');
					free(line);
					line = malloc(200);
					i = 0;
					int tamanio;
					int libre;
					for(;i<cantidadNodos;i++){
						//Total
						if((read = getline(&line, &len, file)) != -1){
							line = str_replace(line,"\n","");
							line = str_replace(line, nodos[i],"");
							line = str_replace(line, "Total=","");
							tamanio = atoi(line);
							free(line);
							line = malloc(200);
						}
						//Libre
						if((read = getline(&line, &len, file)) != -1){
							line = str_replace(line,"\n","");
							line = str_replace(line, nodos[i],"");
							line = str_replace(line, "Libre=","");
							libre = atoi(line);
							free(line);
							line = malloc(200);
						}
						if(list_any_satisfy(fileSystem.ListaNodos,(void*)esElNodoByName)){
							t_nodo* unNodoAux = list_find(fileSystem.ListaNodos,(void*)esElNodoByName);
							unNodoAux->tamanio = tamanio;
							unNodoAux->libre = libre;
							unNodoAux->cantidadBloques = tamanio / (1024*1024);
							if(tamanio % (1024 * 1024) != 0)
								unNodoAux->cantidadBloques++;
						}else{
							t_nodo* unNodo = nodo_create(nodos[i], false, NULL,-2, "ipWorker", -1,tamanio, libre);
							list_add(fileSystem.ListaNodos,unNodo);
						}

					}
					break;
				}
				i++;
			}
			if(line){
				free(line);
			}
		}
		return;
	}

	void actualizarArchivoTablaNodos() {
		crear_subcarpeta("metadata");
		FILE * file = fopen("metadata/nodos.bin", "w");
		if (file != NULL) {
			int libreTotal;
			libreTotal = 0;
			int tamanioTotal;
			tamanioTotal = 0;
			int cantidadNodos;
			cantidadNodos = list_size(fileSystem.ListaNodos);
			int libresPorNodo[cantidadNodos];
			int tamanioPorNodo[cantidadNodos];
			char* nodos[cantidadNodos];
			int i;
			for (i = 0; i < cantidadNodos; i++) {
				t_nodo* aux;
				aux = list_get(fileSystem.ListaNodos, i);
				actualizarBitmap(aux);
				libreTotal += aux->libre;
				tamanioTotal += aux->tamanio;
				libresPorNodo[i] = aux->libre;
				tamanioPorNodo[i] = aux->tamanio;
				nodos[i] = aux->nroNodo;
			}
			fprintf(file, "TAMANIO=%i\nLIBRE=%i\nNODOS=[", tamanioTotal,
					libreTotal);
			for (i = 0; i < cantidadNodos - 1; i++) {
				fprintf(file, "%s,", nodos[i]);
			}
			fprintf(file, "%s]\n", nodos[cantidadNodos-1]);
			for (i = 0; i < cantidadNodos; i++) {
				fprintf(file, "%sTotal=%i\n%sLibre=%i\n", nodos[i],
						tamanioPorNodo[i], nodos[i], libresPorNodo[i]);
			}
			fclose(file);
		}
	}

	void actualizarTablaArchivos() {
		//limpiar todas las metadata
		int i = 0;
		int tamanio = list_size(fileSystem.listaArchivos);
		for (; i < tamanio; i++) {
			actualizarArchivo(list_get(fileSystem.listaArchivos, i));
		}
	}

	void cargarArchivos(){
		DIR* d;
		struct dirent *dir;
		d = opendir("metadata/archivos/");
		if (d){
			while ((dir = readdir(d)) != NULL){
				if(dir->d_type==DT_DIR){
					int aux = atoi(dir->d_name);
					cargarArchivosEnUnSubdirectorio(aux);
				}
			}
		closedir(d);
		}
	}

	void cargarArchivosEnUnSubdirectorio(int subdirectorio){
		DIR* d;
		struct dirent *dir;
		char* aux=string_from_format("./metadata/archivos/%i/",subdirectorio);
		d = opendir(aux);
		if (d){
			while ((dir = readdir(d)) != NULL){
				if(dir->d_type==DT_REG){
					char* path=string_from_format("%s%s", aux, dir->d_name);
					list_add(fileSystem.listaArchivos,cargarArchivoDesdeArchivo(path,subdirectorio));
				}
			}
		closedir(d);
		}
	}

	t_archivo* cargarArchivoDesdeArchivo(char* path, int directorioPadre){
		t_archivo* unArchivo=malloc(sizeof(t_archivo));
		FILE * fp;
		char * line = malloc(255);
		size_t len = 0;
		ssize_t read;

		fp = fopen(path, "r");
		if (fp == NULL){
			printf("No se pudo recuperar el estado anterior del archivo");
			exit(EXIT_FAILURE);
		}
		int i = 0;
		int cantidadBloques = 0;

		for(;i<3;i++){
			if((read = getline(&line, &len, fp)) != -1){
				char* aux = str_replace(line,"\n","");
				switch(i){
					case 0:
						unArchivo->path = malloc(strlen(aux)+1);
						memcpy(unArchivo->path,aux,strlen(aux)+1);
						break;
					case 1:
						unArchivo->tamanio = strtoul(str_replace(aux, "TAMANIO=", ""),NULL,10);
						cantidadBloques = unArchivo->tamanio / (1024 *1024);
						if(unArchivo->tamanio % (1024 *1024)  != 0){
							cantidadBloques++;
						}
						break;
					case 2:
						unArchivo->tipoArchivo = atoi(str_replace(aux,"TIPO=",""));
						break;
				}
				if(aux){
					free(aux);
				}
			}
		}

		i = 0;
		t_list* bloques = list_create();
		for(;i<cantidadBloques;i++){
			t_bloque* unBloque=malloc(sizeof(t_bloque));
			ubicacionBloque* copia1 = malloc(sizeof(ubicacionBloque));
			ubicacionBloque* copia2 = malloc(sizeof(ubicacionBloque));
			unBloque->nroBloque = i;
			//copia 0
			if((read = getline(&line, &len, fp)) != -1){
				line = str_replace(line,"\n","");
				char* lineaBloq1=string_from_format("BLOQUE%iCOPIA%i=",i,0);
				line = str_replace(line,lineaBloq1,"");
				if (line!=NULL){
					char** valores = str_split(line,',');
					char* auxNNodo = str_replace(valores[0],"[","");
					char* auxNBloque = str_replace(valores[1],"]","");
					int numeroBloque=atoi(auxNBloque);
					copia1->nroNodo=malloc(strlen(auxNNodo)+1);
					memcpy(copia1->nroNodo,auxNNodo , strlen(auxNNodo)+1);
					memcpy(&copia1->nroBloque, &numeroBloque, sizeof(int));
					unBloque->copia1 = copia1;
					free(auxNNodo);
					free(auxNBloque);
					free(line);
					free(valores);
					}
				line = malloc(200);
			}
			//copia 1
			if((read = getline(&line, &len, fp)) != -1){
				line = str_replace(line,"\n","");
				char* lineaBloq2=string_from_format("BLOQUE%iCOPIA%i=",i,1);
				line = str_replace(line,lineaBloq2,"");
				if (line!=NULL){
					char** valores = str_split(line,',');
					char* auxNNodo= str_replace(valores[0],"[","");
					char* auxNBloque = str_replace(valores[1],"]","");
					int numeroBloque = atoi(auxNBloque);
					copia2->nroNodo = malloc(strlen(auxNNodo)+1);
					memcpy(copia2->nroNodo,auxNNodo,strlen(auxNNodo)+1);
					memcpy(&copia2->nroBloque, &numeroBloque, sizeof(int));
					unBloque->copia2 = copia2;
					free(auxNNodo);
					free(auxNBloque);
					free(line);
					free(valores);
					}
				line = malloc(200);
			}
			//Fin de bloque
			if((read = getline(&line, &len, fp)) != -1){
				line = str_replace(line,"\n","");
				char* finDeBloque=string_from_format("BLOQUE%iBYTES=",i);
				unBloque->finBloque = strtoul(str_replace(line,finDeBloque,""),NULL,10);
				free(line);
				line = malloc(200);
			}
			list_add(bloques,unBloque);
		}
		unArchivo->bloques = bloques;
		fclose(fp);
		if(line){
			free(line);
		}
		char* p;
		split_path_file(&p, &unArchivo->nombre ,path);
		unArchivo->indiceDirectorioPadre = directorioPadre;
		return unArchivo;
	}

	void actualizarArchivo(t_archivo* unArchivo) {
		char* path=malloc(255);
		char* file=malloc(255);
		split_path_file(&path, &file, unArchivo->path);
		char* ubicacionCarpeta = string_from_format("metadata/archivos/%i/",unArchivo->indiceDirectorioPadre);
		crear_subcarpeta("metadata/archivos");
		crear_subcarpeta(ubicacionCarpeta);
		char* ubicacionArchivo = string_from_format("%s%s",ubicacionCarpeta, unArchivo->nombre);
		FILE * fp = fopen(ubicacionArchivo, "w");
		if(fp){
			fprintf(fp, "%s\n", unArchivo->path);
			fprintf(fp, "TAMANIO=%lu\n", unArchivo->tamanio);
			fprintf(fp, "TIPO=%i\n", unArchivo->tipoArchivo);
			int cantBloques = list_size(unArchivo->bloques);
			int i = 0;
			for (; i < cantBloques; i++) {
				t_bloque* unBloque = list_get(unArchivo->bloques, i);
				ubicacionBloque* ubicBloque1 = unBloque->copia1;
				ubicacionBloque* ubicBloque2 = unBloque->copia2;
				if(unBloque->copia1 != NULL){
				fprintf(fp, "BLOQUE%iCOPIA0=[%s,%i]\n", i, ubicBloque1->nroNodo,
						ubicBloque1->nroBloque);
				}
				if(unBloque->copia2 != NULL){
				fprintf(fp, "BLOQUE%iCOPIA1=[%s,%i]\n", i, ubicBloque2->nroNodo,
						ubicBloque2->nroBloque);
				}
				fprintf(fp, "BLOQUE%iBYTES=%lu\n", i, unBloque->finBloque);
			}
			fclose(fp);
		}
		free(path);
		free(file);
	}

	void info_archivo(char* path) {
		path = str_replace(path, "yamafs://", "");
		bool buscarArchivoPorPath(void* elem) {
			return string_equals_ignore_case(((t_archivo*) elem)->path, path);
		};

		t_archivo* archivoEncontrado = list_find(fileSystem.listaArchivos,
				buscarArchivoPorPath);
		if (archivoEncontrado != NULL) {
			char* tipoArchivo;
			if (archivoEncontrado->tipoArchivo == TEXTO)
				tipoArchivo = "TEXTO";
			else
				tipoArchivo = "BINARIO";

			printf("Nombre Archivo: %s \n Tamaño archivo: %lu \n Tipo Archivo: %s \n",
					archivoEncontrado->nombre, archivoEncontrado->tamanio,
					tipoArchivo);
			//imprimir nombre, tamanio y tipo de archivo

			void imprimirInfoBloque(void* elem) {
				t_bloque* bloque = (t_bloque*) elem;

				printf("Bloque numero %i  ---- Fin Bloque %lu\n",
						bloque->nroBloque, bloque->finBloque);
				//imprimir numero bloque y fin bloque

				if (bloque->copia1 != NULL) {
					//imprimir numero bloque y nodo
					printf("       Copia 1- Nodo %s  - Numero Bloque %i \n",
							bloque->copia1->nroNodo, bloque->copia1->nroBloque);
				} else {
					printf("       Copia 1 - NO EXISTE!\n");
				}

				if (bloque->copia2 != NULL) {
					//imprimir numero bloque y nodo
					printf("       Copia 2- Nodo %s  - Numero Bloque %i \n",
							bloque->copia2->nroNodo, bloque->copia2->nroBloque);
				} else {
					printf("       Copia 2 - NO EXISTE!\n");
				}

			}

			list_iterate(archivoEncontrado->bloques, imprimirInfoBloque);
		} else {
			//imprimir por pantalla que no existe el archivo

			printf("El archivo %s no existe\n", path);
		}
	}

	void Mover_Archivo(char* path_destino, t_archivo* archivoEncontrado) {
		//buscar entrada de directorio de destino
		path_destino = str_replace(path_destino, "yamafs://", "");

		char* path = malloc(255);
		char* file = malloc(255);

		split_path_file(&path, &file, path_destino);
		int cantidadDirectorios = countOccurrences(path_destino, "/");
		char** directorios = string_split(path_destino, "/");
		int i = 0;
		int indicePadre = 0;
		t_directory* directorioActual = NULL;
		for (; i < cantidadDirectorios - 1; i++) {
			if (directorios[i] == NULL)
				break;
			directorioActual = buscarDirectorio(indicePadre, directorios[i]);
			if (directorioActual == NULL) {
				printf("El directorio %s no existe para el padre %i\n",
						directorios[i], indicePadre);

			}
			indicePadre = directorioActual->index;
		}

		archivoEncontrado->nombre = string_duplicate(file);
		archivoEncontrado->path = string_duplicate(path_destino);
		archivoEncontrado->indiceDirectorioPadre = indicePadre;
		//fijarse si cambia el nombre, si cambia actualizar el t_archivo path, y nombre, indiceDirectorioPadre
		free(path);
		free(file);
	}

	void yama_rename(char* path_origen, char* nuevo_nombre, char* modo) {
		path_origen = str_replace(path_origen, "yamafs://", "");

		if(string_equals_ignore_case(modo, "a")){
			bool buscarArchivoPorPath(void* elem) {
				return string_equals_ignore_case(((t_archivo*) elem)->path, path_origen);
			}
			t_archivo* archivoEncontrado = list_find(fileSystem.listaArchivos, buscarArchivoPorPath);
			if (archivoEncontrado != NULL) {
				archivoEncontrado->nombre=string_duplicate(nuevo_nombre);
				char* path = malloc(255);
				char* file = malloc(255);

				split_path_file(&path, &file, path_origen);
				string_append(&path, nuevo_nombre);
				free(archivoEncontrado->path);
				archivoEncontrado->path=path;
			}else{
				printf("El archivo no existe\n");
			}
		}else if(string_equals_ignore_case(modo, "d")){
			if (!string_ends_with(path_origen, "/")) {
				string_append(&path_origen, "/");
			}
			int cantidadDirectorios = countOccurrences(path_origen, "/");
			char** directorios = string_split(path_origen, "/");
			int i = 0;
			int indicePadre = 0;
			t_directory* directorioActual = NULL;
			for (; i < cantidadDirectorios; i++) {
				if (directorios[i] == NULL)
					break;
				directorioActual = buscarDirectorio(indicePadre, directorios[i]);
				if (directorioActual == NULL) {
					printf("El directorio %s no existe para el padre %i\n",
							directorios[i], indicePadre);
					return;
				}
				indicePadre = directorioActual->index;
			}
			directorioActual->nombre =string_duplicate(nuevo_nombre);
		}else{
			printf("El modo debe ser 'a' o 'd' \n");
		}
	}

	void yama_mv(char* path_origen, char* path_destino, char tipo) {

		if (tipo == 'd') {

			//buscar entrada de directorio de destino
			path_origen = str_replace(path_origen, "yamafs://", "");
			if (!string_ends_with(path_origen, "/"))
				string_append(&path_origen, "/");

			int cantidadDirectorios = countOccurrences(path_origen, "/");
			char** directorios = string_split(path_origen, "/");
			int i = 0;
			int indicePadre = 0;
			t_directory* directorioOrigen = NULL;
			for (; i < cantidadDirectorios; i++) {
				if (directorios[i] == NULL)
					break;
				directorioOrigen = buscarDirectorio(indicePadre,
						directorios[i]);
				if (directorioOrigen == NULL) {
					printf("El directorio %s no existe para el padre %i\n",
							directorios[i], indicePadre);

				}
				indicePadre = directorioOrigen->index;
			}

			path_destino = str_replace(path_destino, "yamafs://", "");
			if (!string_ends_with(path_destino, "/"))
				string_append(&path_destino, "/");

			cantidadDirectorios = countOccurrences(path_destino, "/");
			directorios = string_split(path_destino, "/");
			i = 0;
			indicePadre = 0;
			t_directory* directorioDestino = NULL;
			for (; i < cantidadDirectorios; i++) {
				if (directorios[i] == NULL)
					break;
				directorioDestino = buscarDirectorio(indicePadre,
						directorios[i]);
				if (directorioDestino == NULL) {
					printf("El directorio %s no existe para el padre %i\n",
							directorios[i], indicePadre);

				}
				indicePadre = directorioDestino->index;
			}

			directorioOrigen->nombre = directorios[cantidadDirectorios - 1];
			directorioOrigen->padre = directorioDestino->index;
		} else {
			//buscar archivo origen tabla de archivos
			path_origen = str_replace(path_origen, "yamafs://", "");
			bool buscarArchivoPorPath(void* elem) {
				return string_equals_ignore_case(((t_archivo*) elem)->path,
						path_origen);
			}
			;

			t_archivo* archivoEncontrado = list_find(fileSystem.listaArchivos,
					buscarArchivoPorPath);
			if (archivoEncontrado != NULL) {
				Mover_Archivo(path_destino, archivoEncontrado);

			} else {
				printf("El archivo %s no existe\n", path_origen);
			}
		}
	}

	bool elArchivoPuedeSercargado(int cantidadBloques){
		int libreTotal=0;
		void bloquesLibres(t_nodo* unNodo){
			libreTotal += unNodo->libre/2;
		}
		list_iterate(fileSystem.ListaNodos,(void*)bloquesLibres);
		return libreTotal>cantidadBloques;
	}

