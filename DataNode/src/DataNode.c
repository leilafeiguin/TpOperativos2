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

int main(void) {
	t_log* logger;
	char* fileLog;
	fileLog = "DataNodeLogs.txt";

	printf("Inicializando proceso DataNode\n");
	logger = log_create(fileLog, "DataNode Logs", 0, 0);
	log_trace(logger, "Inicializando proceso DataNode");

	dataNode_configuracion configuracion = get_configuracion();
	log_trace(logger, "Archivo de configuracion levantado");

	return EXIT_SUCCESS;
}
