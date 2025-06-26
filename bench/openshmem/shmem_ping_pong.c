#include <shmem.h>
#define MSG_SIZE 1024

int main() {
    shmem_init();
    int pe = shmem_my_pe();
    int n_pes = shmem_n_pes();
    
    static long src[MSG_SIZE], dest[MSG_SIZE];
    
    for (int i = 0; i < MSG_SIZE; i++) {
        src[i] = pe;  // Initialize with PE ID
    }
    
    if (pe == 0) {
        shmem_put(dest, src, MSG_SIZE, 1);  // Send data to PE 1
        shmem_quiet();                      // Ensure completion
        printf("PE 0 sent data to PE 1\n");
    } else if (pe == 1) {
        shmem_barrier_all();                // Wait for data
        printf("PE 1 received data: %ld\n", dest[0]);
    }
    
    shmem_finalize();
    return 0;
}
