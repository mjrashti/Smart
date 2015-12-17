#include <stdio.h>
#include <mpi.h>
#include <string.h>

#define MAX_CLIENTS 2 //(MPI_UNIVERSE_SIZE - 1)
#define TAG_0   0

#define PRINT_FUNC_LINE printf("%s: %d\n",__FUNCTION__,__LINE__);

char port_name[MPI_MAX_PORT_NAME];

int main(int argc, char *argv[]){

	int error,i,clients = 0,new_rank,nprocs;
	char *service_name = "mpi_yarn_service";
	MPI_Comm	new_comm[MAX_CLIENTS],new_intra_comm[MAX_CLIENTS],main_comm,lone_comm;
	MPI_Status	status;
	char new_message[100];

	MPI_Init(&argc,&argv);

	printf("Server starting...\n");
	
	if (MPI_Open_port(MPI_INFO_NULL,port_name ) != MPI_SUCCESS) {
	        printf("MPI_Open_port: %s is failed!\n", port_name);
        	return -1;
   	}
	printf("Server opened port %s\n",port_name);
	if (MPI_Publish_name(service_name, MPI_INFO_NULL, port_name) != MPI_SUCCESS){
		printf("MPI_Publish_name: %s is failed!\n", service_name);
                return -1;
	}

	printf("Published name %s, port %s waiting for clients\n",service_name,port_name);

	/*Steps to test:
		1. Create the client program.
		2. Run and see if a server daemon needs to be running such as in openmpi to 
			access/publish services
		3. If not, try to find a way to merge the intercomms created by multiple
			clients connecting to this server. The result should be a common
			inter or intra communicator.
	*/

	/*if ((error  = MPI_Comm_dup(MPI_COMM_WORLD,&main_comm)) != MPI_SUCCESS){
                printf("MPI_Comm_dup failed: %d!\n", error);
                return -1;
        }*/
	while(clients < MAX_CLIENTS){
		PRINT_FUNC_LINE;
		if(!clients){
			MPI_Comm_accept(port_name,MPI_INFO_NULL,0,MPI_COMM_SELF,&new_comm[clients]);
			PRINT_FUNC_LINE;
		}else{
			MPI_Barrier(new_intra_comm[clients - 1]);
			PRINT_FUNC_LINE;
			MPI_Comm_accept(port_name,MPI_INFO_NULL,0,
							new_intra_comm[clients - 1],&new_comm[clients]);		
		}
		PRINT_FUNC_LINE;
		MPI_Intercomm_merge(new_comm[clients], 0, &new_intra_comm[clients]);
		PRINT_FUNC_LINE;
		MPI_Comm_size(new_intra_comm[clients],&nprocs);
		printf("new client #%d conencted, now have %d procs...!\n",clients, nprocs);
		MPI_Recv(new_message,100,MPI_CHAR,MPI_ANY_SOURCE,TAG_0,new_intra_comm[clients],&status);
		printf("Received client %d message: %s\n",status.MPI_SOURCE,new_message);
		sprintf(new_message,"%d",clients);
                MPI_Send(new_message,strlen(new_message),MPI_CHAR,status.MPI_SOURCE,
								TAG_0,new_intra_comm[clients]);
		PRINT_FUNC_LINE;
		clients++;
	}
	PRINT_FUNC_LINE;
	
	if (MPI_Unpublish_name(service_name, MPI_INFO_NULL, port_name) != MPI_SUCCESS){
		printf("MPI_Unpublish_name: %s is failed!\n", service_name);
                return -1;
	}
/*
	for(i = 0; i<  clients; i++){
		sprintf(new_message,"%d %d",clients,i);
                MPI_Send(new_message,strlen(new_message),MPI_CHAR,0,TAG_0,new_comm[i]);
		MPI_Intercomm_merge(new_comm[i], 0, &new_intra_comm[i]);
		MPI_Comm_rank(new_intra_comm[i],&new_rank);
		printf("My rank in the new intracomm with client #%d: %d\n",i,new_rank);
	}
	
	if ((error  = MPI_Comm_dup(new_intra_comm[0],&main_comm)) != MPI_SUCCESS){
		printf("MPI_Comm_dup failed: %d!\n", error);
                return -1;
	}
	for(i = 1; i<  clients; i++){
		MPI_Intercomm_create(main_comm,0, new_intra_comm[i],1,i, &new_comm[i]);
		MPI_Intercomm_merge(new_comm[i],0,&main_comm);
		MPI_Comm_size(main_comm,&nprocs);
		printf("Merged next process, now main_comm has %d procs.\n",nprocs);
	}
*/
	main_comm = new_intra_comm[clients - 1];	

	//exit process
	//MPI_Barrier(main_comm);
	MPI_Comm_rank(main_comm,&new_rank);
	MPI_Comm_size(main_comm,&nprocs);
        printf("My final rank in the new intracomm of size %d: %d\n",nprocs,new_rank);

	MPI_Comm_split(main_comm,(new_rank==0),new_rank,&lone_comm);
        MPI_Barrier(lone_comm);
        MPI_Comm_rank(lone_comm,&new_rank);
        MPI_Comm_size(lone_comm,&nprocs);
        printf("Server final rank in the split intracomm of size %d: %d\n",nprocs,new_rank);

	MPI_Finalize();
	return 0;
}
