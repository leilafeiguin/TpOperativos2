/*
 ============================================================================
 Name        : DataNode.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "DataNode.h"

int main(void) {
	printf("Inicializando proceso DataNode\n");
	dataNode_configuracion configuracion = get_configuracion();

	return EXIT_SUCCESS;
}
