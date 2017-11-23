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
	TEXTO = 0, BINARIO = 1
} t_tipo_archivo;
//ESTRUCTURA ARCHIVO CONFIGURACION
typedef struct fileSystem_configuracion {
	char* IP_FS;
	char* PUERTO_FS;
} fileSystem_configuracion;
 char* path =
		"/home/utnso/Desktop/tp-2017-2c-Todo-ATR/FileSystem/configFileSystem.cfg";
char* estado = "Estable";
bool existeYama = false;

fileSystem_configuracion get_configuracion() {
	printf("Levantando archivo de configuracion del proceso FileSystem\n");
	fileSystem_configuracion configuracion;
	// Obtiene el archivo de configuracion
	t_config* archivo_configuracion = config_create(path);
	configuracion.IP_FS = get_campo_config_string(archivo_configuracion,
			"IP_FS");
	configuracion.PUERTO_FS = get_campo_config_string(archivo_configuracion,
			"PUERTO_FS");
	return configuracion;
}

//Estructuras FileSystem

typedef struct t_directory {
	int index;
	char* nombre;
	int padre;
} t_directory;

typedef struct ubicacionBloque {
	char* nroNodo;
	int nroBloque; //en nodo
	char* ip;
	int puerto;
} ubicacionBloque;

typedef struct t_bloque {
	int nroBloque;
	ubicacionBloque* copia1;
	ubicacionBloque* copia2;
	unsigned long int finBloque;
} t_bloque;

typedef struct t_archivo {
	char* nombre; //solo el nombre de archivo
	char* path; // escribir todo menos yamafs://
	t_tipo_archivo tipoArchivo;
	unsigned long int tamanio;
	t_list* bloques; // se le deben agregar struct t_bloque
	int indiceDirectorioPadre;
} t_archivo;

typedef struct t_nodo {
	char* nroNodo;
	un_socket socket;
	bool ocupado;
	t_bitarray* bitmap;
	char* ip;
	int puertoWorker;
	int tamanio;
	int libre;
} t_nodo;

typedef struct t_nodoasignado {
	char* nodo1;
	char* nodo2;
	int bloque1;
	int bloque2;
} t_nodoasignado;

typedef struct t_fs {
	t_list* ListaNodos; //se le deben agregar struct t_nodo
	t_list* listaArchivos; //se le deben agregar struct t_archivo
	int tamanio;
	int libre;
} t_fs;

typedef struct t_bloque_particion {
	char* contenido;
	int ultimoByteValido;
} t_bloque_particion;

typedef struct t_archivo_partido {
	int cantidadBloques; //en cuantos bloques se parte
	t_list* bloquesPartidos; //lista de t_bloque_particion
} t_archivo_partido;

static t_nodo *nodo_create(char* nroNodo, bool ocupado, t_bitarray* bitmap,
	un_socket socket, char* ipWorker, int puertoWorker, int tamanio, int cantidadBloques) {
	t_nodo *new = malloc(sizeof(t_nodo));
	new->bitmap = bitmap;
	new->nroNodo = nroNodo;
	new->ocupado = ocupado;
	new->socket = socket;
	new->libre = cantidadBloques;
	new->ip = malloc(strlen(ipWorker) + 1);
	strcpy(new->ip, ipWorker);
	new->puertoWorker = puertoWorker;
	new->tamanio = tamanio;
	return new;
}

static void nodo_destroy(t_nodo* nodo) {
	free(nodo->bitmap); //checkear que sea asi
	free(nodo->nroNodo);
	//free(nodo->ocupado);
}

//inicializacion de estructuras
t_directory* tablaDeDirectorios[99];

t_fs fileSystem;

void hiloFileSystem_Consola();

t_bitarray leerBitmap(char*);

// Copiar un archivo local al yamafs, siguiendo los lineamientos en la operaciòn Almacenar Archivo, de la Interfaz del FileSystem.
void CP_FROM(char* origen, char* destino, t_tipo_archivo tipoArchivo);

//
t_archivo_partido* LeerArchivo(char* archivo, t_tipo_archivo tipoArchivo);

// Copiar un archivo local desde el yamafs
void CP_TO(char* origen, char* destino);

//
t_nodo* buscar_nodo_libre(char* nodoAnterior);

// Busca un nodo a partir del nombre del mismo y devuelce el t_nodo
t_nodo* buscar_nodo(char* nombreNodo);

// Evalia si un nodo esta ocupado y en caso de no estarlo devuelve el nro de bloque libre
int buscarBloque(t_nodo*);

// Envia a un nodo un bloque en el data.bin a ser escrito
void enviar_bloque_a_escribir(int numBloque, void* contenido, t_nodo*,
		int ultimoByteValido);

// Le pide a un nodo el contenido de un bloque especifico en su data.bin
void* getbloque(int, t_nodo*);

//
t_nodoasignado* escribir_bloque(void* bloque);

// Recibe donde dejar el path, donde dejar el file y el path/file concatenado
void split_path_file(char** p, char** f, char *pf);

// Actualiza el temporal en metadata que posee la info sobre la estructura tablaDeDirectorios
void actualizarArchivoDeDirectorios();

// Remplaza rep que exista en orig con with
char *str_replace(char *orig, char *rep, char *with);

// Contabiliza la cantidad de veces que sucede toSearch en str
int countOccurrences(char * str, char * toSearch);

// Crea un directorio nuevo en la primera posicion libre de tablaDeDirectorios[99]
t_directory* crearDirectorio(int padre, char* nombre);

// Busca un directorio en tablaDeDirectorios[99]
t_directory* buscarDirectorio(int padre, char* nombre);

// Crea una subcarpeta (fisica)
void crear_subcarpeta(char* nombre);

//
void actualizarBitmap(t_nodo* unNodo);

//
void actualizarArchivoTablaNodos();

// Muestra el contenido del archivo como texto plano (del fs)
void cat(char*);

//
void imprimirPorPantallaContenidoBloque(void* elem);

//
bool buscarArchivoPorPath(void* elem);

// Crea una copia de un bloque de un archivo en el nodo dado.
void cp_block(char* path, int numeroBloque, char* nombreNodo);

// Calcula un Hash
void calcular_md5(char* path);

// Lista los archivos de un directorio
void ls(char*path);

// Trae todos los subdirectorios a partir de un indice basandose en la tablaDeDirectorios[99]
t_list* obtenerSubdirectorios(int indicePadre);

//
void recopilarInfoCopia(ubicacionBloque* copia, t_archivoxnodo* archivoxnodo,
		t_bloque* bloque);

// Crea un directorio. Si el directorio ya existe, el comando deberá informarlo. //todo revisar
void YAMA_mkdir(char* path);

// Actualiza los temporales que persisten las estructuras de t_archivo pertenecientes a listaArchivos (fs)
void actualizarTablaArchivos();

// Actualiza el temporal que persiste la estructura de un t_archivo perteneciente a la lista listaArchivos (fs)
void actualizarArchivo(t_archivo*);

// Elimina de manera logica un archivo
void eliminar_archivo(char*);

//Imprime por pantalla nombre tamaño bloques e info de bloques de un archivo
void info_archivo(char* path);

//Mueve un archivo o directorio ubicado en path_origen a path_destino
void yama_mv(char* path_origen, char* path_destino, char tipo );

//mueve un archivo a path destino
void Mover_Archivo(char* path_destino, t_archivo* archivoEncontrado);

// Renombra un archivo en yama
void yama_rename(char*, char*);
