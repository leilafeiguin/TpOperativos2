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

	enviar(yamaSocket,cop_master_archivo_a_transformar,sizeof(char*)*strlen(archivoOrigen),archivoOrigen);

	log_trace(logger, "Recibi datos de workers de Yama");
	t_paquete* paqueteRecibido = recibir(yamaSocket);
	//preguntar a seba tema malloc y memoria
	char* buffer = malloc(paqueteRecibido->tamanio);

	memcpy(buffer, paqueteRecibido->data, paqueteRecibido->tamanio);

	int cantidadConexionesPendientes = cantidadCaracterEnString(buffer, ',') +1;
	char** conexionesSplit = str_split(buffer, ',');
	int i;
	for(i = 0; i < cantidadConexionesPendientes; i++) {
		char** auxiliar = str_split(conexionesSplit[i], '|');
		char* ip_aux = auxiliar[0];
		char* puerto_aux = auxiliar[1];

		un_socket unSocket = conectar_a(ip_aux, puerto_aux);
		bool estado = realizar_handshake(unSocket, cop_handshake_master);
		t_worker* unWorker = malloc(sizeof(t_worker));
		unWorker->IP = malloc(strlen(ip_aux)+1);
		strcpy(unWorker->IP,ip_aux);
		unWorker->puerto = malloc(strlen(puerto_aux)+1);
		strcpy(unWorker->puerto , puerto_aux);
		unWorker->estado = estado;
		unWorker->socket = unSocket;

		list_add(workers, unWorker);
		free(auxiliar);
	}

	free(buffer);
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

size_t cantidadCaracterEnString(const char *str, char token)
{
  size_t count = 0;
  while(*str != '\0')
  {
    count += *str++ == token;
  }
  return count;
}

char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}
