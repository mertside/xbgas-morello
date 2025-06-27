/*
 * xbrtime_common.h
 *
 * Copyright (C) 2017-2018 Tactical Computing Laboratories, LLC
 * Copyright (C) 2024 Texas Tech University (Morello adaptation)
 * All Rights Reserved
 * contact@tactcomplabs.com
 *
 * This file is a part of the XBGAS-RUNTIME package.  For license
 * information, see the LICENSE file in the top level directory
 * of the distribution.
 */

/*!
 * \file xbrtime_common.h
 * \brief Common definitions and includes for xBGAS Runtime on CHERI-Morello
 * 
 * This header contains common definitions, macros, and includes used throughout
 * the xBGAS runtime implementation adapted for CHERI-Morello architecture.
 */

#ifndef _XBRTIME_COMMON_H_
#define _XBRTIME_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/*                           SYSTEM INCLUDES                                */
/* ========================================================================= */

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

/* ========================================================================= */
/*                           CHERI INCLUDES                                 */
/* ========================================================================= */

#include <cheri.h>

/* ========================================================================= */
/*                           PROJECT INCLUDES                               */
/* ========================================================================= */

#include "xbMrtime-types.h"
#include "xbMrtime-alloc.h"
#include "xbMrtime-macros.h"
#include "threadpool.h"

/* ========================================================================= */
/*                           CONFIGURATION MACROS                           */
/* ========================================================================= */

/** \brief Enable debug output */
// #define XBGAS_DEBUG 1

/** \brief Enable verbose print statements */
// #define XBGAS_PRINT 1

/** \brief Enable experimental features A */
#define EXPERIMENTAL_A 1

/** \brief Enable experimental features B */
#define EXPERIMENTAL_B 1

/** \brief Maximum number of threads supported */
#define MAX_NUM_OF_THREADS 16

/** \brief Initialization address for xBGAS */
#define INIT_ADDR 0xBB00000000000000ull

/** \brief End address for xBGAS */
#define END_ADDR 0xAA00000000000000ull

/* ========================================================================= */
/*                           GLOBAL VARIABLES                               */
/* ========================================================================= */

/** \brief Global barrier for synchronization */
extern volatile uint64_t *xb_barrier;

/** \brief Global thread pool */
extern volatile tpool_thread_t *threads;

/* ========================================================================= */
/*                           THREAD SYNCHRONIZATION                         */
/* ========================================================================= */

#ifdef EXPERIMENTAL_B
/** \brief Mutex for barrier synchronization */
extern pthread_mutex_t barrier_mutex;

/** \brief Condition variable for barrier synchronization */
extern pthread_cond_t barrier_cond;

/** \brief Counter for threads reaching barrier */
extern int counter;
#endif

/** \brief Mutex for update operations */
extern pthread_mutex_t update_mutex;

/** \brief Condition variable for update operations */
extern pthread_cond_t update_cond;

#ifdef __cplusplus
}
#endif

#endif /* _XBRTIME_COMMON_H_ */

/* EOF */
