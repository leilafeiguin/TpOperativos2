/*
 ============================================================================
 Name        : Yama.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "Yama.h"

int main(void) {
	printf("Inicializando proceso Yama\n");
	yama_configuracion configuracion = get_configuracion();

	return EXIT_SUCCESS;
}
