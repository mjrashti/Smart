
#include <mpi.h>
extern "C"{
 #include <stdlib.h>	
}
#include "yarn_integration.h"

int main(int argc, char *argv[]){
	int num_clients = atoi(argv[1]);
	YarnIntegration yi;

	printf("Starting server with %d clients...\n",num_clients);

	MPI_Init(&argc,&argv);

	return yi.mpiServer(num_clients);
}
