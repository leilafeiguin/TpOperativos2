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

int main(char* scriptTransf, char* scriptReduc, char* archivoOrigen, char* archivoDestino) {
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
	//Master avisa a yama sobre que archivo del FS necesita ejecutar

	enviar(yamaSocket,cop_master_archivo_a_transaformar,sizeof(char*)*strlen(archivoOrigen),archivoOrigen);



	log_trace(logger, "Recibi datos de workers de Yama");
	t_paquete* paqueteRecibido = recibir(yamaSocket);

	//	NO BORRAR SIRVE PARA PROBAR CONEXIONES AL WORKER
	//	un_socket w1 = conectar_a("127.0.0.1","5050");
	//
	//	un_socket w2 = conectar_a("127.0.0.1","5050");
	//
	//	un_socket w3 = conectar_a("127.0.0.1","5050");
	//
	//	realizar_handshake(w1, cop_handshake_master);
	//	realizar_handshake(w2, cop_handshake_master);
	//	realizar_handshake(w3, cop_handshake_master);

	//list_map(workers, );
	//agregandolos a las estructuras correspondientes

	//realizar_handshake(worker1, cop_handshake_master);

	//si falla hay que capturarlo y mandarlo a YAMA
	log_trace(logger, "Respondo a Yama estado de conexiones con workers");
	char* estado_worker1 = "Ok";
	enviar(yamaSocket, cop_master_estados_workers, sizeof(char*)*strlen(estado_worker1), estado_worker1);

	//hacer un if para saber si pasa a etapa de transformacion o si hubo error esperar nuevos workers



	return EXIT_SUCCESS;
}
