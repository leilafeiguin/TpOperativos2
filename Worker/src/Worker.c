#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Worker.h"
#include "socketConfig.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>


#define MAX_LINE 4096

//---------------------------------FUNCIONES---------------------------------------------
unsigned long int lineCountFile(const char *filename){
    FILE *fp = fopen(filename, "r");
    unsigned long int linecount = 0;
    int c;
    if(fp == NULL){
    	printf("No se puede abrir el archivo.\n");
        fclose(fp);
        return 0;
    }
    while((c=fgetc(fp)) != EOF ){
        if(c == '\n')
            linecount++;
    }
    fclose(fp);
    return linecount;
}

void sortfile(char **array, int linecount){
    int i, j;
    char t[MAX_LINE];
    for(i=1;i<linecount;i++){
        for(j=1;j<linecount;j++){
            if(strcmp(array[j-1], array[j]) > 0){
                strcpy(t, array[j-1]);
                strcpy(array[j-1], array[j]);
                strcpy(array[j], t);
            }
        }
    }
}


bool apareo (char* paths [], char* nombre_ordenado){
	int cantPaths = sizeof(paths) / sizeof(paths[0]) + 1;
	int i = 0;
	unsigned long int linecountGlobal;
	for(i=0;i<cantPaths;i++){
		linecountGlobal = linecountGlobal + lineCountFile(paths[i]) + 1;
	}
	char **arrayGlobal = (char**)malloc(linecountGlobal * sizeof(char*));
	for(i=0; i<cantPaths; i++){
		FILE *fileIN;
		fileIN = fopen(paths[i], "rb");
		if(!fileIN){
			printf("No se puede abrir el archivo.\n");
			return false;
		}
		unsigned long int linecount = lineCountFile(paths[i]);
		linecount += 1;
		char **array = (char**)malloc(linecount * sizeof(char*));
		char singleline[MAX_LINE];
		int i = 0;
		while(fgets(singleline, MAX_LINE, fileIN) != NULL){
			array[i] = (char*) malloc (MAX_LINE * sizeof(char));
			singleline[MAX_LINE] = '\0';
			strcpy(array[i], singleline);
			i++;
		}
		strcat(array[linecount - 1], "\n");
		if(arrayGlobal[0] == NULL){
			memcpy(arrayGlobal, array, linecount * sizeof(char*));
		}else{
			memcpy(arrayGlobal+linecount, array, linecount * sizeof(char*));
		}
		fclose(fileIN);
	}
	sortfile(arrayGlobal, linecountGlobal);
	FILE *archivoOrdenado = fopen(nombre_ordenado, "wb");
	if(!archivoOrdenado){
		printf("No se puede abrir el archivo.\n");
		return false;
	}
	for(i=0; i<linecountGlobal; i++){
		fprintf(archivoOrdenado,"%s", arrayGlobal[i]);
	}
	fclose(archivoOrdenado);
	for(i=0; i<linecountGlobal; i++){
		free(arrayGlobal[i]);
	}
	return true;
}

void* archivo;

int main(void) {
	t_log* logger;
	char* fileLog;
	fileLog = "WorkerLogs.txt";
	char* paths [2];
	paths[0] = "fileTest";
	paths[1] = "fileTest2";
	printf("Inicializando proceso Worker\n");
	logger = log_create(fileLog, "Worker Logs", 0, 0);
	log_trace(logger, "Inicializando proceso Worker");
	worker_configuracion configuracion = get_configuracion();
	log_trace(logger, "Archivo de configuracion levantado");

	struct stat sb;
	if(access(configuracion.RUTA_DATABIN, F_OK) == -1) {
		FILE* fd=fopen(configuracion.RUTA_DATABIN, "a+");
		ftruncate(fileno(fd), 20*1024*1024);
		fclose(fd);
	}

	//Se abre data bin en modo read
	int fd=open(configuracion.RUTA_DATABIN, O_RDONLY);
	fstat(fd, &sb);
	archivo= mmap(NULL,sb.st_size,PROT_READ,  MAP_SHARED,fd,0); //PROT_READ ??

	un_socket socketServer=socket_escucha("127.0.0.1", configuracion.PUERTO_WORKER);
	listen(socketServer, 999);
	while(1){
	un_socket socketConexion=aceptar_conexion(socketServer);
	if(socketConexion >0)
	{
		int pid=fork();
			switch(pid)
			{
				case -1: // Si pid es -1 quiere decir que es el proceso padre, ha habido un error
					perror("No se ha podido crear el proceso hijo\n");
					break;
				case 0: // Cuando pid es cero quiere decir que es el proceso hijo
				{
					t_paquete* paquete_recibido = recibir(socketConexion);
					esperar_handshake(socketConexion, paquete_recibido, cop_handshake_master);
					switch(paquete_recibido->codigo_operacion){ //revisar validaciones de habilitados
						case cop_handshake_master:
						break;
						case cop_handshake_worker:
						break;
						case cop_worker_transformacion:
						{
							int desplazamiento = 0;
							int cantidadElementos = 0;
							memcpy(&cantidadElementos, paquete_recibido->data, sizeof(int));
							desplazamiento += sizeof(int);
							int i;
							for(i=0;i<cantidadElementos;i++){
								t_transf* paquete_transformacion = malloc(sizeof(t_transf));
								//memcpy(origen, destino, cuantoQuieroCopiar)
								memcpy(&paquete_transformacion->cant_script, paquete_recibido->data, sizeof(int));
								desplazamiento += sizeof(int);
								paquete_transformacion->script = malloc(paquete_transformacion->cant_script);
								memcpy(paquete_transformacion->script, paquete_recibido->data + desplazamiento, paquete_transformacion->cant_script);
								desplazamiento += paquete_transformacion->cant_script;

								memcpy(&paquete_transformacion->bloq, paquete_recibido->data + desplazamiento, 1024*1024);
								desplazamiento += sizeof(int);
								paquete_transformacion->cant_archivo_temporal = 11;
								paquete_transformacion->archivo_temporal = malloc(paquete_transformacion->cant_archivo_temporal);
								memcpy(paquete_transformacion->archivo_temporal, paquete_recibido->data + desplazamiento, paquete_transformacion->cant_archivo_temporal);
								desplazamiento += paquete_transformacion->cant_archivo_temporal;
								memcpy(paquete_transformacion->cant_ocupada_bloque, paquete_recibido->data + desplazamiento, sizeof(int));


								FILE *archivoPaqueteTransformacion = fopen("./archivoPaqueteTransformacion", "wb");
								fprintf(archivoPaqueteTransformacion,"%s", paquete_transformacion->script);
								chmod("./archivoPaqueteTransformacion", 001); //permiso de ejecucion para ese path
								transformacion(paquete_transformacion->script, obtenerBloque(paquete_transformacion->bloq, paquete_transformacion->cant_ocupada_bloque), paquete_transformacion->archivo_temporal);

							//falta enviar a worker el estado trans

								char* mensaje = malloc(3);
								mensaje = "ok";
								enviar(socketConexion, cop_master_estado_transformacion , 3, mensaje);


							}
						}
						break;
						case cop_worker_reduccionLocal:
						{
							int cantElementos;
							int desplazamiento;
							int i = 0;

							memcpy(&cantElementos, paquete_recibido->data, sizeof(int));
							desplazamiento += sizeof(int);

							char* archivosAReducir[cantElementos];
							for(; i < sizeof(archivosAReducir);i++) {
								int tamanioElemento;
								memcpy(&tamanioElemento, paquete_recibido->data + desplazamiento, sizeof(int));
								desplazamiento += sizeof(int);

								archivosAReducir[i] = malloc(tamanioElemento + 1);

								memcpy(archivosAReducir[i], paquete_recibido->data + desplazamiento, tamanioElemento + 1); //tamanaio ele + 1 ??
								desplazamiento += tamanioElemento;
							}

							int tamanio_archivo_reducido;
							memcpy(&tamanio_archivo_reducido, paquete_recibido->data + desplazamiento, sizeof(int));
							desplazamiento += sizeof(int);

							char* archivo_reducido = malloc(tamanio_archivo_reducido);
							memcpy(archivo_reducido, paquete_recibido->data + desplazamiento, tamanio_archivo_reducido + 1);

							bool resultado = apareo(archivosAReducir, archivo_reducido);

							int longitudIp = strlen(configuracion.IP_NODO);
							char* buffer = malloc(longitudIp + sizeof(int) + sizeof(int) + sizeof(bool));
							desplazamiento = 0;

							memcpy(buffer, &longitudIp, sizeof(int));
							desplazamiento += sizeof(int);

							memcpy(buffer + desplazamiento, configuracion.IP_NODO, longitudIp + 1);
							desplazamiento += longitudIp;

							memcpy(buffer + desplazamiento, &configuracion.PUERTO_WORKER, sizeof(int));
							desplazamiento += sizeof(int);

							memcpy(buffer + desplazamiento, &resultado, sizeof(bool));

							enviar(socketConexion, cop_worker_reduccionLocal, strlen(buffer), buffer);
						}
						break;
						case cop_worker_reduccionGlobal:{

						}
						break;
						case -1:
						{
							printf("Se cayo Master, finaliza Worker.\n");
							exit(-1);
						}
						break;
					}
				}
			break;
		}
	}
}
return EXIT_SUCCESS;
}

char* obtenerBloque(int numeroBloque, int tamanioBloque){
	char* bloque= malloc(tamanioBloque);
	int posicion = (numeroBloque *1024*1024);
	memcpy (bloque, archivo + posicion, tamanioBloque);
	return bloque;
}


void transformacion(char* script, char* bloque, char* destino){
	char* func =string_from_format("echo %s | %s > %s ", bloque, script, destino);
	system(func);
}
