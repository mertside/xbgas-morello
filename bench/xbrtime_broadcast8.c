/*
 * _BROADCAST_DEMO_C_
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
  // Real time recording
  double RealTimeInit, RealTimeBar1, RealTimeBar2, RealTimeBro, RealTimeClose;

  RealTimeInit = -RTSEC(); // Begin timed section
  xbrtime_init();
  RealTimeInit += RTSEC(); // End timed section

	int my_pe, *b_val;
	my_pe = xbrtime_mype();
	b_val = (int*) xbrtime_malloc(sizeof(int));
	*b_val = my_pe;

	printf("Pre-Broadcast - PE:%d B_Val: %d\n", my_pe, *b_val);

  RealTimeBar1 = -RTSEC(); // Begin timed section
	xbrtime_barrier();
  RealTimeBar1 += RTSEC(); // End timed section

  RealTimeBro = -RTSEC(); // Begin timed section
	xbrtime_int_broadcast(b_val, b_val, 1, 1, 4);
  RealTimeBro += RTSEC(); // End timed section

  RealTimeBar2 = -RTSEC(); // Begin timed section
	xbrtime_barrier();
  RealTimeBar2 += RTSEC(); // End timed section

	printf("Post-Broadcast - PE:%d B_Val: %d\n", my_pe, *b_val);

  RealTimeClose = -RTSEC(); // Begin timed section
	xbrtime_close();
  RealTimeClose += RTSEC(); // End timed section

  printf("\tInit Time:      %f\n", RealTimeInit);
  printf("\tBarrier 1 Time: %f\n", RealTimeBar1);
  printf("\tBroadcast Time: %f\n", RealTimeBro);
  printf("\tBarrier 2 Time: %f\n", RealTimeBar2);
  printf("\tClose Time:     %f\n", RealTimeClose);

  return 0;
}
