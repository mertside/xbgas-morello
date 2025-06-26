/*
 * ptr_injection.c
 * by Mert Side (TTU)
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "xbrtime_morello.h"

const char hello[] = "Hello World!";

// Thread function to execute.
void *thread_func(void *arg) {
    // Since all threads share the same memory, 
    //   we can directly print the hello message.
    printf("received %s\n", (char *)arg);
    return NULL;
}

int main(void) {
    xbrtime_init();

    pthread_t thread; // Define thread variable.
    int num_pes = xbrtime_num_pes();

    printf("starting pthread_create...\n");
    // Creating a thread. Pass 'hello' as argument to the thread function.
    if (pthread_create(&thread, NULL, thread_func, (void *)hello) != 0) {
        perror("pthread_create");
        exit(1);
    }

    printf("starting pthread_join...\n");
    // Wait for the thread to finish.
    if (pthread_join(thread, NULL) != 0) {
        perror("pthread_join");
        exit(1);
    }

    return 0;
}
