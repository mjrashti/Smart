#ifndef __YARN_INTEGRATION__
#define __YARN_INTEGRATION__

#define MPI_SERVICE_NAME "mpi_yarn_service"
#define TAG_0   0

#define PRINT_FUNC_LINE printf("%s: %d\n",__FUNCTION__,__LINE__);


#ifdef USE_DEBUG

#define DBGX( fmt, args... ) \
{\
    fprintf( stderr, " RNET PM API - pid %d :%s():%d: "fmt,getpid(), __func__, __LINE__, ##args);\
}
#else

#define DBGX( fmt, args... )

#endif

#define ERRX( fmt, args... ) \
{\
    fprintf( stderr, " RNET PM API Error:%s():%d: "fmt, __func__, __LINE__, ##args);\
}

#ifdef ENABLE_YARN_INTEGRATION
 #define MPI_COMM_WORLD yarn_world_comm

 #define mpiConnectInit(c,v)    	\
        	({int t;			\
                MPI_Init(c,v);		\
                YarnIntegration yi;	\
                yarn_world_comm = yi.mpiConnect();		\
                if(yarn_world_comm == MPI_COMM_NULL){		\
                        ERRX("Error enabling MPI yarn integration, aborting ...\n");\
                        MPI_Finalize();	\
                        t = -1;		\
                }			\
		else    t = 0;t;})

 #define MPI_Init(c,v)  mpiConnectInit(c,v)

 MPI_Comm yarn_world_comm;
#endif

class YarnIntegration {
private:
	MPI_Comm yarnWorldComm;
public:
	MPI_Comm getYarnWorldComm(void);	
	MPI_Comm mpiConnect(void);
	MPI_Comm mpiServer(int num_clients);
	void	mpiDisconnect(void);
};


#endif
