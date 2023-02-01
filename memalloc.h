#pragma once

#include <stdbool.h>
#include <unistd.h>

#define MAX_DEMAND_TYPE size_t
#define CELL_SIZE sizeof(MAX_DEMAND_TYPE)
#define MIN_SEG_SIZE (sizeof(Header) + CELL_SIZE + sizeof(Footer))
#define BRK_ALLOC_LIM 0x100000000

typedef struct Header Header;
typedef struct Footer Footer;

struct Header {
  size_t size;
  bool   free;
} ;

struct Footer {
  size_t size;
} ;

extern void* heapStart;
extern void* heapEnd;


// inline size_t alignBytes(size_t nbytes);
// Header* findBlock(size_t nbytes);
// Header* reqMoreMemory(size_t nbytes);
// Header* splitBlock(Header* h, size_t nbytes);
// inline bool doesNeedSplit(size_t dataSize, size_t nbytes);
// Header* join(Header* prev, Header* curr);
// static void _occupyOrSplit(Header* curr, Header* next, size_t wc_n, size_t req);
void  memcopy(void* dst, void* src, size_t nbytes);
void* memalloc(size_t nbytes);
void  memfree(void* ptr);
void* memrealloc(void* mem, size_t nbytes);
void* cmemalloc(size_t nmemb, size_t size);

/*
 * All of these macros will eventually evaluate to the void*.
 * And, with great probability, the void* will be type-casted
 * somewhere in the code, thus, these expressions must be
 * wrapped in extra parenthesis to ensure that nothing else,
 * but exactly the returned value is casted.
 *
 */

// ^     - pointer state
// a-->b - from a to b
// h - header
// f - footer
// m - memory region

// [h |m |f ][h |m |f ]
//       ^<--^
#define header2pfooter(h) \
  ((void*)h - sizeof(Footer))

// [h |m |f ]
// ^->^
#define header2mem(h) \
  ((void*)h + sizeof(Header))

// [h |m |f ]
// ^---->^
#define header2footer(h) \
  ((h == NULL) ? (h) : ((void*)header2mem(h) + ((Header*)h)->size))

// [h |m |f ]
//    ^<-^
#define footer2mem(f) \
  ((f == NULL) ? (f) : ((void*)f - ((Footer*)f)->size))

// [h |m |f ]
//    ^<-^
#define footer2header(f) \
  ((void*)footer2mem(f) - sizeof(Header))

// [h |m |f ][h |m |f ]
// ^-------->^
#define nextBlock(h) \
  ((h == NULL || (((void*)header2footer(h)) + sizeof(Footer)) >= heapEnd) \
   ? (NULL)\
   : (((void*)header2footer(h)) + sizeof(Footer)))

// [h |m |f ][h |m |f ]
// ^<--------^
#define prevBlock(h) \
  ((h == NULL || (void*)h - sizeof(Footer) <= heapStart)\
   ? (NULL)\
   : ((void*)footer2header(header2pfooter(h))))

// [h |m |f ]
// ^<-^
#define mem2header(m) \
  ((void*)m - sizeof(Header))

