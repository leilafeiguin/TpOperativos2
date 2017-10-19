/*
 * socketConfig.h
 *
 *  Created on: 6/5/2017
 *      Author: utnso
 */

#ifndef SOCKETCONFIG_H_
#define SOCKETCONFIG_H_

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <commons/string.h>
#include <commons/error.h>
#include <commons/config.h>
#include <unistd.h>






//0 - 9 Handhsake
//10 - 29 Master
//30- 49 YAMA
//50 - 69 FS
//70 - 79 DataNode
//80 - 99 Worker

enum codigos_de_operacion {
	cop_generico = 0,
	cop_archivo_programa = 1,
	//cop_handshake_quienLoRealiza
	cop_handshake_datanode = 2,
	cop_handshake_yama = 3,
	cop_handshake_master = 4,
	cop_handshake_worker = 5,

	cop_master_archivo_a_transformar = 10,
	cop_master_estados_workers = 11,

	cop_yama_lista_de_workers = 50,
	cop_yama_info_fs = 51,

	cop_datanode_get_bloque = 70,
	cop_datanode_get_bloque_respuesta = 71,
	cop_datanode_setbloque = 72,
	cop_datanode_info = 73,

	cop_worker_transformacion = 80,
	cop_worker_reduccionLocal = 81,
	cop_worker_reduccionGlobal = 82
};
typedef struct {
	int numero_bloque;

}t_getbloque;

typedef struct {
	int numero_bloque;
	void *datos_bloque;
}t_setbloque;


typedef struct {
	int cant_script; //long script
	char* script;
	char* bloq;
	int cant_ocupada_bloque;
	int cant_archivo_temporal;//long nombre
	char* archivo_temporal; //nombre archivo
} t_transf;

typedef int un_socket;

typedef struct {
	int codigo_operacion;
	int tamanio;
	void * data;
} t_paquete;

typedef struct {
	char* ip;
	int puertoWorker;
	int tamanio;
	char* nombreNodo;
} t_paquete_datanode_info_list;

char* archivo;

/**	@NAME: conectar_a
 * 	@DESC: Intenta conectarse.
 * 	@RETURN: Devuelve el socket o te avisa si hubo un error al conectarse.
 *
 */



un_socket conectar_a(char *IP, char* Port);

/**	@NAME: socket_escucha
 * 	@DESC: Configura un socket que solo le falta hacer listen.
 * 	@RETURN: Devuelve el socket listo para escuchar o te avisa si hubo un error al conectarse.
 *
 */

un_socket socket_escucha(char* IP, char* Port);

/**	@NAME: enviar
 * 	@DESC: Hace el envio de la data que le pasamos. No hay que hacer más nada.
 *
 */

void enviar(un_socket socket_envio, int codigo_operacion, int tamanio,
		void * data);

/**	@NAME: recibir
 * 	@DESC: devuelve un paquete que está en el socket pedido
 *
 */
t_paquete* recibir(un_socket socket_para_recibir);

/**	@NAME: aceptar_conexion
 * 	@DESC: devuelve el socket nuevo que se quería conectar
 *
 */
un_socket aceptar_conexion(un_socket socket_escuchador);

/**	@NAME: liberar_paquete
 * 	@DESC: libera el paquete y su data.
 *
 */
void liberar_paquete(t_paquete * paquete);

/**	@NAME: realizar_handshake
 *
 */
bool realizar_handshake(un_socket socket_del_servidor, int cop_handshake);

/**	@NAME: esperar_handshake
 *
 */
bool esperar_handshake(un_socket socket_del_cliente,
		t_paquete* inicio_del_handshake, int cop_handshake);

char get_campo_config_char(t_config* archivo_configuracion, char* nombre_campo);

int get_campo_config_int(t_config* archivo_configuracion, char* nombre_campo);

char** get_campo_config_array(t_config* archivo_configuracion,
		char* nombre_campo);

char* get_campo_config_string(t_config* archivo_configuracion,
		char* nombre_campo);

/**	@NAME: enviar_archivo
 *
 */
void enviar_archivo(un_socket socket, char* path);

bool comprobar_archivo(char* path);

/** @NAME: leer_bloque
 *
 */
char* obtener_mi_ip();






#endif /* SOCKETCONFIG_H_ */
