/*
 * xbMrtime-macros.h
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
 * \file xbMrtime-macros.h
 * \brief Macro definitions for xBGAS Runtime on CHERI-Morello
 *
 * This header contains macro definitions used throughout the xBGAS runtime
 * implementation to configure performance parameters and operational modes.
 */

#ifndef _XBRTIME_MACROS_H_
#define _XBRTIME_MACROS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/*                           PERFORMANCE MACROS                             */
/* ========================================================================= */

#ifndef _XBRTIME_MIN_UNR_THRESHOLD_
/** 
 * \brief Minimum transfer unrolling threshold (to hide latency)
 * 
 * This defines the minimum number of elements required before the runtime
 * will attempt to unroll transfer operations for performance optimization.
 */
#define _XBRTIME_MIN_UNR_THRESHOLD_ 8
#endif

/** 
 * \brief Unrolling factor for unrolled transfer API functions
 * 
 * This controls how many operations are unrolled in a single iteration
 * to improve memory transfer performance by reducing loop overhead.
 */
#define _XBRTIME_UNROLL_FACTOR_ 4

/* ========================================================================= */
/*                           MEMORY CONFIGURATION                           */
/* ========================================================================= */

/** 
 * \brief Number of memory allocation slots
 * 
 * This defines the maximum number of concurrent memory allocations
 * that the runtime can track simultaneously.
 */
#define _XBRTIME_MEM_SLOTS_ 2048

#ifdef __cplusplus
}
#endif

#endif /* _XBRTIME_MACROS_H_ */

/* EOF */
