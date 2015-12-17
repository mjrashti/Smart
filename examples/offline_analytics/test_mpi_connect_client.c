#include <stdio.h>
#include <mpi.h>
#include <string.h>

#define MAX_CLIENTS 2 //(MPI_UNIVERSE_SIZE - 1)
#define TAG_0   0

#define PRINT_FUNC_LINE printf("%s: %d\n",__FUNCTION__,__LINE__);

char port_name[MPI_MAX_PORT_NAME];

int main(int argc, char *argv[]){

	int error,i,client_num = 0,new_rank,nprocs;
	char *service_name = "mpi_yarn_service";
	MPI_Comm	new_comm[MAX_CLIENTS],new_intra_comm[MAX_CLIENTS],main_comm,client_comm;
	MPI_Status	status;
	char new_message[100];

	MPI_Init(&argc,&argv);

	printf("Client starting \n");
	
	if (MPI_Lookup_name(service_name, MPI_INFO_NULL, port_name) != MPI_SUCCESS) {
	        printf("MPI_Lookup_name: %s is failed!\n", service_name);
        	return -1;
   	}
	printf("Looked up service %s: port %s \n",service_name,port_name);
	if (MPI_Comm_connect(port_name, MPI_INFO_NULL,0, MPI_COMM_SELF,&new_comm[0]) != MPI_SUCCESS){
		printf("MPI_Comm_connect: %s is failed!\n", service_name);
                return -1;
	}
	printf("connected ...");
	MPI_Intercomm_merge(new_comm[0], 1, &new_intra_comm[0]);
	MPI_Comm_size(new_intra_comm[0],&nprocs);
	MPI_Comm_rank(new_intra_comm[0],&new_rank);
	sprintf(new_message,"I am client with new rank %d",new_rank);
	MPI_Send(new_message,strlen(new_message),MPI_CHAR,0,TAG_0,new_intra_comm[0]);
	MPI_Recv(new_message,100,MPI_CHAR,0,TAG_0,new_intra_comm[0],&status);
	sscanf(new_message,"%d",&client_num);
	printf("Client %d connected to port %s, %d procs, rank %d\n",client_num,port_name,nprocs,new_rank);

	for(i = 1;i<MAX_CLIENTS - client_num;i++){
		MPI_Barrier(new_intra_comm[i - 1]);
		MPI_Comm_accept(port_name,MPI_INFO_NULL,0,
							new_intra_comm[i - 1],&new_comm[i]);
		PRINT_FUNC_LINE;
		MPI_Intercomm_merge(new_comm[i], 0, &new_intra_comm[i]);
	}
	PRINT_FUNC_LINE;
	
	main_comm = new_intra_comm[i - 1];	

	//exit process
	MPI_Barrier(main_comm);
	MPI_Comm_rank(main_comm,&new_rank);
	MPI_Comm_size(main_comm,&nprocs);
        printf("Client %d final rank in the new intracomm of size %d: %d\n",client_num,nprocs,new_rank);

	MPI_Comm_split(main_comm,(new_rank==0),new_rank,&client_comm);
	MPI_Barrier(client_comm);
	MPI_Comm_rank(client_comm,&new_rank);
        MPI_Comm_size(client_comm,&nprocs);
	printf("Client %d final rank in the client only intracomm of size %d: %d\n",client_num,nprocs,new_rank);

	MPI_Finalize();
	return 0;
}
