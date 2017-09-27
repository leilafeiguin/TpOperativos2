#include <socketConfig.h>
#include <commons/log.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdbool.h>

//ESTRUCTURA ARCHIVO CONFIGURACION
typedef struct fileSystem_configuracion {
	char* IP_FS;
	char* PUERTO_FS;
} fileSystem_configuracion;

const char* path = "/home/utnso/Desktop/tp-2017-2c-Todo-ATR/FileSystem/configFileSystem.cfg";
char* estado = "Estable";
bool existeYama = false;

fileSystem_configuracion get_configuracion() {
	printf("Levantando archivo de configuracion del proceso FileSystem\n");
	fileSystem_configuracion configuracion;
	// Obtiene el archivo de configuracion
	t_config* archivo_configuracion = config_create(path);
	configuracion.IP_FS = get_campo_config_string(archivo_configuracion, "IP_FS");
	configuracion.PUERTO_FS = get_campo_config_string(archivo_configuracion, "PUERTO_FS");
	return configuracion;
}

//Estructuras FileSystem

typedef struct t_directory {
  int index;
  char nombre[255];
  int padre;
}t_directory;

typedef struct ubicacionBloque{
	int nroNodo;
	int nroBloque; //en nodo
}ubicacionBloque;

typedef struct t_bloques{
	int nroBloque;
	unsigned long int tamanioBloque;
	ubicacionBloque copia1;
	ubicacionBloque copia2;
	unsigned long int finBloque;
	struct t_bloques* siguiente;
}t_bloques;

typedef struct t_archivo{
	char* nombre;
	char* path;
	unsigned long int tamanio;
	bool estado; //Disponible o no - true o false
	t_bloques bloques;
}t_archivo;

typedef struct t_bitmap{
	int estado;
	struct t_bitmap* siguiente;
}t_bitmap;

typedef struct t_nodo{
	int nroNodo;
	int libre;
	int ocupado;
	t_bitmap bitmap;
	struct t_nodo* nodo;
}t_nodo;

typedef struct t_nodos{
	t_nodo nodo;
	int tamanio;
	int libre;
}t_nodos;
t_nodos nodos;

//inicializacion de estructuras
t_directory tablaDeDirectorios[99];


void hiloFileSystem_Consola();
t_bitmap leerBitmap(char*);


