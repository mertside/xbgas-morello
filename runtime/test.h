/*
 * test.h
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
 * \file test.h
 * \brief Testing utilities and benchmarking helpers for xBGAS Runtime
 *
 * This header provides utilities for testing, benchmarking, and debugging
 * the xBGAS runtime implementation, including timing functions and colored
 * output for better visualization of test results.
 */

#ifndef _XBGAS_TEST_H_
#define _XBGAS_TEST_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/*                           SYSTEM INCLUDES                                */
/* ========================================================================= */

#include <sys/time.h>
#include <stdio.h>
#include <stdint.h>

/* ========================================================================= */
/*                           COLOR DEFINITIONS                              */
/* ========================================================================= */

/* Basic Colors */
#define RED     "\x1B[31m"      /*!< Red text */
#define GRN     "\x1B[32m"      /*!< Green text */
#define YEL     "\x1B[33m"      /*!< Yellow text */
#define BLU     "\x1B[34m"      /*!< Blue text */
#define MAG     "\x1B[35m"      /*!< Magenta text */
#define CYN     "\x1B[36m"      /*!< Cyan text */
#define WHT     "\x1B[37m"      /*!< White text */
#define RESET   "\x1B[0m"       /*!< Reset color */

/* Bold Colors */
#define BBLACK  "\033[1m\033[30m"   /*!< Bold Black */
#define BRED    "\033[1m\033[31m"   /*!< Bold Red */
#define BGRN    "\033[1m\033[32m"   /*!< Bold Green */
#define BYEL    "\033[1m\033[33m"   /*!< Bold Yellow */
#define BBLU    "\033[1m\033[34m"   /*!< Bold Blue */
#define BMAG    "\033[1m\033[35m"   /*!< Bold Magenta */
#define BCYN    "\033[1m\033[36m"   /*!< Bold Cyan */
#define BWHT    "\033[1m\033[37m"   /*!< Bold White */

/* ========================================================================= */
/*                           TEST CONFIGURATION                             */
/* ========================================================================= */

/** \brief Default allocation size for test operations */
#define _XBGAS_ALLOC_SIZE_ 8

/** \brief Default number of elements for test operations */
#define _XBGAS_ALLOC_NELEMS_ 4

/* ========================================================================= */
/*                           FUNCTION DECLARATIONS                          */
/* ========================================================================= */

/*!
 * \brief Get current time in seconds with microsecond precision
 * \return Current time as a double precision floating point number
 *
 * This function provides high-precision timing for benchmarking purposes.
 * It returns the current time in seconds with microsecond resolution.
 */
static inline double mysecond(void) {
    struct timeval tp;
    struct timezone tzp;
    
    gettimeofday(&tp, &tzp);
    return ((double)tp.tv_sec + (double)tp.tv_usec * 1.0e-6);
}

/*!
 * \brief Print comprehensive performance statistics
 * \param local Number of local memory accesses
 * \param remote Number of remote memory accesses  
 * \param t_init Time spent in initialization (seconds)
 * \param t_mem Time spent in memory operations (seconds)
 *
 * This function displays a detailed breakdown of memory access patterns
 * and timing information, including a visual representation of the
 * distribution between local and remote accesses.
 */
static inline void PRINT(double local, double remote, double t_init, double t_mem) {
    /* Calculate statistics */
    size_t ne = _XBGAS_ALLOC_NELEMS_;
    int64_t percent = (int64_t)(100 * remote / ne);
    
    /* Display timing information */
    printf("Time.init       = %f sec\n", t_init);
    printf("Time.transfer   = %f sec\n", t_mem);
    
    /* Display access pattern statistics */
    printf("Remote Access   = " BRED "%.3f%%  " RESET "\n", 100 * remote / ne);
    printf("Local  Access   = " BGRN "%.3f%%  " RESET "\n", 100 * local / ne);
    
    /* Display visual distribution */
    printf("------------------------------------------\n");
    printf("Request Distribution:  [");
    
    /* Red bars for remote accesses */
    for (int64_t i = 0; i < percent; i++) {
        printf(BRED "|" RESET);
    }
    
    /* Green bars for local accesses */
    for (int64_t i = 0; i < 100 - percent; i++) {
        printf(BGRN "|" RESET);
    }
    
    printf("]\n");
    printf("------------------------------------------\n");
}

#ifdef __cplusplus
}
#endif

#endif /* _XBGAS_TEST_H_ */

/* EOF */
