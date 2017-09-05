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
	t_log* logger;
	char* fileLog;
	fileLog = "FileSystemLogs.txt";

	printf("Inicializando proceso FileSystem\n");
	logger = log_create(fileLog, "FileSystem Logs", 0, 0);
	log_trace(logger, "Inicializando proceso FileSystem");

	fileSystem_configuracion configuracion = get_configuracion();
	log_trace(logger, "Archivo de configuracion levantado");

	//CONEXIONES
	un_socket dataNodeSocket = socket_escucha(configuracion.IP_DATANODE, configuracion.PUERTO_DATANODE);
	aceptar_conexion(dataNodeSocket);
	t_paquete* handshake = recibir(dataNodeSocket);
	esperar_handshake(dataNodeSocket, handshake,cop_handshake_fileSystem);
	free(handshake);

	return EXIT_SUCCESS;
}
