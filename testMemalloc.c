#include "memalloc.h"
#include "debugHeap.h"
#include <assert.h>

void testCmemalloc(void) {
  size_t cnt = 10;
  size_t size = sizeof(size_t);

  // allocate usual memory region
  size_t* p1 = (size_t*)memalloc(cnt*size);
  assert(p1 != NULL);
  printMem(p1);

  // copy some stuff into it
  memcopy(p1, "Shall I compare thee to a summers day?\n thou art more lovely and more temperate.", cnt*size);
  printMem(p1); nl();
  // free the region
  memfree(p1);
  // make sure that everything would remain the same
  // without explicit initialization
  p1 = (size_t*)memalloc(cnt*size);
  printMem(p1); nl();
  memfree(p1); // free it again...

  // then, allocate the same memory space initializing it to zeroes
  p1 = (size_t*)cmemalloc(cnt, size);
  printMem(p1); nl();

  memfree(p1);
  printHeap(heapStart);

  return ;
}

#define PTRCNT 4
static void* ptrs[PTRCNT] = {0} ;
static size_t sizes[PTRCNT] = {0x1, 0x10, 0x100, 0x1000};

void testAlloc(void) {
  for(int i = 0; i < PTRCNT; i++) {
    ptrs[i] = memalloc(sizes[i]);
    printHeap(heapStart);
  }
}

void testFree(void) {
  for(int i = 0; i < PTRCNT; i++) {
    memfree(ptrs[i]);
    printHeap(heapStart);
  }
}

void testReallocPrevJoin(void) {
  void* p1 = memalloc(0x100);
  void* p2 = memalloc(0x100);
  void* p3 = memalloc(0x100);
  memfree(p1);

  printHeap(heapStart);

  p2 = memrealloc(p2, 0x150);
  assert(p2 != NULL);
  assert(p2 == p1);
  printHeap(heapStart);

  memfree(p2);
  memfree(p3);
  printHeap(heapStart);
}

void testReallocNextJoin(void) {
  void* p1 = memalloc(0x100);

  printHeap(heapStart);

  void* p2 = memrealloc(p1, 0x150);
  assert(p2 != NULL);
  assert(p1 == p2);

  printHeap(heapStart);

  memfree(p2);
  printHeap(heapStart);
}

void testReallocBothJoin(void) {
  void* p1 = memalloc(0x100);
  void* p2 = memalloc(0x100);
  void* p3 = memalloc(0x100);
  void* p4 = memalloc(0xab0);
  memfree(p1);
  memfree(p3);
  printHeap(heapStart);
  p2 = memrealloc(p2, 0x250);
  assert(p2 != NULL);
  assert(p2 == p1);

  printHeap(heapStart);

  memfree(p2);
  memfree(p2);
  memfree(p3);
  memfree(p4);
  printHeap(heapStart);
}

void testReallocNew(void);

int main(void) {
  //testReallocPrevJoin();
  //testReallocNextJoin();
  //testReallocBothJoin();
  testCmemalloc();
  //void* p = memalloc(1000*1024*1024);
  //printHeap(heapStart);
  //memfree(p);
  //while(1);
  return 0;
}
