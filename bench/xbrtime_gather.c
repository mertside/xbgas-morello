/* _xBGAS_gather.c__
 *
 * Copyright (C) 2017-2018 Tactical Computing Laboratories, LLC
 * All Rights Reserved
 * contact@tactcomplabs.com
 *
 * This file is a part of the XBGAS-RUNTIME package.  For license
 * information, see the LICENSE file in the top level directory
 * of the distribution.
 *
 */

#include <stdio.h>
#include <inttypes.h>
#include "xbrtime_morello.h"
#include "test.h"

#define DEBUG 1

// For timing
static double RTSEC() {
  struct timeval tp;
  gettimeofday (&tp, NULL);
  return tp.tv_sec + tp.tv_usec/(double)1.0e6;
}

int main( int argc, char **argv ){
	printf("[M]"GRN " Entered Main matmul...\n"RESET);
	/* vars */
  int 			rtn 			= 0;
  size_t 		sz 				= _XBGAS_ALLOC_SIZE_;
  size_t 		ne 				= _XBGAS_ALLOC_NELEMS_;
	size_t		pe				= 0;
	int64_t		target		= 0;
	int64_t		index			= 0;
	int64_t 	*idx     	= NULL;
  
  uint64_t 	i   			= 0;
  uint64_t  j         = 0;

  int row = 0;
  int col = 0;

  uint64_t 	*private  = NULL;
	uint64_t 	*shared  	= NULL;

	/* statistic gathering var */
	double		local			= 0;
	double		remote		= 0;
	double 		t_init	  = 0;
	double 		t_mem	  	= 0;
	double 		t_start  	= 0;
	double 		t_end  		= 0;

  double RealTime;  // Real time recording

	t_start = mysecond();
	/* init */
  RealTime = -RTSEC(); // Begin timed section
  rtn 	= xbrtime_init();
  RealTime += RTSEC(); // End timed section
  printf("[M]"GRN " Passed xbrtime_init()\n"RESET); 
  printf("\tRTSEC for xbrtime_ulonglong_get = %.6f seconds\n", RealTime );

  int num_pes = xbrtime_num_pes();
  row         = num_pes;
  col         = ne;

	idx  		= malloc( row * col * sizeof( uint64_t ));
	private = malloc( row * col * sizeof( uint64_t ));
  shared = (uint64_t *)(xbrtime_malloc( row * col * sizeof( uint64_t ) ));

  printf("[M]"GRN " Passed xbrtime_malloc()\n"RESET); 

#ifdef DEBUG
  printf( "PE=%d; *SHARED = 0x%"PRIu64"\n", xbrtime_mype(), (uint64_t)(shared) );
#endif
	srand(1);
	// pe		=	xbrtime_num_pes();
 	for( i = 0; i< col; i++ ){
    for( j = 0; j < col; j++ ){
      // idx[i] 			= (uint64_t)(rand()%(pe*ne-1));
      // shared[i] 	= (uint64_t)(xbrtime_mype());
      // private[i] 	= 99;

      idx[i*col + j] 			= (uint64_t)(rand()%(row*col-1));
      shared[i*col + j] 	= (uint64_t)(j + xbrtime_mype());
      private[i*col + j] 	= 99;
    }
	}
  printf("[M]"GRN " Passed idx[], shared[] & private[] init\n"RESET);

  /* perform a barrier */
#ifdef DEBUG
  printf( "PE=%d; EXECUTING BARRIER\n", xbrtime_mype() );
#endif
  xbrtime_barrier();

	if(xbrtime_mype() == 0){
		t_end = mysecond();
		t_init = t_end - t_start;
		printf("=========================\n");
		printf(" xBGAS Gather Benchmark\n");
		printf("=========================\n");
		printf("Data size       = %d Bytes\n", (int)(sz) * (int)(ne) );
		printf("Element #       = %d\n", (int) ne);
		printf("PE #            = %d\n", (int) pe);
		//printf("------------------------------------------\n");
		t_start = mysecond();
	}

	/* Gathering */
  RealTime = -RTSEC(); // Begin timed section
  for( i = 0; i < row; i++ ){
    for( j = 0; j < col; j++ ){
      target = idx[i*col + j]/ne;
      index  = idx[i*col + j] - target*ne;
      // private access
      if(target == xbrtime_mype()){
        private[i*col + j] = shared[index];
        local++;
      } else {
        // remote access

        void* func_args = {(unsigned long long *)(&(shared[i*col + j])),
                          (unsigned long long *)(&(shared[index])),
                            1, 1, 1};                                 

        bool check = false;
        check = tpool_add_work( threads[i].thread_queue, 
                                xbrtime_ulonglong_get, 
                                func_args);
        

        // xbrtime_ulonglong_get((unsigned long long *)(&(private[i])),								// dest
        //                       (unsigned long long *)(&(shared[index])),							// src
        //                       1,																										// ne
        //                       1,																										// stride
        //                       target); 																							// pe
        remote++;
        
      }
      printf("[M] "BYEL"Completed iter: %lu\n"RESET, i+1);
	  } 
  }
  RealTime += RTSEC(); // End timed section

  printf("[M] "BGRN"Passed xbrtime_ulonglong_get()\n"RESET);

  printf("\tRTSEC for xbrtime_ulonglong_get = %.6f seconds\n", RealTime );

  /* perform a barrier */
#ifdef DEBUG
  printf( "PE=%d; EXECUTING BARRIER\n", xbrtime_mype() );
#endif
  RealTime = -RTSEC(); // Begin timed section
  xbrtime_barrier();
  RealTime += RTSEC(); // End timed section

  if(xbrtime_mype() == 0){
    t_end = mysecond();
    t_mem = t_end - t_start;
	}
#ifdef DEBUG
  printf( "PE=%d; SHARED[0]=0x%"PRIu64"\n",
          xbrtime_mype(), shared[0]);
  printf( "PE=%d; xBGAS is Closing\n", xbrtime_mype() );
#endif
  sleep(1);

	if(xbrtime_mype() == 0)
		PRINT(local, remote, t_init, t_mem);

  printf("\tRTSEC for last barrier = %f seconds\n", RealTime );

	free(private);
	free(idx);
  xbrtime_free( shared );
  xbrtime_close();

#ifdef DEBUG
  printf( "xBGAS is Closed\n" );
#endif

  printf("[M]"GRN " Returning Main matmul...\n"RESET);
  return rtn;
}

/* EOF */
