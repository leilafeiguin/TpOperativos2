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



	int fd=open(configuracion.RUTA_DATABIN, O_RDWR);
	fstat(fd, &sb);


	void* archivo=mmap(NULL,sb.st_size,PROT_READ | PROT_WRITE,  MAP_SHARED,fd,0);
	//mmap

	//CONEXIONES
	un_socket fileSystemSocket = conectar_a(configuracion.IP_FILESYSTEM,configuracion.PUERTO_FILESYSTEM);
	realizar_handshake(fileSystemSocket, cop_handshake_datanode);

	while(1){
		t_paquete* paquete=recibir(fileSystemSocket);

		switch (paquete-> codigo_operacion){
			//case cop_datanode_get_bloque:
			{
				//int numeroBloque = ((t_getbloque*)paquete->data)->numero_bloque;
				//void* bloque = leer_bloque(archivo, numeroBloque);//

				//enviar(fileSystemSocket, cop_datanode_get_bloque_respuesta, 1024*1024 /*1MB*/, bloque);//
				//free(bloque);//
				//free(paquete);//
			}//
			break;
			case cop_datanode_setbloque:
						{

						}
						break;






		}


	}

	return EXIT_SUCCESS;
}


