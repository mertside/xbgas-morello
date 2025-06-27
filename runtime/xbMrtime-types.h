/*
 * xbMrtime-types.h
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
 * \file xbMrtime-types.h
 * \brief Type definitions for xBGAS Runtime on CHERI-Morello
 * 
 * This header contains all type definitions and data structures used
 * throughout the xBGAS runtime implementation.
 */

#ifndef _XBRTIME_TYPES_H_
#define _XBRTIME_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/*                           SYSTEM INCLUDES                                */
/* ========================================================================= */

#include <stdint.h>
#include "xbMrtime-alloc.h"

/* ========================================================================= */
/*                           CONSTANTS                                      */
/* ========================================================================= */

/** \brief Maximum number of processing elements supported */
#define __XBRTIME_MAX_PE 1024

/* ========================================================================= */
/*                           TYPE DEFINITIONS                               */
/* ========================================================================= */

/*!
 * \struct XBRTIME_PE_MAP
 * \brief Processing Element (PE) mapping structure
 *
 * This structure defines the mapping between logical and physical processing
 * elements in the xBGAS runtime system. Each PE has a logical identifier,
 * a physical identifier, and a base address for memory operations.
 */
typedef struct {
    int _LOGICAL;           /*!< Logical PE identifier */
    int _PHYSICAL;          /*!< Physical PE identifier */
    uint64_t _BASE;         /*!< Base physical address for this PE */
} XBRTIME_PE_MAP;

/*! \struct XBRTIME_DATA
 *  \brief Defines the internal xbrtime configuration data
 * Defines the internal xbrtime configuration data that includes
 * the respective paramters of the parallel environment and the
 * contiguous memory regions
 *
 */
typedef struct{
  size_t    _MEMSIZE;     /*! XBRTIME_DATA: Size of the shared memory region (in bytes) */
  int       _ID;          /*! XBRTIME_DATA: Local node ID */
  int       _NPES;        /*! XBRTIME_DATA: Number of parallel elements */
  uint64_t  _START_ADDR;  /*! XBRTIME_DATA: Starting address of the shared memory region */
  uint64_t  _SENSE;       /*! XBRTIME_DATA: Sense of the barrier sync stage */
  volatile uint64_t*  _BARRIER;  /*! XBRTIME_DATA: Barrier value */
  //uint64_t  _BARRIER[2];  /*! XBRTIME_DATA: Barrier value */
  XBRTIME_MEM_T *_MMAP;   /*! XBRTIME_DATA: Allocated memory map */
  XBRTIME_PE_MAP *_MAP;   /*! XBRTIME_DATA: PE Mappings */
}XBRTIME_DATA;

extern XBRTIME_DATA *__XBRTIME_CONFIG;

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* _XBRTIME_TYPES_H_ */

/* EOF */
