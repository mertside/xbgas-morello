/*  Benchmark: Return-Oriented Programming (ROP) Attack
 *  @author  : Mert Side for TTU
 *  @brief   : This C program simulates an automated control flow attack by 
 *             exploiting a buffer overflow vulnerability. The goal is to 
 *             overwrite the return address stored on the stack with the address 
 *             of a function that should not normally be executed (print_flag). 
 *             The program is designed to do this automatically without any user 
 *             input.
 * 
 */

/*
 * Changes in This Version after Mishels Comments on August 24th:
 * 
 *   Gadgets: We created three simple gadgets (gadget1, gadget2, final_gadget) 
 *     that perform minor operations and end with ret using inline assembly. 
 *     These represent realistic ROP gadgets.
 * 
 *   ROP Chain: The function vulnerable_function simulates a stack overflow that
 *     overwrites the return address with pointers to the gadgets. The program
 *     will call the gadgets in sequence as the simulated ROP chain.
 * 
 *   Stack Mitigation (Optional): We can add mitigations like stack canaries or
 *     shadow stacks to make the attack more realistic, but we have excluded them
 *     here for simplicity.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void gadget1() {
  printf("Executing Gadget 1: Setting up registers\n");
  asm("ret");  // Simulate gadget return
}

void gadget2() {
  printf("Executing Gadget 2: Modifying control flow\n");
  asm("ret");  // Simulate gadget return
}

void final_gadget() {
  printf("Final Gadget: Payload executed\n");
  asm("ret");  // Simulate gadget return
}

void vulnerable_function() {
  char buffer[64];
  void *return_addresses[3];  // Simulated return address stack for gadgets

  printf("Simulating stack setup for ROP attack...\n");
  
  // Simulated ROP chain: overwrite return address to call gadgets in sequence
  return_addresses[0] = gadget1;
  return_addresses[1] = gadget2;
  return_addresses[2] = final_gadget;

  // Simulate buffer overflow to overwrite return addresses
  memcpy(buffer + sizeof(buffer), return_addresses, sizeof(return_addresses));
  
  // Normally, control would return here, but we'll force it through gadgets instead
  ((void(*)())*(void **)(buffer + sizeof(buffer)))();
}

int main() {
  printf("Starting the ROP attack simulation.\n");
  vulnerable_function();
  printf("If no crash, the ROP chain executed correctly.\n");
  return 0;
}
