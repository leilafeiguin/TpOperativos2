/*
 ============================================================================
 Name        : FileSystem.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "FileSystem.h"

int main(void) {
	printf("Inicializando proceso FileSystem\n");
	fileSystem_configuracion configuracion = get_configuracion();

	return EXIT_SUCCESS;
}
