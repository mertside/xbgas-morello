/* 
 * ptr_rvk_tmprl_cntrl.c
 * 
 * by mert side 
 * 
 * An indirect control flow vulnerability through aliased heap objects. 
 *   Goal is to demonstrate CheriBSD's *pointer revocation* facility 
 *   and its use by the system `malloc`.
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/* Ensure we're being run on a temporal-safety-aware system */
#ifdef __CHERI_PURE_CAPABILITY__
#include <cheri/revoke.h>
__attribute__((used))
static void *check_cheri_revoke = cheri_revoke;

extern void malloc_revoke(void);
__attribute__((used))
static void *check_malloc_revoke = malloc_revoke;
#endif

static void fn1(uintptr_t arg) {
    fprintf(stderr, " First function: %#p\n", (void *)arg);
}

static void fn2(uintptr_t arg) {
    fprintf(stderr, " Second function: %#p\n", (void *)arg);
}

struct obj {
    char buf[32];
    void (* volatile fn)(uintptr_t);
    volatile uintptr_t arg;
};

void *thread_func(void *vargp) {
    struct obj * volatile obj = calloc(1, sizeof(*obj));
    fprintf(stderr, "Installing function pointer in obj at %#p\n", obj);
    obj->fn = fn1;
    obj->arg = (uintptr_t)obj;

    free(obj);

    fprintf(stderr, "Demonstrating use after free in thread:\n");
    obj->fn(obj->arg);

#ifdef CAPREVOKE
    /* Force recycling the free queue now, but with a revocation pass */
    malloc_revoke();
#endif

    struct obj * volatile obj2 = malloc(sizeof(*obj2));
#ifdef CAPREVOKE
    assert(obj == obj2);
#endif

    fprintf(stderr, "Assigning function pointer through obj2 at %#p\n", obj2);
    obj2->fn = fn2;

    fprintf(stderr, "Calling function pointer through obj (now %#p):\n", obj);
    obj->fn(obj->arg);

    return NULL;
}

int main(void) {
    pthread_t thread_id1, thread_id2;

    // Creating two threads, simulating PEs
    pthread_create(&thread_id1, NULL, thread_func, NULL);
    pthread_create(&thread_id2, NULL, thread_func, NULL);

    // Waiting for threads to complete
    pthread_join(thread_id1, NULL);
    pthread_join(thread_id2, NULL);

    return 0;
}
