/*
 * xbrtime_internal.h
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
 * \file xbrtime_internal.h
 * \brief Internal data structures and types for xBGAS Runtime
 *
 * This header contains internal data structures, types, and definitions
 * used within the xBGAS runtime implementation. These are not part of
 * the public API and should not be used directly by applications.
 */

#ifndef _XBRTIME_INTERNAL_H_
#define _XBRTIME_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/*                           INCLUDES                                       */
/* ========================================================================= */

#include "xbrtime_common.h"

/* ========================================================================= */
/*                           INTERNAL DATA STRUCTURES                       */
/* ========================================================================= */

/*!
 * \struct XBRTIME_DATA
 * \brief Main configuration data structure for the xBGAS runtime
 *
 * This structure holds all the configuration and state information
 * for the xBGAS runtime system, including PE mappings, memory
 * management data, and synchronization primitives.
 */
typedef struct {
    uint64_t _ID;                    /*!< Current PE identifier */
    uint64_t _MEMSIZE;              /*!< Available memory size */
    int _NPES;                      /*!< Number of processing elements */
    uint64_t _START_ADDR;           /*!< Start address for memory allocation */
    uint64_t _SENSE;                /*!< Barrier sense variable */
    volatile uint64_t *_BARRIER;    /*!< Barrier synchronization array */
    XBRTIME_PE_MAP *_MAP;           /*!< PE mapping information */
    XBRTIME_MEM_T *_MMAP;           /*!< Memory allocation tracking */
} XBRTIME_DATA;

/* ========================================================================= */
/*                           GLOBAL RUNTIME STATE                           */
/* ========================================================================= */

/** \brief Global runtime configuration */
extern XBRTIME_DATA *__XBRTIME_CONFIG;

/* ========================================================================= */
/*                           BROADCAST TASK STRUCTURES                      */
/* ========================================================================= */

/*!
 * \struct BroadcastTaskArgs
 * \brief Arguments for integer broadcast tasks
 */
typedef struct {
    int *src;           /*!< Source data pointer */
    int *dest;          /*!< Destination data pointer */
    int root_pe;        /*!< Root processing element ID */
} BroadcastTaskArgs;

/*!
 * \struct LongLongBroadcastTaskArgs
 * \brief Arguments for long long broadcast tasks
 */
typedef struct {
    long long *src;     /*!< Source data pointer */
    long long *dest;    /*!< Destination data pointer */
    int root_pe;        /*!< Root processing element ID */
} LongLongBroadcastTaskArgs;

/* ========================================================================= */
/*                           REDUCTION TASK STRUCTURES                      */
/* ========================================================================= */

/*!
 * \struct ReduceTaskArgs
 * \brief Arguments for integer reduction tasks
 */
typedef struct {
    const int *src;     /*!< Source array */
    int *dest;          /*!< Destination array */
    int start;          /*!< Starting index for this task */
    int end;            /*!< Ending index (exclusive) for this task */
} ReduceTaskArgs;

/*!
 * \struct LongLongReduceTaskArgs
 * \brief Arguments for long long reduction tasks
 */
typedef struct {
    long long *src;     /*!< Source array */
    long long *dest;    /*!< Destination array */
    int start;          /*!< Starting index for this task */
    int end;            /*!< Ending index (exclusive) for this task */
} LongLongReduceTaskArgs;

/* ========================================================================= */
/*                           INTERNAL FUNCTION PROTOTYPES                   */
/* ========================================================================= */

/*!
 * \brief Constructor function called at library load
 */
void __xbrtime_ctor(void);

/*!
 * \brief Destructor function called at library unload
 */
void __xbrtime_dtor(void);

/* ========================================================================= */
/*                           ASSEMBLY FUNCTION PROTOTYPES                   */
/* ========================================================================= */

/*!
 * \brief Get memory size from assembly layer
 * \return Memory size in bytes
 */
extern size_t __xbrtime_asm_get_memsize(void);

/*!
 * \brief Get PE ID from assembly layer
 * \return Processing element identifier
 */
extern int __xbrtime_asm_get_id(void);

/*!
 * \brief Get number of PEs from assembly layer
 * \return Number of processing elements
 */
extern int __xbrtime_asm_get_npes(void);

/*!
 * \brief Get start address from assembly layer
 * \return Start address for memory operations
 */
extern uint64_t __xbrtime_asm_get_startaddr(void);

/*!
 * \brief Perform memory fence operation
 */
extern void __xbrtime_asm_fence(void);

/*!
 * \brief Perform quiet memory fence operation
 */
extern void __xbrtime_asm_quiet_fence(void);

/* ========================================================================= */
/*                           TRANSFER FUNCTION PROTOTYPES                   */
/* ========================================================================= */

/*!
 * \brief Sequential get operation for unsigned 8-byte data
 * \param base_src Source base address
 * \param base_dest Destination base address
 * \param nelems Number of elements
 * \param stride Stride in bytes
 */
extern void __xbrtime_get_u8_seq(uint64_t *base_src, uint64_t *base_dest,
                                  uint32_t nelems, uint32_t stride);

/*!
 * \brief Sequential get operation for signed 8-byte data
 * \param base_src Source base address
 * \param base_dest Destination base address
 * \param nelems Number of elements
 * \param stride Stride in bytes
 */
extern void __xbrtime_get_s8_seq(uint64_t *base_src, uint64_t *base_dest,
                                  uint32_t nelems, uint32_t stride);

/*!
 * \brief Sequential put operation for signed 8-byte data
 * \param base_src Source base address
 * \param base_dest Destination base address
 * \param nelems Number of elements
 * \param stride Stride in bytes
 */
extern void __xbrtime_put_s8_seq(uint64_t *base_src, uint64_t *base_dest,
                                  uint32_t nelems, uint32_t stride);

/*!
 * \brief Sequential get operation for signed 4-byte data
 * \param base_src Source base address
 * \param base_dest Destination base address
 * \param nelems Number of elements
 * \param stride Stride in bytes
 */
extern void __xbrtime_get_s4_seq(uint64_t *base_src, uint64_t *base_dest,
                                  uint32_t nelems, uint32_t stride);

/*!
 * \brief Sequential put operation for signed 4-byte data
 * \param base_src Source base address
 * \param base_dest Destination base address
 * \param nelems Number of elements
 * \param stride Stride in bytes
 */
extern void __xbrtime_put_s4_seq(uint64_t *base_src, uint64_t *base_dest,
                                  uint32_t nelems, uint32_t stride);

/* ========================================================================= */
/*                           TASK FUNCTION PROTOTYPES                       */
/* ========================================================================= */

/*!
 * \brief Task function for integer broadcast operations
 * \param arg Pointer to BroadcastTaskArgs structure
 */
extern void broadcast_task(void *arg);

/*!
 * \brief Task function for long long broadcast operations
 * \param arg Pointer to LongLongBroadcastTaskArgs structure
 */
extern void longlong_broadcast_task(void *arg);

/*!
 * \brief Task function for integer reduction operations
 * \param arg Pointer to ReduceTaskArgs structure
 */
extern void reduction_task(void *arg);

/*!
 * \brief Task function for long long reduction operations
 * \param arg Pointer to LongLongReduceTaskArgs structure
 */
extern void longlong_reduction_task(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* _XBRTIME_INTERNAL_H_ */

/* EOF */
