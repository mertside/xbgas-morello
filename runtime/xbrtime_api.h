/*
 * xbrtime_api.h
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
 * \file xbrtime_api.h
 * \brief xBGAS Runtime API declarations for CHERI-Morello
 *
 * This header provides the public API functions for the xBGAS runtime system
 * adapted for CHERI-Morello architecture. It includes all the function
 * declarations needed by applications using the xBGAS runtime.
 */

#ifndef _XBRTIME_API_H_
#define _XBRTIME_API_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/*                           INCLUDES                                       */
/* ========================================================================= */

#include "xbrtime_common.h"

/* ========================================================================= */
/*                           RUNTIME MANAGEMENT                             */
/* ========================================================================= */

/*!
 * \brief Initialize the xBGAS Runtime environment
 * \return 0 on success, non-zero on error
 *
 * This function must be called before any other xBGAS runtime functions.
 * It initializes the thread pool, memory management, and synchronization
 * primitives required for the runtime system.
 */
extern int xbrtime_init(void);

/*!
 * \brief Close the xBGAS Runtime environment
 *
 * This function performs cleanup of all runtime resources including
 * thread pools, memory allocations, and synchronization primitives.
 * It should be called before program termination.
 */
extern void xbrtime_close(void);

/* ========================================================================= */
/*                           PROCESSING ELEMENT QUERIES                     */
/* ========================================================================= */

/*!
 * \brief Get the logical PE (Processing Element) number of the calling entity
 * \return Logical PE number on success, -1 on error
 *
 * Returns the logical identifier of the current processing element.
 * This is used to identify which PE is executing the current code.
 */
extern int xbrtime_mype(void);

/*!
 * \brief Get the total number of configured PEs
 * \return Total number of PEs on success, -1 on error
 *
 * Returns the total number of processing elements available in the
 * current xBGAS runtime configuration.
 */
extern int xbrtime_num_pes(void);

/* ========================================================================= */
/*                           MEMORY MANAGEMENT                              */
/* ========================================================================= */

/*!
 * \brief Allocate a block of contiguous shared memory
 * \param sz Minimum size of the allocated block in bytes
 * \return Valid pointer on success, NULL on failure
 *
 * Allocates shared memory that can be accessed by all processing elements
 * in the xBGAS system. The memory is guaranteed to be accessible across
 * all PEs in the runtime.
 */
extern void *xbrtime_malloc(size_t sz);

/*!
 * \brief Free a previously allocated memory block
 * \param ptr Pointer to the memory block to free
 *
 * Frees memory that was previously allocated with xbrtime_malloc().
 * The pointer becomes invalid after this call.
 */
extern void xbrtime_free(void *ptr);

/*!
 * \brief Check if an address on the target PE can be reached
 * \param addr Pointer to check for accessibility
 * \param pe Target processing element identifier
 * \return 1 if accessible, 0 otherwise
 *
 * Verifies whether the specified address is accessible from the given
 * processing element. This is useful for validation before performing
 * remote memory operations.
 */
extern int xbrtime_addr_accessible(const void *addr, int pe);

/* ========================================================================= */
/*                           SYNCHRONIZATION                                */
/* ========================================================================= */

/*!
 * \brief Perform a global barrier operation across all configured PEs
 *
 * This function blocks until all processing elements in the system
 * have reached the barrier point. It ensures synchronization across
 * all PEs before continuing execution.
 */
extern void xbrtime_barrier(void);

/* ========================================================================= */
/*                           DATA TRANSFER OPERATIONS                       */
/* ========================================================================= */

/*!
 * \brief Get (read) long long data from remote PE
 * \param dest Destination buffer for the data
 * \param src Source address on the remote PE
 * \param nelems Number of elements to transfer
 * \param stride Stride between elements in the source
 * \param pe Source processing element identifier
 *
 * Reads long long integer data from a remote processing element.
 * The operation is performed element-by-element with the specified stride.
 */
extern void xbrtime_longlong_get(long long *dest, const long long *src, 
                                 size_t nelems, int stride, int pe);

/*!
 * \brief Put (write) long long data to remote PE
 * \param dest Destination address on the remote PE
 * \param src Source buffer containing the data
 * \param nelems Number of elements to transfer
 * \param stride Stride between elements in the destination
 * \param pe Destination processing element identifier
 *
 * Writes long long integer data to a remote processing element.
 * The operation is performed element-by-element with the specified stride.
 */
extern void xbrtime_longlong_put(long long *dest, const long long *src, 
                                 size_t nelems, int stride, int pe);

/*!
 * \brief Get (read) unsigned long long data from remote PE
 * \param dest Destination buffer for the data
 * \param src Source address on the remote PE
 * \param nelems Number of elements to transfer
 * \param stride Stride between elements in the source
 * \param pe Source processing element identifier
 *
 * Reads unsigned long long integer data from a remote processing element.
 */
extern void xbrtime_ulonglong_get(unsigned long long *dest, 
                                  const unsigned long long *src, 
                                  size_t nelems, int stride, int pe);

/*!
 * \brief Get (read) integer data from remote PE
 * \param dest Destination buffer for the data
 * \param src Source address on the remote PE
 * \param nelems Number of elements to transfer
 * \param stride Stride between elements in the source
 * \param pe Source processing element identifier
 *
 * Reads integer data from a remote processing element.
 */
extern void xbrtime_int_get(int *dest, const int *src, 
                            size_t nelems, int stride, int pe);

/*!
 * \brief Put (write) integer data to remote PE
 * \param dest Destination address on the remote PE
 * \param src Source buffer containing the data
 * \param nelems Number of elements to transfer
 * \param stride Stride between elements in the destination
 * \param pe Destination processing element identifier
 *
 * Writes integer data to a remote processing element.
 */
extern void xbrtime_int_put(int *dest, const int *src, 
                            size_t nelems, int stride, int pe);

/* ========================================================================= */
/*                           COLLECTIVE OPERATIONS                          */
/* ========================================================================= */

/*!
 * \brief Broadcast integer data from root PE to all PEs
 * \param dest Destination buffer (all PEs)
 * \param src Source data (root PE only)
 * \param nelems Number of elements to broadcast
 * \param stride Stride between elements
 * \param root_pe Root processing element identifier
 *
 * Broadcasts integer data from the specified root processing element
 * to all other processing elements in the system.
 */
extern void xbrtime_int_broadcast(int *dest, const int *src, 
                                  size_t nelems, int stride, int root_pe);

/*!
 * \brief Broadcast long long data from root PE to all PEs
 * \param dest Destination buffer (all PEs)
 * \param src Source data (root PE only)
 * \param nelems Number of elements to broadcast
 * \param stride Stride between elements
 * \param root_pe Root processing element identifier
 *
 * Broadcasts long long integer data from the specified root processing
 * element to all other processing elements in the system.
 */
extern void xbrtime_longlong_broadcast(long long *dest, const long long *src, 
                                       size_t nelems, int stride, int root_pe);

/*!
 * \brief Perform integer reduction sum across all PEs
 * \param dest Destination for the reduced result
 * \param src Source data for reduction
 * \param nelems Number of elements to reduce
 * \param stride Stride between elements
 * \param pe Processing element identifier
 *
 * Performs a sum reduction operation across all processing elements,
 * combining integer data from all PEs into a single result.
 */
extern void xbrtime_int_reduce_sum(int *dest, const int *src, 
                                   size_t nelems, int stride, int pe);

/*!
 * \brief Perform long long reduction sum across all PEs
 * \param dest Destination for the reduced result
 * \param src Source data for reduction
 * \param nelems Number of elements to reduce
 * \param stride Stride between elements
 * \param pe Processing element identifier
 *
 * Performs a sum reduction operation across all processing elements,
 * combining long long integer data from all PEs into a single result.
 */
extern void xbrtime_longlong_reduce_sum(long long *dest, const long long *src, 
                                        size_t nelems, int stride, int pe);

#ifdef __cplusplus
}
#endif

#endif /* _XBRTIME_API_H_ */

/* EOF */
