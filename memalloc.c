#include <assert.h>
#include <stdbool.h>
#include <unistd.h>
#include "memalloc.h"
#include "debugHeap.h"

// #define NDEBUG

void* heapStart = NULL;
void* heapEnd   = NULL;
size_t sizeofMeta = sizeof(Header) + sizeof(Footer);
size_t pageSize   = 0;

inline size_t alignBytes(size_t nbytes) {
  return (size_t)(nbytes + CELL_SIZE - 1) / CELL_SIZE * CELL_SIZE;
}

Header* findBlock(size_t nbytes) {
  Header* bptr = (Header*)heapStart;
  for(; bptr != NULL; bptr = nextBlock(bptr))
    if(bptr->free && bptr->size >= nbytes) break;
  return bptr;
}

Header* reqMoreMemory(size_t nbytes) {
  if(heapStart == NULL) heapStart = sbrk(0);
  assert(heapStart != NULL);

  if(pageSize == 0) pageSize = sysconf(_SC_PAGESIZE);
  assert(pageSize != 0);

  if(nbytes < pageSize) nbytes = pageSize;

  void* memptr = sbrk(nbytes);
  if(memptr == (void*)-1) return NULL;

  Header* h = (Header*)memptr;
  assert(h != NULL);

  h->size = nbytes - sizeofMeta;
  h->free = true;

  Footer* f = (Footer*)header2footer(memptr);
  assert(f != NULL);
  f->size = h->size;

  heapEnd = (void*)f + sizeof(Footer);
  return h;
}

Header* splitBlock(Header* h, size_t nbytes) {

  assert(h != NULL);
  Footer* fInit = (Footer*)header2footer(h);
  size_t initSize = h->size;

  h->size = nbytes;
  h->free = false;

  Footer* f = (Footer*)header2footer(h);

  assert(f != NULL);
  f->size = nbytes;

  Header* hTail = (Header*)nextBlock(h);

  assert(hTail != NULL);
  hTail->size = initSize - nbytes - sizeofMeta;
  hTail->free = true;

  Footer* fTail = (Footer*)header2footer(hTail);

  assert(fTail != NULL);
  fTail->size = hTail->size;

  assert(fInit == fTail);

  return h;
}

inline bool doesNeedSplit(size_t dataSize, size_t nbytes) {
  assert(dataSize >= nbytes);
  return dataSize - nbytes >= MIN_SEG_SIZE;
}


void* memalloc(size_t nbytes) {
  if(nbytes == 0) return NULL;
  if(nbytes + sizeofMeta > BRK_ALLOC_LIM) return NULL;

  // alignment doesn't include Header and Footer sizes
  nbytes = alignBytes(nbytes);

  Header* block = findBlock(nbytes);
  if(block == NULL) {
    block = reqMoreMemory(nbytes + sizeofMeta);
    if(block == NULL) return NULL;
  }
  assert(block->size >= nbytes);

  if(doesNeedSplit(block->size, nbytes))
    block = splitBlock(block, nbytes);

  assert(block->size == (((Footer*)header2footer(block))->size));

  block->free = false;
  return header2mem(block);
}

Header* join(Header* prev, Header* curr) {
  assert(prev != NULL);
  assert(curr != NULL);

  prev->size += sizeofMeta + curr->size;
  Footer* f = (Footer*)header2footer(prev);
  f->size = prev->size;
  return prev;
}

void memfree(void* ptr) {
  Header *h, *prev, *next;
  bool joinPrev, joinNext;

  if(ptr == NULL) return ;

  h = mem2header(ptr);
  assert(h != NULL);
  assert((void*)h >= heapStart && (void*)h < heapEnd);

  h->free = true;

  prev = (Header*)prevBlock(h);
  next = (Header*)nextBlock(h);

  joinPrev = prev && prev->free;
  joinNext = next && next->free;

  if(joinPrev) h = join(prev, h);
  if(joinNext) h = join(h, next);

  return;
}

void memcopy(void* dst, void* src, size_t nbytes) {
  int nchunks = nbytes / CELL_SIZE;
  size_t rem = nbytes % CELL_SIZE;
  size_t shift = nbytes - rem;
  // first copy chunks of memory by multiples of CELL_SIZE
  MAX_DEMAND_TYPE* dstch = dst;
  MAX_DEMAND_TYPE* srcch = src;
  for(; nchunks > 0; nchunks--)
    *dstch++ = *srcch++;

  // then copy the remaining bytes
  dst += shift;
  src += shift;
  for(nbytes = rem; nbytes > 0; nbytes--)
    *(char*)dst++ = *(char*)src++;

  return;
}

static void _occupyOrSplit(Header* curr, Header* next, size_t wc_n, size_t req) {
  if(wc_n - req >= MIN_SEG_SIZE) { // you can split
    // thus, it must be guaranteed that there is enough space to
    // allocate at least minimally sized segment
    curr->size = req - sizeofMeta;
    curr->free = false;
    ((Footer*)header2footer(curr))->size = req - sizeofMeta;
    next = nextBlock(curr);
    next->size = wc_n - req - sizeofMeta;
    next->free = true;
    ((Footer*)header2footer(next))->size = next->size;
  } else { // you cannot split
    // so, occupy the whole space of the neighbor block
    curr->size += sizeofMeta + next->size;
    curr->free = false;
    ((Footer*)header2footer(curr))->size = curr->size;
    assert(((Footer*)header2footer(curr)) == header2footer(next));
  }
  return ;
}

void* memrealloc(void* mem, size_t nbytes) {
  Header *curr, *prev, *next;
  bool nextFree, prevFree, nextEnough, prevEnough, bothEnough;
  size_t wc_n, wp_c, wp_n, req;

  if(nbytes == 0) {
    memfree(mem);
    return NULL;
  }

  if(mem == NULL) return NULL;
  nbytes = alignBytes(nbytes);

  curr = (Header*)mem2header(mem);
  if(nbytes == curr->size) return mem;

  assert(!curr->free);
  assert((void*)curr >= heapStart && (void*)curr < heapEnd);

  prev = (Header*)prevBlock(curr);
  next = (Header*)nextBlock(curr);

  nextFree = next && next->free;
  prevFree = prev && prev->free;

  req = nbytes + sizeofMeta;
  wc_n = (nextFree) ? curr->size + next->size + sizeofMeta * 2 : 0;
  wp_c = (prevFree) ? prev->size + curr->size + sizeofMeta * 2 : 0;
  wp_n = (prevFree && nextFree)
       ? prev->size + curr->size + next->size + sizeofMeta * 3 : 0;

  nextEnough = wc_n >= req; // width of curr-next is enough to satisfy the req.
  prevEnough = wp_c >= req; // width of prev-curr is enough to satisfy the req.
  bothEnough = wp_n >= req; // width of prev-next is enough to satisfy the req.

  // decrease
  if(nbytes < curr->size && doesNeedSplit(curr->size, nbytes)) {
    curr = splitBlock(curr, nbytes);
    assert(curr != NULL);
    if(nextFree) curr = join(nextBlock(curr), next);
    mem = header2mem(curr);
  }
  else if(nbytes > curr->size) {
    if(nextEnough) {
      _occupyOrSplit(curr, next, wc_n, req);
      mem = header2mem(curr);
    } else if(prevEnough) {
      memcopy(header2mem(prev), header2mem(curr), curr->size);
      _occupyOrSplit(prev, curr, wp_c, req);
      mem = header2mem(prev);
    } else if(bothEnough) {
      memcopy(header2mem(prev), header2mem(curr), curr->size);
      _occupyOrSplit(prev, next, wp_n, req);
      mem = header2mem(prev);
    } else {
      void* memNew = memalloc(nbytes);
      if(memNew == NULL) return NULL;
      memcopy(memNew, header2mem(curr), curr->size);
      memfree(header2mem(curr));
      mem = memNew;
    }
  }

  return mem;
}

void* cmemalloc(size_t nmemb, size_t size) {
  void* res = memalloc(nmemb*size);
  if(res == NULL) return NULL;

  MAX_DEMAND_TYPE* tmp = (MAX_DEMAND_TYPE*)res;
  size = ((Header*)mem2header(res))->size / CELL_SIZE;
  for(size_t i = 0; i < size ; i++) tmp[i] = 0;

  return res;
}
