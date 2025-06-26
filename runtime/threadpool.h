/*                                                                              
 * _THREADPOOL_H_                                                       
 *
 * on 2023/08/15                                                       
 *                                                                              
 */   
#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include <stdbool.h>
#include <stddef.h>

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include <pthread.h>
#include <sys/types.h>

//#include "threadpool.h"

// ---------------------------------------------------------------- PROTOTYPES

// Thread struct definition
struct tpool_thread;
typedef struct tpool_thread tpool_thread_t;

// Work unit struct definition
struct tpool_work_unit;
typedef struct tpool_work_unit tpool_work_unit_t;

// Queue struct definition
struct tpool_work_queue;
typedef struct tpool_work_queue tpool_work_queue_t;

// Arguments for the function passed on the work unit
typedef void (*thread_func_t)(void *arg);

// Queue creation prototype
tpool_thread_t *tpool_create(size_t num);

// Queue destruction prototype
void tpool_destroy(tpool_work_queue_t *wq);

// Adds work to the queue for processing. 
bool tpool_add_work(tpool_work_queue_t *wq, thread_func_t func, void *arg);

// Blocks until all work has been completed.
void tpool_wait(tpool_work_queue_t *wq);

// ------------------------------------------------------------------- STRUCTS  
struct tpool_thread{
  uint64_t            thread_id;
  pthread_t           thread_handle;  
  tpool_work_queue_t  *thread_queue;
};

// Simple linked list which stores the function to call and its arguments.      
struct tpool_work_unit {
  thread_func_t           func;
  void                   *arg;
  struct tpool_work_unit *next;
};

struct tpool_work_queue {
  tpool_work_unit_t  *work_head;     // queue head pointer
  tpool_work_unit_t  *work_tail;     // queue tail pointer
  pthread_mutex_t     work_mutex;    // single mutex for all locking
  pthread_cond_t      work_cond;     // signal: there is work to process
  pthread_cond_t      working_cond;  // signal: no threads processing
  size_t              working_cnt;   // number of threads actively working
  size_t              num_threads;   // number of threads alive
  bool                stop;          // stops the threads
};

// ---------------------------------- Simple helper for creating work objects.  
static tpool_work_unit_t *tpool_work_unit_create(thread_func_t func, void *arg)
{
  if (func == NULL)
    return NULL;

  tpool_work_unit_t *work = malloc(sizeof(tpool_work_unit_t));
  
  if (work == NULL)  // Handle malloc failure
    return NULL;

  work->func = func;
  work->arg  = arg;
  work->next = NULL;
  
  return work;
} 
 
// -------------------------------- Simple helper for destroying work objects.  
static void tpool_work_unit_destroy(tpool_work_unit_t *work)                              
{ 
  if (work == NULL) 
    return; 
  free(work); 
} 

// --------------------------------------------------------- GET WORK FUNCTION  
// Handles pulling an object from the list and                                  
//   maintain the list work_head and work_tail references.                      
static tpool_work_unit_t *tpool_work_unit_get(tpool_work_queue_t *wq)
{
  if (wq == NULL || wq->work_head == NULL)
    return NULL;

  tpool_work_unit_t *work = wq->work_head;
  wq->work_head = work->next;

  // If the work_head becomes NULL after the update, then reset the work_tail as well.
  if (wq->work_head == NULL)
    wq->work_tail = NULL;

  return work;
}                                                                             
                                                                                
// ----------------------------------------------------------- WORKER FUNCTION  
// At a high level: this function waits for work and processes it.              
static void *tpool_worker(void *arg) 
{
  tpool_work_queue_t *wq = (tpool_work_queue_t *) arg;
  tpool_work_unit_t *work;

  while (1) {
    pthread_mutex_lock(&(wq->work_mutex));

    // Wait for work to be available or for a stop signal.
    while (wq->work_head == NULL && !wq->stop) {
      pthread_cond_wait(&(wq->work_cond), &(wq->work_mutex));
    }

    // Exit if the stop signal is received.
    if (wq->stop) {
      wq->num_threads--;
      if (wq->num_threads == 0 || wq->working_cnt == 0) {
        pthread_cond_signal(&(wq->working_cond));
      }
      pthread_mutex_unlock(&(wq->work_mutex));
      break;
    }

    // Retrieve work and increase working count.
    work = tpool_work_unit_get(wq);
    wq->working_cnt++;

    pthread_mutex_unlock(&(wq->work_mutex));

    // Process work.
    if (work != NULL) {
      work->func(work->arg);
      tpool_work_unit_destroy(work);
    }

    pthread_mutex_lock(&(wq->work_mutex));

    // Decrement working count and signal if needed.
    wq->working_cnt--;
    if (!wq->stop && wq->working_cnt == 0 && wq->work_head == NULL) {
      pthread_cond_signal(&(wq->working_cond));
    }

    pthread_mutex_unlock(&(wq->work_mutex));
  }

    return NULL;
}

// ------------------------------------------------------ POOL CREATE FUNCTION  
tpool_thread_t *tpool_create(size_t num)
{
  int i;

  // If the specified number of threads is zero, use the default of 2 threads.
  if (num == 0)
    num = 2; 

  // Allocate memory for the thread structures.
  tpool_thread_t *threads = calloc(num, sizeof(*threads));
    
  // Check if memory allocation for threads succeeded.
  if (!threads) {
    perror("Failed to allocate memory for threads");
    return NULL;  // Return early on failure.
  }

  // Loop to initialize thread structures and work queues.
  for (i = 0; i < num; i++) {
    // Assign thread ID
    threads[i].thread_id = i;

#if XBGAS_DEBUG
    // Debug output for thread initialization
    fprintf(stdout, "\tThread #%d is set with ID %lu!", i, threads[i].thread_id);
    if (i % 2 == 1) fprintf(stdout, "\n");
#endif

    // Allocate and initialize a work queue for the current thread.
    tpool_work_queue_t *wq = calloc(1, sizeof(*wq));
        
    // Check if memory allocation for the work queue succeeded.
    if (!wq) {
      perror("Failed to allocate memory for work queue");

      // Clean up previously allocated work queues before returning.
      while (i--)
        free(threads[i].thread_queue);
      free(threads);
      return NULL;  // Return early on failure.
    }

    // Initialize the mutex and condition variables for the work queue.
    // If any of these initializations fail, print an error and clean up.
    if (pthread_mutex_init(&(wq->work_mutex), NULL) ||
      pthread_cond_init(&(wq->work_cond), NULL) ||
      pthread_cond_init(&(wq->working_cond), NULL)) {
      perror("Failed to initialize mutex or cond variable");
            
      // Clean up on failure
      free(wq);
      free(threads);
      return NULL;
    }

    // Set the number of threads in the work queue.
    wq->num_threads = num;
        
    // Associate the thread with its work queue.
    threads[i].thread_queue = wq;
  }

#if XBGAS_DEBUG
  // Debug output for the first thread's handle
  fprintf(stdout, "\tThread #0 has the handle %lu!\n", (uint64_t)pthread_self());
#endif

  // Loop to create the threads.
  for (i = 0; i < num; i++) {
    // Create a new thread with the `tpool_worker` function, 
    //   passing the work queue as its argument.
    if (pthread_create(&threads[i].thread_handle, 
                       NULL, tpool_worker, threads[i].thread_queue)) {
      perror("Failed to create thread");
      // You can add more cleanup or error handling here 
      //   if thread creation fails.
    }
        
    // Detach the thread. After a detached thread terminates, 
    //   its resources are automatically released back to the system.
    pthread_detach(threads[i].thread_handle);
  }

  // Return the array of thread structures.
  return threads;
}



// // ------------------------------------------------------ POOL CREATE FUNCTION  
// tpool_thread_t *tpool_create(size_t num)                                               
// {     
//   int i;                                                                           
//   // The minumum acceptable number of threads is two threads.                   
//   if (num == 0) {
//     num = 2; 
//   }                                                 
//   // XXX:IDEA: May set the number of core/processors + 1 as the default... 
//   // num = num + 1;     

//   // tpool_thread_t threads[num];
//   tpool_thread_t *threads;
//   threads = calloc(num, sizeof(*threads));

//   // initialize thread args
//   for( i=0; i<num; i++ ){
//     pthread_t  temp_thread_handle;
//     threads[i].thread_id     = i;                           // thread id
//     threads[i].thread_handle = temp_thread_handle;          // thread handle
//                             // (uint64_t) pthread_self();
// #if XBGAS_DEBUG
//     fprintf(stdout, "\tThread #%d is set with ID %lu!", 
//             i, threads[i].thread_id);
//     if(i % 2 == 1) fprintf(stdout,"\n");
// #endif
//   }

//   for( i=0; i<num; i++ ){
//     tpool_work_queue_t   *wq;                     // work queue pointer
//     wq = calloc(1, sizeof(*wq));                  // allocate the work queue

//     wq->work_head = NULL;                         // queue head pointer                    
//     wq->work_tail = NULL;                         // queue tail pointer   
  
//     pthread_mutex_init(&(wq->work_mutex), NULL);  // one mutex for all locking               
//     pthread_cond_init(&(wq->work_cond), NULL);    // there is work to process   
//     pthread_cond_init(&(wq->working_cond), NULL); // no threads processing         
  
//     wq->working_cnt = (size_t) 0;                 // #threads actively working  
//     wq->num_threads = num;                        // number of threads alive
  
//     threads[i].thread_queue = wq;                 // thread queue pointer
//   }                                                               

// #if XBGAS_DEBUG
//   fprintf(stdout, "\tThread #0 has the handle %lu!\n", 
//           (uint64_t) pthread_self());
// #endif

//   // The requested threads are started and tpool_worker is specified.           
//   for (i=0; i<num; i++) {                                                       
//     // Start num of threads; tpool_worker is specified as the thread function.  
//     pthread_create(&threads[i].thread_handle,
//                    NULL,
//                    tpool_worker, 
//                    threads[i].thread_queue
//                   );                            
//     // XXX:     Attempted fix via tpool_thread_t
//     // Before:  20230718
//     //      Did NOT store the thread ids; they are not accessed directly.       
//     //      If we wanted to implement some kind of force exit,                  
//     //      instead of having to wait then we’d need to track the ids.          
//     pthread_detach(threads[i].thread_handle);                               
//   }                                                                             
                                                                                
//   return threads;                                                                    
// }                                                                               
                                                                               
// ----------------------------------------------------- POOL DESTROY FUNCTION  
void tpool_destroy(tpool_work_queue_t *wq) 
{
  tpool_work_unit_t *work;
  tpool_work_unit_t *work2;

  if (wq == NULL)
    return;

  // Lock the work queue to safely manipulate it.
  pthread_mutex_lock(&(wq->work_mutex));

  // Throwing away all pending work; caller BEWARE!!!
  work = wq->work_head;
  while (work != NULL) {
    work2 = work->next;
    tpool_work_unit_destroy(work);
    work = work2;
  }

  // Cleaned up the queue; tell the threads they need to stop.
  wq->stop = true;

  // Broadcasting to all waiting threads that they should stop.
  pthread_cond_broadcast(&(wq->work_cond));

  pthread_mutex_unlock(&(wq->work_mutex));

  // Wait for the processing threads to finish.
  tpool_wait(wq);

  // Destroy mutex and condition variables.
  pthread_mutex_destroy(&(wq->work_mutex));
  pthread_cond_destroy(&(wq->work_cond));
  pthread_cond_destroy(&(wq->working_cond));

  free(wq);
} 

// ---
/*enrty func
{
 // asm get ret
 tpool_add_work()

}
*/

// -------------------------------------------------- Adding work to the queue     
bool tpool_add_work(tpool_work_queue_t *wq, thread_func_t func, void *arg)                 
{
  /*
   * calculate number of bytes in func to travel up the stack by 
   *    tpool_work_queue_t *wq, thread_func_t func, void *arg
   * __asm__
   *   get rtrn adrr
   */
  if (wq == NULL)
    return false;

  // Create a work object outside the locked region.
  tpool_work_unit_t *work = tpool_work_unit_create(func, arg);
  if (work == NULL)
    return false;

  pthread_mutex_lock(&(wq->work_mutex));

  // Add the object to the linked list.
  if (wq->work_head == NULL) {
    wq->work_head = wq->work_tail = work;
  } else {
    wq->work_tail->next = work;
    wq->work_tail = work;
  }

  pthread_cond_broadcast(&(wq->work_cond));
  pthread_mutex_unlock(&(wq->work_mutex));

  return true;
}


// ----------------------------------------- Waiting for processing to complete  
void tpool_wait(tpool_work_queue_t *wq)
{
  if (wq == NULL)
    return;

  pthread_mutex_lock(&(wq->work_mutex));
  
  while ((!wq->stop && wq->working_cnt != 0) || (wq->stop && wq->num_threads != 0)) {
    pthread_cond_wait(&(wq->working_cond), &(wq->work_mutex));
  }

  pthread_mutex_unlock(&(wq->work_mutex));
}

// --------------------------------------------------------- POOL FREE FUNCTION
void tpool_unit_free(tpool_work_unit_t *unit) 
{
  if (unit == NULL) {
    return;
  }

  // free(unit->func);
  // free(unit->arg);
  if (unit->next != NULL) {
    tpool_unit_free(unit->next);
  }

  free(unit);
}     
// struct tpool_work_unit { thread_func_t func; void *arg; 
//    struct tpool_work_unit *next; };

// -------------------------------------------------------- QUEUE FREE FUNCTION
void tpool_queue_free(tpool_work_queue_t *queue) 
{
  if (queue == NULL) {
    return;
  }
  
  tpool_unit_free(queue->work_head);
  tpool_unit_free(queue->work_tail);

  // free(queue->work_mutex);
  // free(queue->work_cond);
  // free(queue->working_cond);
  // free(queue->working_cnt);
  // free(queue->num_threads);
  // free(queue->stop);

  free(queue);
}
// struct tpool_work_queue { tpool_work_unit_t *work_head; 
//    tpool_work_unit_t *work_tail; pthread_mutex_t work_mutex;
//    pthread_cond_t work_cond; pthread_cond_t working_cond;
//    size_t working_cnt; size_t num_threads; bool stop; };

// ------------------------------------------------------- THREAD FREE FUNCTION
void tpool_thread_free(tpool_thread_t *pool) 
{
  if (pool == NULL) {
    return;
  }

  // free(pool->thread_id);
  // free(pool->thread_handle);

  tpool_queue_free(pool->thread_queue);

  free(pool);
}
// struct tpool_thread{ uint64_t thread_id; pthread_t thread_handle; 
//    tpool_work_queue_t *thread_queue; };

#endif /* __THREADPOOL_H__ */
