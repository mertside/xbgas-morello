/*
 * Benchmark: HeartBleed - Out-of-Bounds Read with length input
 * Adapted for xBGAS by Mert Side for Texas Tech University
 * 
 * Key Notes:
 * This program simulates a Heartbleed-like vulnerability where an attacker can 
 * request more data than what was originally allocated, leading to the exposure 
 * of sensitive information. This version is adapted to run in a multi-threaded 
 * xBGAS environment.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xbrtime_morello.h"

#define HB_INPUT_SIZE 6
#define RSA_KEY_SIZE 7

// Function to simulate the Heartbleed vulnerability
void* heartbleed_vulnerability(void* arg) {
  long tid = (long)arg;
  printf("[Thread %ld] Starting test: HeartBleed Example\n", tid);

  int test_status = 1;

  // Simulate input heartbeat request
  char *hb_input = (char *)malloc(HB_INPUT_SIZE);
  strcpy(hb_input, "HB MSG");

  // Sensitive information stored in memory
  char *rsa_key = (char *)malloc(RSA_KEY_SIZE);
  strcpy(rsa_key, "RSA KEY");

  // Calculate offset between hb_input and rsa_key
  int offset = rsa_key - hb_input;

  // Simulate user input with len greater than provided characters
  int num = HB_INPUT_SIZE + offset + RSA_KEY_SIZE;

  printf("[Thread %ld] Responding to heartbeat request with %d characters:\n", 
          tid, num);
  for (int i = 0; i < num; i++) {
    printf("%c", hb_input[i]);

    if (i > (HB_INPUT_SIZE + offset) && hb_input[i] == rsa_key[i - offset])
      test_status = 0;
  }
  printf("\n");

  if (test_status == 0)
    printf("[Thread %ld] Test Failed: HeartBleed\n\n", tid);

  // Free allocated memory
  free(hb_input);
  free(rsa_key);

  return NULL;
}

int main() {
  xbrtime_init();
  int num_pes = xbrtime_num_pes();

  printf("Starting multi-threaded test: HeartBleed Example\n");

  // Add work to each thread in the thread pool
  for (long i = 0; i < num_pes; i++) {
    tpool_add_work(threads[i].thread_queue, heartbleed_vulnerability, (void*)i);
  }

  // Wait for all threads to complete their work
  for (int i = 0; i < num_pes; i++) {
    tpool_wait(threads[i].thread_queue);
  }

  printf("Completed multi-threaded test: HeartBleed Example:     EXPLOITED!\n");

  xbrtime_close();

  return 0;
}
