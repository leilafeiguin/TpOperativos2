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

int main(void) {
	printf("Inicializando proceso Master\n");
	master_configuracion configuracion = get_configuracion();

	return EXIT_SUCCESS;
}
