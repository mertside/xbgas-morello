/* _xBGAS_matmul.c__
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

#define _XBGAS_ALLOC_SIZE_ 8
//#define _XBGAS_ALLOC_NELEMS_ 4

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
  uint64_t 	i   			= 0;
  uint64_t  j         = 0;

  int row = 0;
  int col = 0;
  
  uint64_t 	*private  = NULL;
	uint64_t 	*shared  	= NULL;
	
  /* statistic gathering var */
	double 		t_mem	  	= 0;
	double 		t_start  	= 0;
	double 		t_end  		= 0;

  double RealTime;  // Real time recording

  /*
  unsigned long long x = 11;
  unsigned long long y = 22;
  printf("=================================================================\n");
  printf("  X:\n"
      //  "address: %p\n"
         "\tbase  : %p\n"
         "\tlength: %lu\t"
         "\toffset: %lu\n"
         "\tperms : %lu\t"
         "\ttag   : %lu\n",
      // cheri_address_get(dest),
        cheri_base_get(&x),
        cheri_length_get(&x),
        cheri_offset_get(&x),
        cheri_perms_get(&x),
        cheri_tag_get(&x));
  printf("=================================================================\n");
  printf("  Y:\n"
      // "address: %p\n"
          "\tbase  : %p\n"
          "\tlength: %lu\t"
          "\toffset: %lu\n"
          "\tperms : %lu\t"
          "\ttag   : %lu\n",
      // cheri_address_get(dest),
        cheri_base_get(&y),
        cheri_length_get(&y),
        cheri_offset_get(&y),
        cheri_perms_get(&y),
        cheri_tag_get(&y));
  printf("=================================================================\n");
  */

	printf("[M]"GRN " Passed vars\n"RESET);

	/* init */
  RealTime = -RTSEC(); // Begin timed section
  rtn = xbrtime_init();
  RealTime += RTSEC(); // End timed section
  printf("[M]"GRN " Passed xbrtime_init()\n"RESET);
  printf("\tRTSEC used fior INIT = %.6f seconds\n", RealTime );

  int num_pes = xbrtime_num_pes();
  row         = num_pes;
  col         = ne;
  // int *arr = malloc(N*M*sizeof(int));
  // arr[i*M + j]

	private = malloc( row * col * sizeof( uint64_t ));
	printf("[M]"GRN " Passed malloc\n"RESET);

  //rtn = xbrtime_init();
	//printf("[M]"GRN " Passed xbrtime_init()\n"RESET); 

  shared = (uint64_t *)(xbrtime_malloc( row * col * sizeof( uint64_t ) ));
	printf("[M]"GRN " Passed xbrtime_malloc()\n"RESET); 

#ifdef DEBUG
  printf("PE=%d; *SHARED = 0x%"PRIu64"\n", xbrtime_mype(), (uint64_t)(shared));
#endif
  for( i = 0; i < row; i++ ){
 	  for( j = 0; j < col; j++ ){
		  shared[i*col + j] 	= (uint64_t)(j + xbrtime_mype());
		  private[i*col + j] 	= 1;
    }
  }

	printf("[M]"GRN " Passed shared[] & private[] init\n"RESET);

  /* perform a barrier */
#ifdef DEBUG
  printf( "PE=%d; EXECUTING BARRIER\n", xbrtime_mype() );
#endif
  xbrtime_barrier();

	if(xbrtime_mype() == 0){
		printf("========================\n");
		printf(" xBGAS Matmul Benchmark\n");
		printf("========================\n");
		printf(" Data type: uint64_t\n");
		printf(" Element #: %lu\n", ne);
  	printf(" Data size: %lu bytes\n",  (uint64_t)(sz) * (uint64_t)(ne) );
		printf(" PE #     : %d\n", xbrtime_num_pes());
    printf("========================\n");

		t_start = mysecond();
	}

  /* fetch via loop */
  RealTime = -RTSEC(); // Begin timed section
  if(xbrtime_mype() == 0){
    //xbrtime_ulonglong_get(&x,&y,1,1,1);
    for( i = 0; i < row; i++ ){
      for( j = 0; j < col; j++ ){
        // remote access
        void* func_args = {(unsigned long long *)(&(shared[i*col + j])),
                           (unsigned long long *)(&(shared[i*col + j])),
                           1, 1, 1};                                 

        //tpool_add_work(pool, xbrtime_ulonglong_get, func_args);
        bool check = false;
        check = tpool_add_work( threads[i].thread_queue, 
                        xbrtime_ulonglong_get, 
                        func_args);
        

        /*
        xbrtime_ulonglong_get((unsigned long long *)(&(shared[i])), // dest
                              (unsigned long long *)(&(shared[i])), // src
                              1,                                    // ne
                              1,                                    // stride
                              1);                                   // pe
        */
        // shared[i] = i;
        fflush(stdout);
        fprintf(stdout,"[M] "BYEL"Iter:\ti:%lu, j:%lu \t%s\n    Thread:\t%lu\n"
              RESET, i, j, check ? "true" : "false", threads[i].thread_handle);
        fflush(stdout);
      }
    }
    printf("[M] "BGRN"Passed xbrtime_ulonglong_get()\n"RESET);
  }
  RealTime += RTSEC(); // End timed section

  printf("\tRTSEC for xbrtime_ulonglong_get = %.6f seconds\n", RealTime );

  /* perform a barrier */
#ifdef DEBUG
  printf( "PE=%d; EXECUTING BARRIER\n", xbrtime_mype() );
#endif
  RealTime = -RTSEC(); // Begin timed section
  xbrtime_barrier();
  RealTime += RTSEC(); // End timed section
  printf("\tRTSEC for xbrtime_barrier = %f seconds\n", RealTime );

	if(xbrtime_mype() == 0){
		t_end = mysecond();
		t_mem = t_end - t_start;
		printf("--------------------------------------------\n");
		printf("Time cost"BRED	" (raw transactions):       %f\n"RESET, t_mem);
		printf("--------------------------------------------\n");
		t_start = mysecond();
	}

 	// if(xbrtime_mype() == 0){
	// 	// remote access
  //   xbrtime_ulonglong_get((unsigned long long *)(&(shared[0])), // dest
  //                         (unsigned long long *)(&(shared[0])), // src
  //                         ne,																	 // ne
  //                         1,																		 // stride
  //                         1);									 								 // pe
	// }

  RealTime = -RTSEC(); // Begin timed section
  xbrtime_barrier();
  RealTime += RTSEC(); // End timed section
  printf("\tRTSEC for xbrtime_barrier = %f seconds\n", RealTime );

	if(xbrtime_mype() == 0){
		t_end = mysecond();
		t_mem = t_end - t_start;
		// printf("Time cost"BGRN " (coalesced transactions): %f\n"RESET, t_mem);
		printf("--------------------------------------------\n");
		t_start = mysecond();
	}

  for( i = 0; i < row; i++ ){
    for( j = 0; j < col; j++ ){
		  private[i*col + j] *= shared[i*col + j];        // matrix multiplication
    }
  }

	if(xbrtime_mype() == 0){
		t_end = mysecond();
		t_mem = t_end - t_start;
		printf("Time cost"BMAG " (matrix multiplication):  %f\n"RESET, t_mem);
		printf("--------------------------------------------\n");
	}

	free(private);
  xbrtime_free( shared );
  xbrtime_close();

#ifdef DEBUG
  printf( "xBGAS is Closed\n" );
#endif

  sleep(1);
	printf("[M]"GRN " Returning Main matmul...\n"RESET);
  return rtn;
}

/* EOF */
