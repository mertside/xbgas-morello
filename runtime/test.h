/* _xBGAS_test.h__
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

#include <sys/time.h>

/*color print*/
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"
#define BBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BRED     "\033[1m\033[31m"      /* Bold Red */
#define BGRN   "\033[1m\033[32m"      /* Bold Green */
#define BYEL  "\033[1m\033[33m"      /* Bold Yellow */
#define BBLU    "\033[1m\033[34m"      /* Bold Blue */
#define BMAG "\033[1m\033[35m"      /* Bold Magenta */
#define BCYN    "\033[1m\033[36m"      /* Bold Cyan */
#define BWHT   "\033[1m\033[37m"      /* Bold White */

#define _XBGAS_ALLOC_SIZE_ 8
// #define _XBGAS_ALLOC_NELEMS_ 1024*10
#define _XBGAS_ALLOC_NELEMS_ 4

double mysecond() {
	  struct timeval tp;
		struct timezone tzp;
		int i = 0;
		i = gettimeofday( &tp, &tzp );
		return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}


void PRINT(double local, double remote, double t_init, double t_mem){
	/* vars */
	size_t			ne  				= _XBGAS_ALLOC_NELEMS_;
	int64_t 		i  					= 0;
	int64_t 		percent			= 0;

	/* gather statistics*/
	percent = (int64_t)(100*remote/ne);
	printf("Time.init       = %f sec\n", t_init);	
	printf("Time.transfer   = %f sec\n", t_mem);
	printf("Remote Access   = " BRED "%.3f%%  " RESET, 100*remote/ne);
	printf("\n");
	printf("Local  Access   = " BGRN "%.3f%%  " RESET, 100*local/ne);
	printf("\n");
	printf("------------------------------------------\n");
	printf("Request Distribution:  [");
	for(i = 0; i < percent; i++)
		printf(BRED "|" RESET);
	for(i = 0; i < 100 - percent; i++)
		printf(BGRN "|" RESET);
	printf("]\n");
	/*Latency*/
	printf("------------------------------------------\n");


}
