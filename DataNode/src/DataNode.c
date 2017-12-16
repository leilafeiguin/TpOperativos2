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

void* archivo;
t_log* logger;
int main(void) {
	imprimir("/home/utnso/Desktop/tp-2017-2c-Todo-ATR/Libraries/datanode.txt");

	char* fileLog;
	fileLog = "DataNodeLogs.txt";

	logger = log_create(fileLog, "DataNode Logs", 1, 1);
	log_trace(logger, "Inicializando proceso DataNode. \n");

	dataNode_configuracion configuracion = get_configuracion();
	log_trace(logger, "Archivo de configuracion levantado. \n");

	//MMAP
	struct stat sb;
	if(access(configuracion.RUTA_DATABIN, F_OK) == -1) {
		FILE* fd=fopen(configuracion.RUTA_DATABIN, "a+");
		ftruncate(fileno(fd), configuracion.CANTIDAD_MB_DATABIN*1024*1024);
		fclose(fd);
	}

	int fd=open(configuracion.RUTA_DATABIN, O_RDWR);
	fstat(fd, &sb);
	archivo= mmap(NULL,sb.st_size,PROT_READ | PROT_WRITE,  MAP_SHARED,fd,0);

	//CONEXIONES
	un_socket fileSystemSocket = conectar_a(configuracion.IP_FILESYSTEM,configuracion.PUERTO_FILESYSTEM);
	realizar_handshake(fileSystemSocket, cop_handshake_datanode);
	t_paquete_datanode_info_list* paquete = malloc(sizeof(t_paquete_datanode_info_list));
	char*ip=configuracion.IP_NODO;
	int puerto=atoi(configuracion.PUERTO_DATANODE);
	paquete->ip= malloc(strlen(ip)+1);
	strcpy(paquete->ip,ip);
	paquete->puertoDataNode=puerto;
	paquete->puertoWorker = atoi(configuracion.PUERTO_WORKER);
	paquete->tamanio = sb.st_size;

	paquete->nombreNodo = malloc(strlen( configuracion.NOMBRE_NODO )+1);
	strcpy(paquete->nombreNodo, configuracion.NOMBRE_NODO);

	int longitudIp=strlen(paquete->ip)+1;
	int longitudNombre = strlen(paquete->nombreNodo )+1;
	int desplazamiento=0;

	void* buffer = malloc( longitudIp+ sizeof(int) + sizeof(int)+sizeof(int) + sizeof(int) + longitudNombre + sizeof(int));
	memcpy(buffer, &longitudIp, sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(buffer+desplazamiento, paquete->ip, longitudIp);
	desplazamiento+= longitudIp;
	memcpy(buffer+desplazamiento, &paquete->puertoDataNode, sizeof(int));
	desplazamiento+=sizeof(int);
	memcpy(buffer+ desplazamiento, &paquete->puertoWorker, sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(buffer+ desplazamiento, &paquete->tamanio, sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(buffer+ desplazamiento, &longitudNombre, sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(buffer+ desplazamiento, paquete->nombreNodo,longitudNombre);
	desplazamiento+= longitudNombre;

	enviar(fileSystemSocket, cop_datanode_info,desplazamiento, buffer);

	while(1){
		t_paquete* paquete=recibir(fileSystemSocket);
		switch (paquete-> codigo_operacion){
			case cop_datanode_get_bloque:
			{
				int numeroBloque;
				memcpy(&numeroBloque, paquete->data, sizeof(int));
				void* bloqueAenviar = malloc(1024*1024);
				leer_bloque_datanode(numeroBloque, bloqueAenviar);
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
				memset(bloqueArecibir, '\0', 1024*1024);
				memcpy(bloqueArecibir, paquete->data + desplazamiento, 1024*1024);
				escribir_bloque_datanode (numeroBloque, bloqueArecibir);
				free(bloqueArecibir);
				free(paquete);
			}
			break;
			case -1:
			{
				log_trace(logger, "Se cayo FS, finaliza DataNode.\n");
				exit(-1);
			}
			break;
		}
	}
	return EXIT_SUCCESS;
}

void leer_bloque_datanode(int numeroBloque, void* bloqueAleer) {
	int posicion = (numeroBloque *1024*1024);
	memcpy (bloqueAleer, archivo + posicion, 1024*1024);
	return;
}

void escribir_bloque_datanode(int numeroBloque, void* bloqueAescribir) {
	int posicion= (numeroBloque *1024*1024);
	memcpy (archivo+ posicion,bloqueAescribir,1024*1024);
	return;
}

