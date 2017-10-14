#include <socketConfig.h>
#include <commons/log.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdbool.h>
#include <commons/bitarray.h>
#include <commons/collections/list.h>

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

typedef struct t_bloque{
	int nroBloque;
	unsigned long int tamanioBloque;
	ubicacionBloque copia1;
	ubicacionBloque copia2;
	unsigned long int finBloque;
}t_bloque;

typedef struct t_archivo{
	char* nombre;
	char* path;
	unsigned long int tamanio;
	bool estado; //Disponible o no - true o false
	t_list bloques; // se le deben agregar struct t_bloque
}t_archivo;

typedef struct t_nodo{
	char* nroNodo;
	un_socket socket;
	bool ocupado;
	t_bitarray* bitmap;
	char* ip;
	int puertoWorker;
}t_nodo;



typedef struct t_fs{
	t_list* ListaNodos; //se le deben agregar struct t_nodo
	int tamanio;
	int libre;
}t_fs;

static t_nodo *nodo_create(int nroNodo, bool ocupado, t_bitarray* bitmap, un_socket socket){
	t_nodo *new = malloc(sizeof(t_nodo));
	new->bitmap = bitmap;
	new->nroNodo = nroNodo;
	new->ocupado = ocupado;
	new->socket = socket;
	return new;
}

static void nodo_destroy(t_nodo* nodo){
	free(nodo->bitmap);//checkear que sea asi
	free(nodo->nroNodo);
	//free(nodo->ocupado);
}

//inicializacion de estructuras
t_directory tablaDeDirectorios[99];

t_fs fileSystem;
t_list* nodos;

void hiloFileSystem_Consola();


t_nodo* buscar_nodo_libre (int nodoAnterior);

int buscarBloque (t_nodo*);

void enviar_bloque_a_escribir (int numBloque, void* contenido, t_nodo*);

void escribir_bloque (void* bloque);
