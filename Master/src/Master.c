/*
 ============================================================================
 Name        : Master.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "Master.h"

int main(void) {
	t_log* logger;
	char* fileLog;
	fileLog = "MasterLogs.txt";

	printf("Inicializando proceso Master\n");
	logger = log_create(fileLog, "Master Logs", 0, 0);
	log_trace(logger, "Inicializando proceso Master");

	master_configuracion configuracion = get_configuracion();
	log_trace(logger, "Archivo de configuracion levantado");

	//Se conecta al YAMA
	un_socket yamaSocket = conectar_a(configuracion.IP_YAMA,configuracion.PUERTO_YAMA);
	realizar_handshake(yamaSocket, cop_handshake_master);
	if(true){//comprobar_archivo(configuracion.PATH_ARCHIVO)){
		enviar_archivo(yamaSocket, configuracion.PATH_ARCHIVO);
		//desarrollo una vez que se envia el archivo ejecutable ..
	}else{
		log_trace(logger, "El archivo no existe o no puede ser leido");
	}
	return EXIT_SUCCESS;
}
