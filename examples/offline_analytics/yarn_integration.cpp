
extern "C"{
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
}
#include <mpi.h>

#include "yarn_integration.h"

MPI_Comm YarnIntegration::mpiConnect(void){
	int i,client_num = 0,new_rank,nprocs,num_clients;
        MPI_Comm        *new_comm,*new_intra_comm,main_comm,client_comm,inter_comm,intra_comm;
        MPI_Status      status;
	char new_message[100];
	char port_name[MPI_MAX_PORT_NAME];

        if (MPI_Lookup_name((char *)MPI_SERVICE_NAME, MPI_INFO_NULL, port_name) != MPI_SUCCESS) {
                ERRX("MPI_Lookup_name: %s is failed!\n", MPI_SERVICE_NAME);
                return -1;
        }
        DBGX("Looked up service %s: port %s \n",MPI_SERVICE_NAME,port_name);
        if (MPI_Comm_connect(port_name, MPI_INFO_NULL,0, MPI_COMM_SELF,&inter_comm) != MPI_SUCCESS){
                ERRX("MPI_Comm_connect: %s is failed!\n", MPI_SERVICE_NAME);
                return MPI_COMM_NULL;
        }
		
	MPI_Intercomm_merge(inter_comm, 1, &intra_comm);
	MPI_Comm_size(intra_comm,&nprocs);
	MPI_Comm_rank(intra_comm,&new_rank);
	sprintf(new_message,"I am client with new rank %d",new_rank);
	MPI_Send(new_message,strlen(new_message),MPI_CHAR,0,TAG_0,intra_comm);
	MPI_Recv(new_message,100,MPI_CHAR,0,TAG_0,intra_comm,&status);
	sscanf(new_message,"%d %d",&client_num,&num_clients);
	DBGX("Client %d connected to port %s, %d procs so far out of %d, rank %d\n",client_num,port_name,nprocs,num_clients,new_rank);

	new_comm = (MPI_Comm *)malloc(num_clients * sizeof(MPI_Comm));
        new_intra_comm = (MPI_Comm *)malloc(num_clients * sizeof(MPI_Comm));
        if(new_comm == NULL || new_intra_comm == NULL){
                perror("Error allocating structures for continuing MPI client");
                return MPI_COMM_NULL;
        }
	new_comm[0] = inter_comm;
	new_intra_comm[0] = intra_comm;

	for(i = 1;i<num_clients - client_num;i++){
		//TODO: This barrier is not required - remove later
		MPI_Barrier(new_intra_comm[i - 1]);
		MPI_Comm_accept(port_name,MPI_INFO_NULL,0,
							new_intra_comm[i - 1],&new_comm[i]);
		MPI_Intercomm_merge(new_comm[i], 0, &new_intra_comm[i]);
	}
	
	main_comm = new_intra_comm[i - 1];	

	//exit process
	MPI_Comm_rank(main_comm,&new_rank);
	MPI_Comm_size(main_comm,&nprocs);
        DBGX("Client %d final rank in the new intracomm of size %d: %d\n",client_num,nprocs,new_rank);

	MPI_Comm_split(main_comm,(new_rank==0),new_rank,&client_comm);
	MPI_Barrier(client_comm);
	MPI_Comm_rank(client_comm,&new_rank);
        MPI_Comm_size(client_comm,&nprocs);
	DBGX("Client %d final rank in the client only intracomm of size %d: %d\n",client_num,nprocs,new_rank);
	yarnWorldComm = client_comm;
	return yarnWorldComm;
}

MPI_Comm YarnIntegration::getYarnWorldComm(void){
	return yarnWorldComm;
}

int YarnIntegration::mpiServer(int num_clients){
	
	int clients = 0,new_rank,nprocs;
	MPI_Comm	*new_comm,*new_intra_comm,main_comm,lone_comm;
	MPI_Status	status;
	char new_message[100];
	char port_name[MPI_MAX_PORT_NAME];

	new_comm = (MPI_Comm *)malloc(num_clients * sizeof(MPI_Comm));
	new_intra_comm = (MPI_Comm *)malloc(num_clients * sizeof(MPI_Comm));
	if(new_comm == NULL || new_intra_comm == NULL){
		perror("Error allocating structures for launching MPI service");
		return -1;
	}

	DBGX("Server starting for %d clients...\n",num_clients);
	
	if (MPI_Open_port(MPI_INFO_NULL,port_name ) != MPI_SUCCESS) {
	        ERRX("MPI_Open_port: %s is failed!\n", port_name);
        	return -1;
   	}
	DBGX("Server opened port %s\n",port_name);
	if (MPI_Publish_name(MPI_SERVICE_NAME, MPI_INFO_NULL, port_name) != MPI_SUCCESS){
		ERRX("MPI_Publish_name: %s is failed!\n", MPI_SERVICE_NAME);
                return -1;
	}

	DBGX("Published name %s, port %s waiting for clients\n",MPI_SERVICE_NAME,port_name);

	/*Steps to test:
		1. Create the client program.
		2. Run and see if a server daemon needs to be running such as in openmpi to 
			access/publish services
		3. If not, try to find a way to merge the intercomms created by multiple
			clients connecting to this server. The result should be a common
			inter or intra communicator.
	*/

	while(clients < num_clients){
		if(!clients){
			MPI_Comm_accept(port_name,MPI_INFO_NULL,0,MPI_COMM_SELF,&new_comm[clients]);
		}else{
			//TODO: This barrier is not required - remove it later.
			MPI_Barrier(new_intra_comm[clients - 1]);
			MPI_Comm_accept(port_name,MPI_INFO_NULL,0,
							new_intra_comm[clients - 1],&new_comm[clients]);		
		}
		MPI_Intercomm_merge(new_comm[clients], 0, &new_intra_comm[clients]);
		MPI_Comm_size(new_intra_comm[clients],&nprocs);
		DBGX("new client #%d conencted, now have %d procs...!\n",clients, nprocs);
		MPI_Recv(new_message,100,MPI_CHAR,MPI_ANY_SOURCE,TAG_0,new_intra_comm[clients],&status);
		DBGX("Received client %d message: %s\n",status.MPI_SOURCE,new_message);
		sprintf(new_message,"%d %d",clients,num_clients);
                MPI_Send(new_message,strlen(new_message),MPI_CHAR,status.MPI_SOURCE,
								TAG_0,new_intra_comm[clients]);
		clients++;
	}
	PRINT_FUNC_LINE;
	
	if (MPI_Unpublish_name(MPI_SERVICE_NAME, MPI_INFO_NULL, port_name) != MPI_SUCCESS){
		ERRX("MPI_Unpublish_name: %s is failed!\n", MPI_SERVICE_NAME);
                return -1;
	}
	main_comm = new_intra_comm[clients - 1];	

	//exit process
	MPI_Comm_rank(main_comm,&new_rank);
	MPI_Comm_size(main_comm,&nprocs);
        printf("My final rank in the new intracomm of size %d: %d\n",nprocs,new_rank);

	MPI_Comm_split(main_comm,(new_rank==0),new_rank,&lone_comm);
        //MPI_Barrier(lone_comm);
        MPI_Comm_rank(lone_comm,&new_rank);
        MPI_Comm_size(lone_comm,&nprocs);
        printf("Server final rank in the split intracomm of size %d: %d\n",nprocs,new_rank);

	MPI_Finalize();
	return 0;
}

