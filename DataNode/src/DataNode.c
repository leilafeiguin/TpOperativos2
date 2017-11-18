/*
 ============================================================================
 Name        : DataNode.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "DataNode.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(void) {
	t_log* logger;
	char* fileLog;
	fileLog = "DataNodeLogs.txt";

	printf("Inicializando proceso DataNode\n");
	logger = log_create(fileLog, "DataNode Logs", 0, 0);
	log_trace(logger, "Inicializando proceso DataNode");

	dataNode_configuracion configuracion = get_configuracion();
	log_trace(logger, "Archivo de configuracion levantado");


	//MMAP
	struct stat sb;
	if(access(configuracion.RUTA_DATABIN, F_OK) == -1) {
		FILE* fd=fopen(configuracion.RUTA_DATABIN, "a+"); //dudoso el r+
		fseek(fd, 20971520, SEEK_SET);
		fputc('\0',fd);
		fclose(fd);

		}

	int fd=open(configuracion.RUTA_DATABIN, O_RDWR);
	fstat(fd, &sb);
	char* archivo= mmap(NULL,sb.st_size,PROT_READ | PROT_WRITE,  MAP_SHARED,fd,0);

	//CONEXIONES
	un_socket fileSystemSocket = conectar_a(configuracion.IP_FILESYSTEM,configuracion.PUERTO_FILESYSTEM);
	realizar_handshake(fileSystemSocket, cop_handshake_datanode);
	t_paquete_datanode_info_list* paquete = malloc(sizeof(t_paquete_datanode_info_list));
	char*ip=obtener_mi_ip();
	paquete->ip= malloc(strlen(ip)+1);
	strcpy(paquete->ip,ip);
	paquete->puertoWorker = atoi(configuracion.PUERTO_WORKER);
	paquete->tamanio = sb.st_size;
	paquete->nombreNodo = malloc(strlen(configuracion.NOMBRE_NODO)+1);
	strcpy(paquete->nombreNodo, configuracion.NOMBRE_NODO);

	int longitudIp=strlen(paquete->ip)+1;
	int longitudNombre = strlen(paquete->nombreNodo )+1;
	int desplazamiento=0;

	void* buffer = malloc( longitudIp+ sizeof(int) + sizeof(int)+sizeof(int) + sizeof(int) + longitudNombre);
	memcpy(buffer, &longitudIp, sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(buffer+desplazamiento, paquete->ip, longitudIp);
	desplazamiento+= longitudIp;
	memcpy(buffer+ desplazamiento, &paquete->puertoWorker, sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(buffer+ desplazamiento, &paquete->tamanio, sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(buffer+ desplazamiento, &longitudNombre, sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(buffer+ desplazamiento, paquete->nombreNodo,longitudNombre);
	desplazamiento+= longitudNombre;


	enviar(fileSystemSocket, cop_datanode_info,desplazamiento, buffer);

	//todo mati e, aca hacer enviar
	while(1){
		t_paquete* paquete=recibir(fileSystemSocket);
		switch (paquete-> codigo_operacion){
			case cop_datanode_get_bloque:
			{
				int numeroBloque;
				memcpy(&numeroBloque, paquete->data, sizeof(int));
				void* bloqueAenviar = malloc(1024*1024);
				leer_bloque(numeroBloque, bloqueAenviar);
				enviar(fileSystemSocket, cop_datanode_get_bloque_respuesta, 1024*1024, bloqueAenviar);
				free(bloqueAenviar);
				free(paquete);
			}
			break;
			case cop_datanode_setbloque:
			{
				int numeroBloque;
				int desplazamiento=0;
				memcpy(&numeroBloque, paquete->data, sizeof(int));
				desplazamiento += sizeof(int);
				void* bloqueArecibir =malloc(1024*1024);
				memcpy(bloqueArecibir, paquete->data + desplazamiento, 1024*1024);
				escribir_bloque (numeroBloque, bloqueArecibir);
				free(bloqueArecibir);
				free(paquete);
			}
			break;
			case -1:
			{
				printf("Se cayo FS, finaliza DataNode.");
				exit(-1);
			}
			break;
		}
	}
	return EXIT_SUCCESS;
}


