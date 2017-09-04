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


int main(void) {
	printf("Inicializando proceso Worker\n");
	worker_configuracion configuracion = get_configuracion();

	return EXIT_SUCCESS;
}
