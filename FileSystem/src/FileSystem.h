#include <socketConfig.h>
#include <commons/log.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdbool.h>
#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef enum {

	TEXTO=0,
	BINARIO=1
}t_tipo_archivo;
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
  char* nombre;
  int padre;
}t_directory;

typedef struct ubicacionBloque{
	char* nroNodo;
	int nroBloque; //en nodo
}ubicacionBloque;

typedef struct t_bloque{
	int nroBloque;
	unsigned long int tamanioBloque; //todo marco revisa si hace falta este campo
	ubicacionBloque* copia1;
	ubicacionBloque* copia2;
	unsigned long int finBloque;
}t_bloque;

typedef struct t_archivo{
	char* nombre;
	char* path;
	t_tipo_archivo tipoArchivo;
	unsigned long int tamanio;
	bool estado; //Disponible o no - true o false
	t_list* bloques; // se le deben agregar struct t_bloque
}t_archivo;

typedef struct t_nodo{
	char* nroNodo;
	un_socket socket;
	bool ocupado;
	t_bitarray* bitmap;
	char* ip;
	int puertoWorker;
	int tamanio;
	int libre;
}t_nodo;

typedef struct t_nodoasignado{
	char* nodo1;
	char* nodo2;
	int bloque1;
	int bloque2;
}t_nodoasignado;

typedef struct t_fs{
	t_list* ListaNodos; //se le deben agregar struct t_nodo
	t_list* listaArchivos; //se le deben agregar struct t_archivo
	int tamanio;
	int libre;
}t_fs;

typedef struct t_bloque_particion {
	char* contenido;
	int ultimoByteValido;
}t_bloque_particion;

typedef struct t_archivo_partido {
	int cantidadBloques;//en cuantos bloques se parte
	t_list* bloquesPartidos;
}t_archivo_partido;

static t_nodo *nodo_create(int nroNodo, bool ocupado, t_bitarray* bitmap, un_socket socket, char* ipWorker, int puertoWorker, int tamanio){
	t_nodo *new = malloc(sizeof(t_nodo));
	new->bitmap = bitmap;
	new->nroNodo = nroNodo;
	new->ocupado = ocupado;
	new->socket = socket;
	new->ip = malloc(strlen(ipWorker)+1);
	strcpy(new->ip, ipWorker);
	new->puertoWorker = puertoWorker;
	new->tamanio = tamanio;
	return new;
}

static void nodo_destroy(t_nodo* nodo){
	free(nodo->bitmap);//checkear que sea asi
	free(nodo->nroNodo);
	//free(nodo->ocupado);
}

//inicializacion de estructuras
t_directory* tablaDeDirectorios[99];

t_fs fileSystem;

void hiloFileSystem_Consola();
t_bitarray leerBitmap(char*);

void CP_FROM(char* origen, char* destino, char tipoArchivo);
t_archivo_partido* LeerArchivo(char* archivo, char tipoArchivo);
void CP_TO (char* origen, char* destino);


t_nodo* buscar_nodo_libre (int nodoAnterior);
t_nodo* buscar_nodo (char* nombreNodo);
int buscarBloque (t_nodo*);
void enviar_bloque_a_escribir (int numBloque, void* contenido, t_nodo*);
void* getbloque (int, t_nodo*);
t_nodoasignado* escribir_bloque (void* bloque);
void split_path_file(char** p, char** f, char *pf);
void actualizarArchivoDeDirectorios();
char *str_replace(char *orig, char *rep, char *with);
int countOccurrences(char * str, char * toSearch);
t_directory* crearDirectorio(int padre, char* nombre);
t_directory* buscarDirectorio(int padre, char* nombre);
void crear_subcarpeta(char* nombre);
void actualizarBitmap(t_nodo unNodo);
void actualizarArchivoTablaNodos();
void cat (char*);
void imprimirPorPantallaContenidoBloque(void* elem);
bool buscarArchivoPorPath(void* elem);
void cp_block(char* path, int numeroBloque, char* nombreNodo);
void calcular_md5(char* path);
void ls(char*path);
