/* -*- mode: C; tab-width: 2; indent-tabs-mode: nil; -*- */

/*
 * This code has been contributed by the DARPA HPCS program.  Contact
 * David Koester <dkoester@mitre.org> or Bob Lucas <rflucas@isi.edu>
 * if you have questions.
 *
 * GUPS (Giga UPdates per Second) is a measurement that profiles the memory
 * architecture of a system and is a measure of performance similar to MFLOPS.
 * The HPCS HPCchallenge RandomAccess benchmark is intended to exercise the
 * GUPS capability of a system, much like the LINPACK benchmark is intended to
 * exercise the MFLOPS capability of a computer. In each case, we would
 * expect these benchmarks to achieve close to the "peak" capability of the
 * memory system. The extent of the similarities between RandomAccess and
 * LINPACK are limited to both benchmarks attempting to calculate a peak system
 * capability.
 *
 * GUPS is calculated by identifying the number of memory locations that can be
 * randomly updated in one second, divided by 1 billion (1e9). The term "randomly"
 * means that there is little relationship between one address to be updated and
 * the next, except that they occur in the space of one half the total system
 * memory. An update is a read-modify-write operation on a table of 64-bit words.
 * An address is generated, the value at that address read from memory, modified
 * by an integer operation (add, and, or, xor) with a literal value, and that
 * new value is written back to memory.
 *
 * We are interested in knowing the GUPS performance of both entire systems and
 * system subcomponents --- e.g., the GUPS rating of a distributed memory
 * multiprocessor the GUPS rating of an SMP node, and the GUPS rating of a
 * single processor. While there is typically a scaling of FLOPS with processor
 * count, a similar phenomenon may not always occur for GUPS.
 *
 */

#define MAXTHREADS 256
#include "xbrtime_morello.h"
// #include "xbrtime.h"
// #include <hpcc.h>
#include <stdio.h>
// #include "RandomAccess.h"

#define EXPERIMENTAL 1

// Define 64-bit types and corresponding format strings for printf() and constants
#ifdef LONG_IS_64BITS
typedef unsigned long u64Int;
typedef long s64Int;
#define FSTR64 "%ld"
#define FSTRU64 "%lu"
#define ZERO64B 0L
#else
typedef unsigned long long u64Int;
typedef long long s64Int;
#define FSTR64 "%lld"
#define FSTRU64 "%llu"
#define ZERO64B 0LL
#endif

/* Random number generator */
#ifdef LONG_IS_64BITS
#define POLY 0x0000000000000007UL
#define PERIOD 1317624576693539401L
#else
#define POLY 0x0000000000000007ULL
#define PERIOD 1317624576693539401LL
#endif

/* For timing */
static double RTSEC()
{
 struct timeval tp;
 gettimeofday (&tp, NULL);
 return tp.tv_sec + tp.tv_usec/(double)1.0e6;
}

extern s64Int starts (u64Int);

#ifdef HPCC_MEMALLCTR
extern int HPCC_alloc_init(size_t total_size);
extern int HPCC_alloc_finalize();
extern void *HPCC_malloc(size_t size);
extern void HPCC_free(void *ptr);
#define HPCC_fftw_malloc HPCC_malloc
#define HPCC_fftw_free HPCC_free
#define HPCC_XMALLOC(t,s) ((t*)HPCC_malloc(sizeof(t)*(s))) 
#else
/*
#define HPCC_malloc malloc
#define HPCC_free free
*/

#define HPCC_malloc malloc
#define HPCC_free free

#define HPCC_fftw_malloc fftw_malloc
#define HPCC_fftw_free fftw_free
#define HPCC_XMALLOC(t,s) XMALLOC(t,s)
#endif

void
do_abort(char* f)
{
  fprintf(stderr, "%s\n", f);
}

u64Int srcBuf[] = {
  0xb1ffd1da
};
u64Int targetBuf[sizeof(srcBuf) / sizeof(u64Int)];

/* Allocate main table (in global memory) */
u64Int *HPCC_Table;

int main(int argc, char **argv)
{
  int debug = 0;

  s64Int i;
  int NumProcs, logNumProcs, MyProc;
  u64Int GlobalStartMyProc;
  u64Int Top;               /* Number of table entries in top of Table */
  s64Int LocalTableSize;    /* Local table width */
  u64Int MinLocalTableSize; /* Integer ratio TableSize/NumProcs */
  u64Int logTableSize, TableSize;

  double CPUTime;               /* CPU  time to update table */
  double RealTime;              /* Real time to update table */

  double TotalMem;
  int PowerofTwo;

  double timeBound = -1;  /* OPTIONAL time bound for execution time */
  u64Int NumUpdates_Default; 
  /* Number of updates to table (suggested: 4x number of table entries) */
  u64Int NumUpdates;  /* actual number of updates to table - may be smaller than
                       * NumUpdates_Default due to execution time bounds */
  s64Int ProcNumUpdates; /* number of updates per processor */
  s64Int *NumErrors, *GlbNumErrors;
#ifdef RA_TIME_BOUND
  s64Int GlbNumUpdates;  /* for reduction */
#endif

  long *llpSync;
  long long *llpWrk;

  long *ipSync;
  int *ipWrk;

  FILE *outFile = NULL;
  double *GUPs;
  double *temp_GUPs;


  int numthreads;
  int *sAbort, *rAbort;

  RealTime = -RTSEC(); // Begin timed section
  xbrtime_init();
  RealTime += RTSEC(); // End timed section
  printf("\tINIT: RTSEC used = %.6f seconds\n", RealTime );

  /*Allocate symmetric memory*/
  sAbort = (int *)xbrtime_malloc(sizeof(int));
  rAbort = (int *)xbrtime_malloc(sizeof(int));
  GUPs = (double *)xbrtime_malloc(sizeof(double));
  temp_GUPs = (double *)xbrtime_malloc(sizeof(double));
  GlbNumErrors = (s64Int *)xbrtime_malloc(sizeof(s64Int));
  NumErrors = (s64Int *)xbrtime_malloc(sizeof(s64Int));

  *GlbNumErrors = 0;
  *NumErrors = 0;
  *GUPs = -1;

  NumProcs = xbrtime_num_pes();
  MyProc = xbrtime_mype();

  if (0 == MyProc) {
    outFile = stdout;
    setbuf(outFile, NULL);
  }

  TotalMem = 20000000;  /* max single node memory */
  TotalMem *= NumProcs; /* max memory in NumProcs nodes */

  TotalMem /= sizeof(u64Int);

  /* calculate TableSize --- the size of update array (must be a power of 2) */
  for (TotalMem *= 0.5, logTableSize = 0, TableSize = 1;
       TotalMem >= 1.0;
       TotalMem *= 0.5, logTableSize++, TableSize <<= 1)
    ; /* EMPTY */


  MinLocalTableSize = (TableSize / NumProcs);
  LocalTableSize = MinLocalTableSize;
  GlobalStartMyProc = (MinLocalTableSize * MyProc);

  *sAbort = 0;

  /*Shmalloc HPCC_Table for RMA*/
  HPCC_Table = (u64Int *)xbrtime_malloc( NumProcs * sizeof(u64Int)*LocalTableSize );
  if (! HPCC_Table) *sAbort = 1;

  // xbrtime_int_reduce_sum(rAbort, sAbort, 1, 1, 0);    
  //   // ERROR-CHECK: Collect abort flags
  // xbrtime_int_broadcast(rAbort, rAbort, 1, 1, 0);     
  //   // ERROR-CHECK: Broadcast abort flags
  // xbrtime_reduce_sum_broadcast_all((long long) rAbort, (long long) sAbort, NumProcs, 1, 0);

  // if (*rAbort > 0) {
  //   if (MyProc == 0) 
  //     fprintf(outFile, "Failed to allocate memory for the main table.\n");
  //   /* check all allocations in case there are new added and their order changes */
  //   if (HPCC_Table) HPCC_free( HPCC_Table );
  //   goto failed_table;
  // }

  /* Default number of global updates to table: 4x number of table entries */
  NumUpdates_Default = 4 * TableSize;
  ProcNumUpdates = 4*LocalTableSize;
  NumUpdates = NumUpdates_Default;

  if (MyProc == 0) {
    fprintf( outFile, 
      "Running on %d processors%s\n", NumProcs, PowerofTwo ? " (PowerofTwo)" : "");
    fprintf( outFile, 
      "Total Main table size = 2^" FSTR64 " = " FSTR64 " words\n",
      logTableSize, TableSize );
    if (PowerofTwo)
        fprintf( outFile, 
          "PE Main table size = 2^" FSTR64 " = " FSTR64 " words/PE\n",
          (logTableSize - logNumProcs), TableSize/NumProcs );
      else
        fprintf( outFile, 
          "PE Main table size = (2^" FSTR64 ")/%d  = " FSTR64 " words/PE MAX\n",
          logTableSize, NumProcs, LocalTableSize);

    fprintf( outFile, 
      "Default number of updates (RECOMMENDED) = " FSTR64 "\tand actually done = %d\n", 
      NumUpdates_Default,ProcNumUpdates*NumProcs);
  }

  /* Initialize main table */
  for(int currentPE = 0; currentPE < NumProcs; currentPE++){
    for (i=0; i<LocalTableSize; i++)
      HPCC_Table[currentPE * LocalTableSize + i] = MyProc;
  }
  //xbrtime_barrier();
  // for (int currentPE = 0; currentPE < NumProcs; currentPE++) {
  //   tpool_add_work(threads[currentPE].thread_queue, xbrtime_barrier, NULL);
  // }
  xbrtime_barrier_all();

  int j,k;
  int logTableLocal,ipartner,iterate,niterate;
  int ndata,nkeep,nsend,nrecv,index,nlocalm1;
  int numthrds;
  u64Int datum,procmask;
  u64Int *data,*send;
  void * tstatus;
  int *remote_proc, offset; // remote_proc is an array of remote processors
  u64Int *tb;
  s64Int remotecount;
  int thisPeId;
  int numNodes;
  int count2;

  s64Int *count;
  s64Int *updates;
  s64Int *all_updates;
  s64Int *ran;

  thisPeId = xbrtime_mype(); // the id of the current PE
  numNodes = xbrtime_num_pes(); // the total number of PEs

  count = (s64Int *) xbrtime_malloc(sizeof(s64Int)); 
  ran = (s64Int *) xbrtime_malloc(NumProcs * sizeof(s64Int)); 
    // Random number generator
  updates     = (s64Int *) xbrtime_malloc(sizeof(s64Int) * numNodes); 
    // An array of length npes to avoid overwrites
  all_updates = (s64Int *) xbrtime_malloc(sizeof(s64Int) * numNodes);  
    //  An array to collect sum*/

  *ran = starts(4*GlobalStartMyProc);

  niterate = ProcNumUpdates;
  logTableLocal = logTableSize - logNumProcs;
  nlocalm1 = LocalTableSize - 1;


  for (j = 0; j < numNodes; j++){
    updates[j] = 0;
    all_updates[j] = 0;
  }
  int verify=1;
  u64Int *remote_val;
  remote_val = (u64Int *) xbrtime_malloc(NumProcs * sizeof(u64Int));

  remote_proc = (int *) xbrtime_malloc(NumProcs * sizeof(int));


  //xbrtime_barrier();
  // for (int currentPE = 0; currentPE < NumProcs; currentPE++) {
  //   tpool_add_work(threads[currentPE].thread_queue, xbrtime_barrier, NULL);
  // }
  xbrtime_barrier_all();

  // niterate = 1000;
  fprintf(outFile, "niterate: %d\n", niterate);
  /* Begin timed section */
  RealTime = -RTSEC();
  
  for (int currentPE = 0; currentPE < NumProcs; currentPE++) {
    for (int iterate = 0; iterate < niterate; iterate++) {
      ran[currentPE] = (ran[currentPE] << 1) ^ (
                        (s64Int) ran[currentPE] < ZERO64B ? POLY : ZERO64B);
      remote_proc[currentPE] = 
                        (ran[currentPE] >> logTableLocal) & (numNodes - 1);

      // Forces updates to remote PE only
      if (remote_proc[currentPE] == MyProc) {
        remote_proc[currentPE] = (remote_proc[currentPE] + 1) % numNodes;  
        // Using modulo instead of division
      }

      void *func_args_get = {
        (long long *)(&remote_val[currentPE]),
        (long long *)(&HPCC_Table[currentPE * niterate + 
                      (ran[currentPE] & (LocalTableSize-1))]),
        1, 0, remote_proc[currentPE]
      }; 
      // Get a long long integer value from a remote memory location
      bool checkGet = tpool_add_work(threads[currentPE].thread_queue, 
                                     xbrtime_longlong_get, &func_args_get);
      if (!checkGet) {
        printf("Error: Unable to add get work to thread pool.\n");
      }
          
      remote_val[currentPE] ^= ran[currentPE];

      void *func_args_put = {
        (long long *)(&HPCC_Table[currentPE * niterate + 
                      (ran[currentPE] & (LocalTableSize-1))]),
        (long long *)(&remote_val[currentPE]),
        1, 0, remote_proc[currentPE]
      };   
      // Put a long long integer value to a remote memory location
      bool checkPut = tpool_add_work(threads[currentPE].thread_queue, 
                                     xbrtime_longlong_put, &func_args_put);
      if (!checkPut) {
        printf("Error: Unable to add put work to thread pool.\n");
      }

      tpool_add_work(threads[currentPE].thread_queue, xbrtime_barrier, NULL);

      if (verify) {
        // Atomic add of long long integer value to a remote mem location 
        __atomic_add_fetch(&updates[thisPeId], 1, __ATOMIC_SEQ_CST);  
        // Corrected the atomic memory order
        // xbrtime_longlong_atomic_add(&updates[thisPeId], 1, remote_proc);
        // __atomic_add_fetch(&updates[thisPeId], 1, remote_proc);
        // https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html
      }    
    }
  } 

  //xbrtime_barrier();
  // for (int currentPE = 0; currentPE < NumProcs; currentPE++) {
  //   tpool_add_work(threads[currentPE].thread_queue, xbrtime_barrier, NULL);
  // }
  xbrtime_barrier_all();

  /* End timed section */
  RealTime += RTSEC(); // End timed section

  /* Print timing results */
  if (MyProc == 0){
    *GUPs = 1e-9*NumUpdates / RealTime;
    fprintf( outFile, "Real time used = %.6f seconds\n", RealTime );
    fprintf( outFile, "%.9f Billion(10^9) Updates    per second [GUP/s]\n",
             *GUPs );
    fprintf( outFile, "%.9f Billion(10^9) Updates/PE per second [GUP/s]\n",
             *GUPs / NumProcs );
  }

  // if(verify){
  //   s64Int pe_updates = 0;
  //   for (j = 0; j < numNodes; j++)
  //     pe_updates += updates[j];
  //   printf("PE%d updates:%d\n",MyProc,updates[0]);
  //   // ERROR-CHECK: Collect all updates
  //   xbrtime_longlong_reduce_sum(all_updates, updates, NumProcs, 1, 0);    
  //   // ERROR-CHECK: Broadcast all updates   
  //   xbrtime_longlong_broadcast(all_updates, all_updates, NumProcs, 1, 0);   
    
  //   // xbrtime_reduce_sum_broadcast_all(all_updates, updates, NumProcs, 1, 0);

  //   if(MyProc == 0){
  //     for (j = 1; j < numNodes; j++)
  //       all_updates[0] += all_updates[j];
      
  //     printf("Total updates:%d\n",all_updates[0]);
  //     printf("%d * %d = %d\n", ProcNumUpdates, NumProcs, ProcNumUpdates*NumProcs);
      
  //     if(ProcNumUpdates*NumProcs == all_updates[0])
  //       printf("Verification passed!\n");
  //     else
  //       printf("Verification failed!\n");
  //   }
  // }

  RealTime = -RTSEC(); // Begin timed section
  xbrtime_barrier_all();
  RealTime += RTSEC(); // End timed section
  printf("\tBARRIER: RTSEC used = %.6f seconds\n", RealTime );

  //xbrtime_barrier();
  // for (int currentPE = 0; currentPE < NumProcs; currentPE++) {
  //   tpool_add_work(threads[currentPE].thread_queue, xbrtime_barrier, NULL);
  // }

  /* End verification phase */

  printf("Closing...\n");

  xbrtime_free(count);
  xbrtime_free(updates);
  xbrtime_free(ran);
  //xbrtime_barrier();
  // for (int currentPE = 0; currentPE < NumProcs; currentPE++) {
  //   tpool_add_work(threads[currentPE].thread_queue, xbrtime_barrier, NULL);
  // }
  // xbrtime_barrier_all();

  // Deallocate memory 
  //    (in reverse order of allocation which should help fragmentation)

  // if (HPCC_Table) HPCC_free( HPCC_Table );
  // xbrtime_free( HPCC_Table );
  // failed_table:

  // if (0 == MyProc) if (outFile != stderr) fclose( outFile );
  // fclose( outFile );
  // printf("Closed output file.\n");

  // xbrtime_barrier();
  // for (int currentPE = 0; currentPE < NumProcs; currentPE++) {
  //   tpool_add_work(threads[currentPE].thread_queue, xbrtime_barrier, NULL);
  // }
  // xbrtime_barrier_all();

  xbrtime_free(sAbort);
  xbrtime_free(rAbort);

  //xbrtime_barrier();
  // for (int currentPE = 0; currentPE < NumProcs; currentPE++) {
  //   tpool_add_work(threads[currentPE].thread_queue, xbrtime_barrier, NULL);
  // }
  xbrtime_barrier_all();

  xbrtime_close();

  return 0;
}

/* Utility routine to start random number generator at Nth step */
s64Int
starts(u64Int n)
{
  /* s64Int i, j; */
  int i, j;
  u64Int m2[64];
  u64Int temp, ran;

  while (n < 0)
    n += PERIOD;
  while (n > PERIOD)
    n -= PERIOD;
  if (n == 0)
    return 0x1;

  temp = 0x1;
  for (i=0; i<64; i++)
    {
      m2[i] = temp;
      temp = (temp << 1) ^ ((s64Int) temp < 0 ? POLY : 0);
      temp = (temp << 1) ^ ((s64Int) temp < 0 ? POLY : 0);
    }

  for (i=62; i>=0; i--)
    if ((n >> i) & 1)
      break;

  ran = 0x2;

  while (i > 0)
    {
      temp = 0;
      for (j=0; j<64; j++)
        if ((ran >> j) & 1)
          temp ^= m2[j];
      ran = temp;
      i -= 1;
      if ((n >> i) & 1)
        ran = (ran << 1) ^ ((s64Int) ran < 0 ? POLY : 0);
    }

  return ran;
}
