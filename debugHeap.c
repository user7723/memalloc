#include "memalloc.h"
#include <unistd.h> // size_t, sbrk()
#include <assert.h>

char numToHexChar(size_t x) {
  if(x <= 9) return x + '0';
  return x - 10 + 'a';
}

#define STDOUT 1
void printHex(size_t x) {
  static char hexBuf[64];
  size_t x1, i, c;
  if(x == 0) {
    write(STDOUT, "0x0", 3);
    return ;
  }

  for(x1 = x, i = 0; x1 != 0; x1 /= 16, i++);

  c = i;

  for(; x != 0; x /= 16, i--)
    hexBuf[i-1] = numToHexChar(x % 16);

  write(STDOUT, "0x", 2);
  write(STDOUT, hexBuf, c);
  return ;
}
void nl(void) {
  write(STDOUT, "\n", 1);
  return ;
}

void sp(void) {
  write(STDOUT, " ", 1);
  return ;
}

// Should I care that there's an exorbitant amount system calls?
void printBlock(void* ptr) {
  assert(ptr != NULL);
  Header* h = (Header*) ptr;
  Footer* f = (Footer*)header2footer(h);

  printHex((size_t)h);         // header ptr
  write(STDOUT, "[size:", 6);
  printHex(h->size);           // header size field
  write(STDOUT, ";free:", 6);
  printHex(h->free);           // header free field
  write(STDOUT, "] ", 2);

  printHex((size_t)header2mem(h));    // data region ptr
  write(STDOUT, "[...] ", 6);

  printHex((size_t)f);         // footer ptr
  write(STDOUT, "[size:", 6);
  printHex(f->size);           // footer size field
  write(STDOUT, "]\n", 2);
}

void printBreak(void) {
  printHex((size_t)sbrk(0)); write(STDOUT, "\n", 1);
  return ;
}

void printMem(void* mem) {
  Header* h = (Header*)((void*)mem - sizeof(Header));
  for(size_t i = 0; i < h->size / sizeof(int); i++) {
    printHex(((int*)mem)[i]);
    sp();
  }
  nl();
  return ;
}

void printHeap(void* ptr) {
  write(STDOUT, "\n", 1);
  for(; ptr && ptr >= heapStart && ptr < heapEnd; ptr = nextBlock(ptr)) {
    printBlock(ptr);
  }
  return ;
}
