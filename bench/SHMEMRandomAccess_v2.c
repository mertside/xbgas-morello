#include <stdio.h>
#include <stdlib.h>
#include "xbrtime_morello.h"
#include <sys/time.h>

#define TABLE_SIZE (1 << 24)  // Example: 16M elements
#define NUM_UPDATES (1 << 26) // Example: 64M updates

typedef struct {
    int64_t *table;
    int64_t remote_index;
    int target_pe;
} work_t;

// Timer function
static double RTSEC() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec + tp.tv_usec / (double)1.0e6;
}

// Worker function for thread pool
void update_remote_value(void *arg) {
    work_t *work = (work_t *)arg;
    int64_t remote_value = 0;

    // Debugging print
    printf("PE %d updating index %ld on PE %d\n", xbrtime_mype(), work->remote_index, work->target_pe);
    fflush(stdout);

    // Fetch, modify, and write back
    xbrtime_longlong_get(&remote_value, &work->table[work->remote_index], 1, 0, work->target_pe);
    remote_value += 1;
    xbrtime_longlong_put(&work->table[work->remote_index], &remote_value, 1, 0, work->target_pe);

    free(work);
}

int main(int argc, char *argv[]) {
    xbrtime_init();
    int me = xbrtime_mype();
    int npes = xbrtime_num_pes();

    // Allocate symmetric shared memory table
    int64_t *table = (int64_t *) xbrtime_malloc(TABLE_SIZE * sizeof(int64_t));
    if (!table) {
        fprintf(stderr, "PE %d: Failed to allocate memory\n", me);
        xbrtime_close();
        return EXIT_FAILURE;
    }

    // Initialize table
    for (size_t i = 0; i < TABLE_SIZE; i++) {
        table[i] = 0;
    }
    xbrtime_barrier();

    // Start benchmark
    double start_time = RTSEC();

    // Ensure thread pools are correctly initialized
    for (int currentPE = 0; currentPE < npes; currentPE++) {
        for (size_t i = 0; i < NUM_UPDATES / npes; i++) {
            // Generate a random index
            int64_t index = (rand() % TABLE_SIZE);
            int target_pe = index % npes;
            int64_t remote_index = index / npes;

            // Bounds check for capability safety
            if (remote_index >= TABLE_SIZE / npes) {
                fprintf(stderr, "PE %d: Invalid remote index: %ld\n", me, remote_index);
                continue;
            }

            // Allocate work safely
            work_t *work = malloc(sizeof(work_t));
            if (!work) {
                fprintf(stderr, "PE %d: Memory allocation failed!\n", me);
                continue;
            }

            work->table = table;
            work->remote_index = remote_index;
            work->target_pe = target_pe;

            // Submit task to the correct PE's thread queue
            tpool_add_work(threads[currentPE].thread_queue, update_remote_value, work);
        }
    }

    // Wait for all tasks before measuring GUPS
    for (int currentPE = 0; currentPE < npes; currentPE++) {
        tpool_wait(threads[currentPE].thread_queue);
    }

    xbrtime_barrier();
    double end_time = RTSEC();

    // Compute and print GUPS
    double elapsed_time = end_time - start_time;
    double updates_per_second = (double)NUM_UPDATES / elapsed_time;
    double gups = updates_per_second / 1e9;

    if (me == 0) {
        printf("GUPS: %lf\n", gups);
    }

    // Cleanup
    printf("PE %d freeing memory...\n", me);
    fflush(stdout);
    xbrtime_free(table);
    printf("PE %d memory freed!\n", me);
    fflush(stdout);

    xbrtime_close();
    return EXIT_SUCCESS;
}