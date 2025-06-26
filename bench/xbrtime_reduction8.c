/*
 * _REDUCTION_DEMO_C_
 *
 * Copyright (C) 2017-2019 Tactical Computing Laboratories, LLC
 * All Rights Reserved
 * contact@tactcomplabs.com
 *
 * This file is a part of the XBGAS-RUNTIME package.  For license
 * information, see the LICENSE file in the top level directory
 * of the distribution.
 *
 */

 #include "xbrtime_morello.h"

// For timing
static double RTSEC() {
  struct timeval tp;
  gettimeofday (&tp, NULL);
  return tp.tv_sec + tp.tv_usec/(double)1.0e6;
}

int main()
{
  double RealTimeInit, RealTimeBar1, RealTimeBar2, RealTimeRed, RealTimeClose;

  RealTimeInit = -RTSEC(); // Begin timed section
  xbrtime_init();
  RealTimeInit += RTSEC(); // End timed section

	int i, my_pe, *r_val;
	my_pe = xbrtime_mype();
	r_val = (int*) xbrtime_malloc(10 * sizeof(int));

  for(i = 0; i < 10; i++)
  {
      r_val[i] = 1;
  }

	printf("Pre-Reduction - PE:%d R_Val: %d %d %d %d %d %d %d %d %d %d\n", \
            my_pe, r_val[0], r_val[1], r_val[2], r_val[3], r_val[4], 
            r_val[5], r_val[6], r_val[7], r_val[8], r_val[9]);

  RealTimeBar1 = -RTSEC(); // Begin timed section
	xbrtime_barrier();
  RealTimeBar1 += RTSEC(); // End timed section

  RealTimeRed = -RTSEC(); // Begin timed section
	xbrtime_int_reduce_sum(r_val, r_val, 5, 2, 0);
  RealTimeRed += RTSEC(); // End timed section

  RealTimeBar2 = -RTSEC(); // Begin timed section
	xbrtime_barrier();
  RealTimeBar2 += RTSEC(); // End timed section

	printf("Post-Reduction - PE:%d R_Val: %d %d %d %d %d %d %d %d %d %d\n", \
            my_pe, r_val[0], r_val[1], r_val[2], r_val[3], r_val[4], 
            r_val[5], r_val[6], r_val[7], r_val[8], r_val[9]);

  RealTimeClose = -RTSEC(); // Begin timed section  
	xbrtime_close();
  RealTimeClose += RTSEC(); // End timed section

  printf("\tInit Time:      %f\n", RealTimeInit);
  printf("\tBarrier 1 Time: %f\n", RealTimeBar1);
  printf("\tReduction Time: %f\n", RealTimeRed);
  printf("\tBarrier 2 Time: %f\n", RealTimeBar2);
  printf("\tClose Time:     %f\n", RealTimeClose);

  return 0;
}
