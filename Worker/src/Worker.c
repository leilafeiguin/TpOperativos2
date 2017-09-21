/*
 ============================================================================
 Name        : Worker.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "Worker.h"
#include "socketConfig.h"

int main(void) {
	t_log* logger;
	char* fileLog;
	fileLog = "WorkerLogs.txt";

	printf("Inicializando proceso Worker\n");
	logger = log_create(fileLog, "Worker Logs", 0, 0);
	log_trace(logger, "Inicializando proceso Worker");

	worker_configuracion configuracion = get_configuracion();
	log_trace(logger, "Archivo de configuracion levantado");

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
											case cop_worker_tranformacion:
													//procesar tr paquete_recibido->data
											break;
											case cop_worker_reduccionLocal:

											break;
											case cop_worker_reduccionGlobal:

											break;
							}

						}
							break;



		}
				}


	}


	return EXIT_SUCCESS;
}
/*
 *

 * */
