/*
 * Benchmark: Data Oriented Programming (DOP)
 * Adapted for xBGAS by Mert Side for Texas Tech University
 * 
 * Key Notes:
 * This program simulates a DOP attack where a buffer overflow is used to
 * manipulate a control variable (`adminAccess`). This version is adapted to run 
 * in a multi-threaded xBGAS environment, testing the system’s capability to 
 * prevent such attacks.
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "xbrtime_morello.h"

#define BUFFER_SIZE 5
#define INPUT_SIZE 10

void* dop_vulnerability(void* arg) {
  long tid = (long)arg;
  printf("[Thread %ld] Starting test: Data Oriented Programming\n", tid);

  int exploit_input[INPUT_SIZE] = {55, 23, 45, 31, 88, 14, 32, 45, 32, 10};
  int buffer[BUFFER_SIZE];  // Small buffer
  int adminAccess = 0;      // Variable controlling access

  char* adminAccessPointer = (char*)&adminAccess;
  char* bufferAddress = (char*)buffer;
  int bufferAccessDifference = adminAccessPointer - bufferAddress;

  // Attempt to overflow the buffer and modify adminAccess
  for (int i = 0; i < INPUT_SIZE; i++) {
    // This line should trigger bounds checking on a CHERI-enabled platform
    buffer[i + bufferAccessDifference] = exploit_input[i];
  }

  if (adminAccess) {
    printf("[Thread %ld] Test Failed: DOP - Overflow into adjacent variable\n", 
            tid);
  } else {
    printf("[Thread %ld] Regular user access.\n", tid);
  }

  return NULL;
}

int main() {
  xbrtime_init();
  int num_pes = xbrtime_num_pes();

  printf("Starting multi-threaded test: Data Oriented Programming\n");

  // Add work to each thread in the thread pool
  for (long i = 0; i < num_pes; i++) {
    tpool_add_work(threads[i].thread_queue, dop_vulnerability, (void*)i);
  }

  // Wait for all threads to complete their work
  for (int i = 0; i < num_pes; i++) {
    tpool_wait(threads[i].thread_queue);
  }

  printf("Completed test: Data Oriented Programming:             EXPLOITED!\n");

  xbrtime_close();

  return 0;
}
