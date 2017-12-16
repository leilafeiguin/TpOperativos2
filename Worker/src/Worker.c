#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Worker.h"
#include "socketConfig.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <time.h>
#include <dirent.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_LINE 4096
t_log* logger;

void* archivo;

int main(void) {
	imprimir("/home/utnso/Desktop/tp-2017-2c-Todo-ATR/Libraries/worker.txt");
	char* fileLog;

	fileLog = "WorkerLogs.txt";
	logger = log_create(fileLog, "Worker Logs", 0, 0);
	log_trace(logger, "Inicializando proceso Worker. \n");
	worker_configuracion configuracion = get_configuracion();
	log_trace(logger, "Archivo de configuracion levantado. \n");
	struct stat sb;
	if(access(configuracion.RUTA_DATABIN, F_OK) == -1) {
		FILE* fd=fopen(configuracion.RUTA_DATABIN, "a+");
		ftruncate(fileno(fd), 20*1024*1024);
		fclose(fd);
	}
	//Se limpian los directorios
	crear_subcarpeta("./bloque");
	crear_subcarpeta("./script");
	crear_subcarpeta("./tm");
	crear_subcarpeta("./reduc");
	vaciarCarpeta("./bloque");
	vaciarCarpeta("./script");
	vaciarCarpeta("./tm");
	vaciarCarpeta("./reduc");

	//Se abre data bin en modo read
	int fd=open(configuracion.RUTA_DATABIN, O_RDONLY, S_IWUSR);
	fstat(fd, &sb);
	archivo= mmap(NULL,sb.st_size,PROT_READ,  MAP_SHARED,fd,0); //PROT_READ ??

	un_socket socketServer=socket_escucha(configuracion.IP_NODO, configuracion.PUERTO_WORKER);
	listen(socketServer, 999);
	while(1){
	un_socket socketConexion=aceptar_conexion(socketServer);
	if(socketConexion >0)
	{
		printf("Antes de fork1\n");
		int pid=fork();
		if(pid == 0){
			printf("Antes de fork2\n");
			if(fork() == 0){
				switch(pid)
				{
					case -1: // Si pid es -1 quiere decir que es el proceso padre, ha habido un error
						perror("No se ha podido crear el proceso hijo\n");
						break;
					case 0: // Cuando pid es cero quiere decir que es el proceso hijo
					{
						printf("Termine Fork\n");
						t_paquete* paquete_recibido = recibir(socketConexion);
						if(paquete_recibido->codigo_operacion == cop_handshake_master){
							esperar_handshake(socketConexion, paquete_recibido, cop_handshake_master);
							paquete_recibido = recibir(socketConexion);

							switch(paquete_recibido->codigo_operacion){ //revisar validaciones de habilitados
								case cop_worker_transformacion:
								{
									int desplazamiento = 0;
									int cantidadElementos = 0;
									memcpy(&cantidadElementos, paquete_recibido->data, sizeof(int));
									desplazamiento += sizeof(int);
									int i;
									char* fileTemporal=generarDirectorioTemporal( "./script/");
									char* fileScript=string_from_format("%s%s" , fileTemporal, ".py");
									mode_t mode = S_IRUSR | S_IWUSR | S_IXUSR| S_IRGRP | S_IWGRP | S_IXGRP |S_IXOTH | S_IWOTH | S_IROTH;
									int fileBloque = open(fileScript, O_RDWR | O_CREAT | O_SYNC, mode);
									bool primeraVez=true;


									for(i=0;i<cantidadElementos;i++){
										t_transf* paquete_transformacion = malloc(sizeof(t_transf));
										memcpy(&paquete_transformacion->cant_script, paquete_recibido->data+desplazamiento, sizeof(int));
										desplazamiento += sizeof(int);
										paquete_transformacion->script = malloc(paquete_transformacion->cant_script);
										memcpy(paquete_transformacion->script, paquete_recibido->data + desplazamiento, paquete_transformacion->cant_script);
										desplazamiento += paquete_transformacion->cant_script;
										memcpy(&paquete_transformacion->bloq, paquete_recibido->data + desplazamiento, sizeof(int));
										desplazamiento += sizeof(int);
										paquete_transformacion->cant_archivo_temporal = 11;
										paquete_transformacion->archivo_temporal = malloc(paquete_transformacion->cant_archivo_temporal);
										memcpy(paquete_transformacion->archivo_temporal, paquete_recibido->data + desplazamiento, paquete_transformacion->cant_archivo_temporal);
										desplazamiento += paquete_transformacion->cant_archivo_temporal;
										memcpy(&paquete_transformacion->cant_ocupada_bloque, paquete_recibido->data + desplazamiento, sizeof(int));
										desplazamiento += sizeof(int);

										if(primeraVez)
										{
											paquete_transformacion->cant_script--;
											ftruncate(fileBloque,paquete_transformacion->cant_script);
											char* bloqueTemp= mmap(NULL,paquete_transformacion->cant_script,PROT_READ | PROT_WRITE  | PROT_EXEC,  MAP_SHARED,fileBloque,0);
											memcpy(bloqueTemp,paquete_transformacion->script,paquete_transformacion->cant_script);
											msync(bloqueTemp,paquete_transformacion->cant_script,MS_SYNC);
											munmap(bloqueTemp,paquete_transformacion->cant_script);
											printf("%i\n",close(fileBloque));
											system(string_from_format("chmod +x %s", fileScript));

											primeraVez=false;
										}

										printf("Escribiendo bloque en temporal \n");
										t_retorno_bloque* retornoBloque = obtenerBloque(paquete_transformacion->bloq, paquete_transformacion->cant_ocupada_bloque);
										printf("obtuvo el bloque  \n");
										char* pathFileBloque= generarDirectorioTemporal("./bloque/");
										printf("genero dir temp \n");
										pathFileBloque = string_from_format("%s%i", pathFileBloque,paquete_transformacion->bloq);
										int fileBloque = open(pathFileBloque, O_RDWR | O_CREAT | O_SYNC, mode);
										ftruncate(fileBloque,retornoBloque->tamanio);

										char* bloqueTemp= mmap(NULL,retornoBloque->tamanio,PROT_READ | PROT_WRITE | PROT_EXEC,  MAP_SHARED,fileBloque,0);
										memcpy(bloqueTemp,retornoBloque->bloque,retornoBloque->tamanio);
										msync(bloqueTemp,retornoBloque->tamanio,MS_SYNC);
										close(fileBloque);
										chmod(pathFileBloque,777);
										printf("%s\n",paquete_transformacion->archivo_temporal);
										transformacion(fileScript, pathFileBloque, paquete_transformacion->archivo_temporal);//paquete_transformacion->archivo_temporal);

										printf("Transforme\n");
									}
									char* mensaje = malloc(3);
									mensaje = "ok";
									enviar(socketConexion, cop_master_estado_transformacion , 3, mensaje);
									exit(0);
								}
								break;
								case cop_worker_reduccionLocal:
								{
									printf("Comienzo reduc local \n");
									int cantElementos;
									int desplazamiento=0;
									int i = 0;
									int longitudIdWorker;
									char* worker_id;
									int tamScriptReduc;

									memcpy(&cantElementos, paquete_recibido->data, sizeof(int));
									desplazamiento += sizeof(int);

									char* archivosAReducir[cantElementos];
									for(; i < cantElementos;i++) {
										int tamanioElemento=0;
										memcpy(&tamanioElemento, paquete_recibido->data + desplazamiento, sizeof(int));
										desplazamiento += sizeof(int);
										archivosAReducir[i] = malloc(tamanioElemento);

										memcpy(archivosAReducir[i], paquete_recibido->data + desplazamiento, tamanioElemento); //tamanaio ele + 1 ??
										desplazamiento += tamanioElemento;
									}

									int tamanio_archivo_reducido;
									memcpy(&tamanio_archivo_reducido, paquete_recibido->data + desplazamiento, sizeof(int));
									desplazamiento += sizeof(int);

									char* archivo_reducido = malloc(tamanio_archivo_reducido);
									memcpy(archivo_reducido, paquete_recibido->data + desplazamiento, tamanio_archivo_reducido);
									desplazamiento +=tamanio_archivo_reducido;

									memcpy(&longitudIdWorker,paquete_recibido->data+desplazamiento,sizeof(int));
									desplazamiento+=sizeof(int);
									worker_id = malloc(longitudIdWorker);

									memcpy(worker_id,paquete_recibido->data + desplazamiento, longitudIdWorker);
									desplazamiento+=longitudIdWorker;

									memcpy(&tamScriptReduc,paquete_recibido->data + desplazamiento,sizeof(int));
									desplazamiento+=sizeof(int);

									char* scriptReduc = malloc(tamScriptReduc);
									memcpy(scriptReduc,paquete_recibido->data + desplazamiento,tamScriptReduc);
									desplazamiento+=tamScriptReduc;
									char* reducidoTemporal = generarDirectorioTemporal( "./reduc/");
									printf("Estoy por aparear \n");
									bool resultado = apareo(archivosAReducir,cantElementos ,reducidoTemporal);

									printf("Ya pase el apareo \n");
									char* reductorTemporal=generarDirectorioTemporal( "./script/");
									char* fileScript=string_from_format("%s%s" , reductorTemporal, ".py");
									printf("%s",fileScript);
									mode_t mode = S_IRUSR | S_IWUSR | S_IXUSR| S_IRGRP | S_IWGRP | S_IXGRP |S_IXOTH | S_IWOTH | S_IROTH;
									int filereduc = open(fileScript, O_RDWR | O_CREAT | O_SYNC, mode);
									ftruncate(filereduc,tamScriptReduc);
									char* scriptTemp= mmap(NULL,tamScriptReduc,PROT_READ | PROT_WRITE  | PROT_EXEC,  MAP_SHARED,filereduc,0);
									memcpy(scriptTemp,scriptReduc,tamScriptReduc);
									msync(scriptTemp,tamScriptReduc,MS_SYNC);
									munmap(scriptTemp,tamScriptReduc);
									printf("%i\n",close(filereduc));
									system(string_from_format("chmod +x %s", fileScript));
									system(string_from_format("chmod +x %s", reducidoTemporal));
									printf("Voy a reducir \n");
									char* func = string_from_format("cat %s | %s > .%s", reducidoTemporal,"./reductor.py" ,archivo_reducido);
									system(func);
									printf("Ya reduje \n");

									int longitudIp = strlen(configuracion.IP_NODO);
									char* buffer = malloc(longitudIp + sizeof(int) + sizeof(int) + sizeof(bool)+sizeof(int)+longitudIdWorker);
									desplazamiento = 0;

									memcpy(buffer, &longitudIp, sizeof(int));
									desplazamiento += sizeof(int);

									memcpy(buffer + desplazamiento, configuracion.IP_NODO, longitudIp + 1);
									desplazamiento += longitudIp;

									memcpy(buffer + desplazamiento, &configuracion.PUERTO_WORKER, sizeof(int));
									desplazamiento += sizeof(int);

									memcpy(buffer + desplazamiento, &resultado, sizeof(bool));
									desplazamiento+=sizeof(bool);

									memcpy(buffer+desplazamiento,&longitudIdWorker,sizeof(int));
									desplazamiento+=sizeof(int);

									memcpy(buffer+desplazamiento,worker_id,longitudIdWorker);
									desplazamiento+=longitudIdWorker;

									enviar(socketConexion, cop_worker_reduccionLocal, strlen(buffer), buffer);
									free(buffer);

								}
								break;
								case cop_worker_reduccionGlobal:
								{
									t_list* archivosPorSolicitar = list_create();
									int cantidadElementos;
									int desplazamiento = 0;
									int i = 0;
									memcpy(&cantidadElementos, paquete_recibido->data, sizeof(int));
									desplazamiento+=sizeof(int);
									for(;i<cantidadElementos;i++){
										t_archivoRG* unPedido = malloc(sizeof(t_archivoRG));
										int tamIp;
										int tamArch;
										int tamId;
										memcpy(&tamIp, paquete_recibido->data+desplazamiento, sizeof(int));
										desplazamiento+=sizeof(int);

										memcpy(unPedido->ip, paquete_recibido->data+desplazamiento, tamIp);
										desplazamiento+=tamIp;

										memcpy(&unPedido->puerto, paquete_recibido->data+desplazamiento, sizeof(int));
										desplazamiento+=sizeof(int);

										memcpy(&tamArch, paquete_recibido->data+desplazamiento, sizeof(int));
										desplazamiento+=sizeof(int);

										memcpy(unPedido->archivo, paquete_recibido->data+desplazamiento, tamArch);
										desplazamiento+=tamArch;

										memcpy(&tamId, paquete_recibido->data+desplazamiento, sizeof(int));
										desplazamiento+=sizeof(int);

										memcpy(unPedido->id, paquete_recibido->data+desplazamiento, tamId);
										desplazamiento+=tamId;

										list_add(archivosPorSolicitar,unPedido);
									}
									int tamArchFin;
									memcpy(&tamArchFin, paquete_recibido->data+desplazamiento, sizeof(int));
									desplazamiento+=sizeof(int);

									char* archFin = malloc(tamArchFin);
									memcpy(archFin, paquete_recibido->data+desplazamiento, tamArchFin);
									desplazamiento+=tamArchFin;

									t_list* archivosAReucir = list_create();
									void solicitarArchivo(t_archivoRG* unPedido){
										if(strcmp(unPedido->id,configuracion.NOMBRE_NODO)!=0){
											un_socket socket;
											socket = conectar_a(unPedido->ip,string_from_format("%i",unPedido->puerto));
											realizar_handshake(socket,cop_handshake_worker);
											enviar(socket,cop_worker_archivo,strlen(unPedido->archivo)+1,unPedido->archivo);
											t_paquete* paquete_recibido;
											paquete_recibido = recibir(socket);
											FILE* fp = fopen(unPedido->archivo,"w");
											if(fp!=NULL){
												fwrite(paquete_recibido->data,paquete_recibido->tamanio,1,fp);
												fclose(fp);
											}
										}
										unPedido->archivo = malloc(strlen(unPedido->archivo)+1);
										list_add(archivosAReucir, unPedido->archivo);
										return;
									}
									list_iterate(archivosPorSolicitar,(void*)solicitarArchivo);
									i = 0;
									int cantidadArchivos = list_size(archivosAReucir);
									char* archivosAReucirPosta[cantidadArchivos-1];
									for(;0<cantidadArchivos;i++){
										archivosAReucirPosta[i] = malloc(strlen(list_get(archivosAReucir,i))+1);
										archivosAReucirPosta[i] = list_get(archivosAReucir,i);
									}
									bool resultado = apareo(archivosAReucirPosta,cantidadArchivos,archFin);
									if(resultado){
										desplazamiento = 0;
										FILE* fp = fopen(archFin,"r");
										int tamBuffer;
										char* buffer;
										if(fp!=NULL){
											fseek(fp, 0, SEEK_END);
											int tamanioArchivo = ftell(fp);
											fseek(fp, 0, SEEK_SET);
											tamBuffer =sizeof(int)+sizeof(int)+tamArchFin+tamanioArchivo;
											buffer = malloc(tamBuffer);
											memcpy(buffer + desplazamiento,&tamArchFin,sizeof(int));
											desplazamiento +=sizeof(int);

											memcpy(buffer + desplazamiento, archFin, tamArchFin);
											desplazamiento +=tamArchFin;

											memcpy(buffer+desplazamiento, &tamanioArchivo, sizeof(int));
											desplazamiento+=sizeof(int);

											fread(buffer+desplazamiento,tamanioArchivo,1,fp);
											fclose(fp);
										}
										enviar(socketConexion,cop_worker_reduccionGlobal , tamBuffer, buffer);
										free(buffer);
									}
								}
								break;
								case -1:
								{
									log_trace(logger, "Se cayo Master, finaliza Worker.\n");
									exit(-1);
								}
								break;
							}
						}else if(paquete_recibido->codigo_operacion == cop_handshake_worker){
							esperar_handshake(socketConexion, paquete_recibido, cop_handshake_worker);
							paquete_recibido = recibir(socketConexion);
							switch(paquete_recibido->codigo_operacion){ //revisar validaciones de habilitados
								case cop_worker_archivo:
								{
									FILE* fp = fopen((char*)paquete_recibido->data,"r");
									if(fp!=NULL){
										fseek(fp, 0, SEEK_END);
										int tamanioArchivo = ftell(fp);
										fseek(fp, 0, SEEK_SET);
										char* file = malloc(tamanioArchivo+1);
										fread(file,tamanioArchivo,1,fp);
										enviar(socketConexion,cop_worker_archivo,tamanioArchivo,file);
										free(file);
									}
								}
							}
						}
					}
				break;

			}
			exit(0);
		}
		else
		{
			int status;
			waitpid(pid, &status, 0 );
		}


			}
		}
	}
	return EXIT_SUCCESS;
}

char* generarDirectorioTemporal(char* carpeta){
	char* dirTemp= string_new();
	string_append(&dirTemp, carpeta);
	string_append(&dirTemp, randstring(5));//todo buscar una funcion que te genere X cantidad de caracteres aleatorios
	return dirTemp;
}

char *randstring(size_t length) {
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
    static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char *randomString = NULL;
    if (length){
    	srand(tm.tm_sec);
        randomString = malloc(sizeof(char) * (length +1));
        if (randomString){
        	int n;
            for ( n = 0;n < length;n++) {
                int key = rand() % (int)(sizeof(charset) -1);
                randomString[n] = charset[key];
            }
            randomString[length] = '\0';
        }
    }
    return randomString;
}




t_retorno_bloque* obtenerBloque(int numeroBloque, int tamanioBloque){
	t_retorno_bloque* retornoBloque = malloc(sizeof(t_retorno_bloque));
	printf("%i - %i\n",numeroBloque,tamanioBloque);
	char* bloque= malloc(tamanioBloque);
	int posicion = (numeroBloque *1024*1024);
	memcpy (bloque,archivo + posicion, tamanioBloque);
	printf("2 obtener bloque \n");
 	int aux = strrchr(bloque,'\n')-bloque;
	printf("Tam = %i \n",aux);
	char* bloqueBien = malloc(aux);
	memcpy(bloqueBien, bloque, aux);
	printf("4 obtener bloque \n");
	free(bloque);
	retornoBloque->bloque = bloqueBien;
	retornoBloque->tamanio = aux;
	return retornoBloque;
}

void transformacion(char* script, char* bloque, char* destino){
	char* func = string_from_format("cat %s | %s | sort -d > %s ", bloque, script, destino);
	system(func);
}

void crear_subcarpeta(char* nombre) {
	struct stat st = { 0 };
	if (stat(nombre, &st) == -1) {
		mkdir(nombre, 0777);
	}
}

void vaciarCarpeta(char* ruta){
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (ruta)) != NULL) {
		while ((ent = readdir (dir)) != NULL) {
			char* file = string_from_format("%s/%s",ruta,ent->d_name);
			remove(file);
	  }
	  closedir (dir);
	}
}

unsigned long int lineCountFile(const char *filename){
    FILE *fp = fopen(filename, "r");
    unsigned long int linecount = 0;
    int c;
    if(fp == NULL){
    	log_trace(logger, "No se puede abrir el archivo.\n");
        fclose(fp);
        return 0;
    }
    while((c=fgetc(fp)) != EOF ){
        if(c == '\n')
            linecount++;
    }
    fclose(fp);
    return linecount;
}

void sortfile(char **array, int linecount){
    int i, j;
    char t[MAX_LINE];
    for(i=1;i<linecount;i++){
        for(j=1;j<linecount;j++){
            if(strcmp(array[j-1], array[j]) > 0){
                strcpy(t, array[j-1]);
                strcpy(array[j-1], array[j]);
                strcpy(array[j], t);
            }
        }
    }
}


bool apareo (char* paths [], int cantElems,char* nombre_ordenado){
	int i =0;
	char* nom=malloc(MAX_LINE);
	for(;i<cantElems;i++){
		strcat(nom,paths[i]);
		strcat(nom, " ");
	}
	char* func = string_from_format("cat %s> %s", nom, nombre_ordenado);
	printf("%s",func);
	system(func);
	return true;
}

