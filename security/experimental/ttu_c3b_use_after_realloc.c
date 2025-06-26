/*  Benchmark: use-after-reallocation
 *  @author  : Mert Side for TTU
 *  @brief   : A use-after-reallocation vulnerability occurs when memory that 
 *             was freed and then reallocated for another purpose is still 
 *             being accessed using a pointer that references the old data. 
 *             This can lead to undefined behavior, data corruption, or 
 *             security exploits.
 * 
 */

/*
 * Changes in This Version after Mishels Comments on August 24th:
 *   
 *  Focus on Temporal Safety: The memory allocated for new_ptr has the same 
 *      size as original_ptr. If the allocator reuses the same memory block, 
 *      the test would trigger a use-after-reallocation issue when original_ptr 
 *      is accessed after being freed.
 *   
 *  No Out-of-Bounds Access: The previous out-of-bounds write was removed. 
 *      This revision now focuses purely on temporal safety without introducing 
 *      spatial safety violations.
 *   
 *   Checking for Address Reuse: A check is added to see if the allocator reuses 
 *      the same memory address for both original_ptr and new_ptr. If this 
 *      happens, it highlights the temporal safety issue.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void use_after_reallocation() {
  // Step 1: Allocate memory and initialize
  int *original_ptr = (int *)malloc(10 * sizeof(int));
  if (original_ptr == NULL) {
    printf("Memory allocation failed\n");
    return;
  }
  for (int i = 0; i < 10; i++) {
    original_ptr[i] = i * 10;
  }
  printf("Original data: ");
  for (int i = 0; i < 10; i++) {
    printf("%d ", original_ptr[i]);
  }
  printf("\n");

  // Store the address of the original pointer for comparison later
  printf("Address of original_ptr: %p\n", (void *)original_ptr);

  // Step 2: Free the allocated memory
  free(original_ptr);

  // Step 3: Reallocate memory for a different purpose
  int *new_ptr = (int *)malloc(10 * sizeof(int));  // Same size allocation
  if (new_ptr == NULL) {
      printf("Memory allocation failed\n");
      return;
  }

  // Print the address of the new pointer
  printf("Address of new_ptr: %p\n", (void *)new_ptr);

  // Check if the same memory was reused for new_ptr
  if (new_ptr == original_ptr) {
    printf("Memory reused! Potential temporal safety violation.\n");
  } else {
    printf("Memory not reused.\n");
  }

  // Step 4: Access the old data through the original pointer
  // (Note: This is unsafe and leads to undefined behavior)
  printf("Attempting to access old data through original_ptr:\n");
  for (int i = 0; i < 10; i++) {
    printf("%d ", original_ptr[i]);  // This would trigger a temporal violation
  }
  printf("\n");

  // Step 5: Access the old data through the new pointer
  // (Note: This is unsafe and leads to undefined behavior)
  printf("Attempting to access old data through new_ptr:\n");
  for (int i = 0; i < 10; i++) {
    printf("%d ", new_ptr[i]);  // This would trigger a temporal violation
  }
  printf("\n");


  // Clean up
  free(new_ptr);
}

int main() {
  use_after_reallocation();
  return 0;
}
