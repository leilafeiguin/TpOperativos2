/*
 ============================================================================
 Name        : Yama.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "Yama.h"

int main(void) {
	t_log* logger;
	char* fileLog;
	fileLog = "YamaLogs.txt";

	printf("Inicializando proceso Yama\n");
	logger = log_create(fileLog, "Yama Logs", 0, 0);
	log_trace(logger, "Inicializando proceso Yama");

	yama_configuracion configuracion = get_configuracion();
	log_trace(logger, "Archivo de configuracion levantado");

	return EXIT_SUCCESS;
}
