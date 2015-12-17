#include <memory>
#include <mpi.h>
#include <typeinfo>
#include <sys/time.h>

//MJR added
#define ENABLE_YARN_INTEGRATION
#include "yarn_integration.h"

//#include "hdf5_partitioner.h"
#include "netcdf_partitioner.h"

#include "logistic_regression.h"
#include "partitioner.h"
#include "scheduler.h"

#define NUM_THREADS 4  // The # of threads for analytics task.
// For logistic regression application, STEP and NUM_COLS in logistic_regression.h must be equal.
#define STEP  NUM_COLS  // The size of unit chunk for each single read, which groups a bunch of elements for mapping and reducing. (E.g., for a relational table, STEP should equal the # of columns.)
//#define NUM_ELEMS 1024  // The total number of elements of the simulated data.
#define NUM_ELEMS 524288

//#define NUM_ITERS 2
#define NUM_ITERS 10000  // The # of iterations.

#define PRINT_COMBINATION_MAP 1
#define PRINT_OUTPUT 1

//#define FILENAME  "data.h5"
//#define FILENAME  "/data/home/mrashti/projects/informer_hpcc/Smart/util/covtype_data_logistic_32col.csv.nc"
//#define VARNAME "var_31"

#define FILENAME  "/data/home/mrashti/projects/informer_hpcc/Smart/util/32_col_data_converted_for_smart_aws_test_osu.nc"
#define VARNAME "point"

//#define FILENAME "data.h5"
//#define VARNAME "point"


using namespace std;


/*MJR added - temporarily here
char port_name[MPI_MAX_PORT_NAME];
char new_message[100];
*/

int main(int argc, char* argv[]) {
  // MPI initialization.
  struct timeval tv1,tv2;
  gettimeofday(&tv1,NULL);

  int mpi_status = MPI_Init(&argc, &argv);
  if (mpi_status != MPI_SUCCESS) {
    printf("Failed to initialize MPI environment.\n");
    MPI_Abort(MPI_COMM_WORLD, mpi_status);
  }

/*#ifdef ENABLE_YARN_INTEGRATION

  YarnIntegration yi;
  yarn_world_comm = yi.mpiConnect();
  if(yarn_world_comm == MPI_COMM_NULL){
	printf("Error enabling MPI yarn integration, aborting ...\n");
	MPI_Finalize();
	exit(-1);
  }	
#endif
*/
  int rank,size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  printf("I am rank %d out of %d, pid %d\n",rank,size,getpid());

  // Load the data partition.
  //unique_ptr<Partitioner> p(new HDF5Partitioner(FILENAME, VARNAME, STEP));
  unique_ptr<Partitioner> p(new NetCDFPartitioner(FILENAME, VARNAME, STEP));
  p->load_partition();

  // Check if the data type matches the input data of Smart scheduler.
  assert(p->is_vartype(typeid(double).name()));

  // The output is a 2D array that indicates k vectors in a multi-dimensional
  // space.
  const size_t out_len = 1;  // The output is only a single weight vector.
  double** out = new double*[1];
  out[0] = new double[NUM_DIMS];

  // Set up the initial weights.
  double weights[NUM_DIMS];
  for (int i = 0; i < NUM_DIMS; ++i) {
    weights[i] = (double)rand() / RAND_MAX;
  }
  if (rank == 0) {
    printf("\nInitial weights:\n");
    printVector(weights);
  }

  SchedArgs args(NUM_THREADS, STEP, (void*)weights, NUM_ITERS);
  unique_ptr<Scheduler<double, double*>> lr(new LogisticRegression<double>(args)); 
  lr->set_red_obj_size(sizeof(GradientObj));
  lr->run((const double*)p->get_data(), p->get_len(), out, out_len);

  // Print out the combination map if required.
  if (PRINT_COMBINATION_MAP && rank == 0) {
    printf("\n");
    lr->dump_combination_map();
  }

  // Print out the final result on the master node if required.
  if (PRINT_OUTPUT && rank == 0) {
    printf("Final output on the master node:\n");
    for (int i = 0; i < NUM_DIMS; ++i) {
      printf("%.2f ", out[0][i]);
    }
    printf("\n");
  }

  delete [] out[0];
  delete [] out;

  MPI_Finalize();
  gettimeofday(&tv2,NULL);
  printf("Total time to execute this program was: %ld usec\n",(tv2.tv_sec - tv1.tv_sec)*1000000+(tv2.tv_usec - tv1.tv_usec));

  return 0;
}
