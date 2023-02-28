# memalloc

## Why does it exist?
`memalloc` is a primitive dynamic memory allocator, that I've written simply to learn how can one implement something with resembling functionality of the `malloc` et al. from standard library.

## Description

### Memory Layout

The memory in the heap is represented as a sequence of consecutive blocks each enclosed by the `Header` and `Footer` structures:
```
...[header:[size;free]][memory-region][footer:[size]]...
```
### Structure of Metadata
- `header` consists of
  - the `size` of `memory-region`
  - and, a `free`-flag that tells whether the block is free or not
- `footer` contains identical `size` value as the `header` from the same block


### Blocks List Organization
Blocks are linked implicitly to each other. That means that there isn't any pointer fields to next and previous blocks of memory, their addresses must be computed arithmetically, depending on the values stored in `size` field of either the `header` or `footer`, e.g. in order to get next block, given some Header pointer `h` you need to make roughly the following:
```C
void* prev = (void*)h + (sizeof(Header) + h->size + sizeof(Footer))
if(!(prev >= heapStart && prev < heapEnd))
  return NULL;
```
i.e skip the current `header`, skip memory region, and then the `footer`. After that check whether resultant pointer resides within the heap bounds determined by `heapStart` and `heapEnd` global variables.

The consequence of implicit linking is that you cannot use that memory allocator together with anything that may interfere with the heap and break contiguity of the blocks, which will lead to wrong pointer arithmetic and, thus, will break implicit linkage of the blocks.

### Pointer Arithmetic Macros
All pointer operations are "abstracted" using macros:
  - `header2footer(h)` - given valid `Header*` computes `void*` to the `Footer` of the same block
```
[h |m |f ]
^---->^
```
  - `header2pfooter(h)` - given valid `Header*` computes `void*` that represents `Footer` of previous block
```
[h |m |f ][h |m |f ]
      ^<--^
```
  - `header2mem(h)` - given valid `Header*` computes `void*` that corresponds to data segment of its block
```
[h |m |f ]
^->^
```
  - `nextBlock(h)` - given valid `Header*` computes `void*` that corresponds to previous block which may be `NULL` in case if either the `h` is invalid pointer or it points to the last block on the heap

```
[h |m |f ][h |m |f ]
^-------->^
```
  - `footer2mem(f)` - given valid `Footer*` computes `void*` that corresponds to memory segment of its block
```
[h |m |f ]
   ^<-^
```
  - `footer2header(f)` - given valid `Footer*` computes `void*` that correspond to `Header` of the same block
```
[h |m |f ]
^<----^
```
  - `prevBlock(h)` - given valid `Header*` computes previous blocks `Header*` position
```
[h |m |f ][h |m |f ]
^<--------^
```
 - `mem2header(m)` - given valid memory segment pointer computes its `Header` address
```
[h |m |f ]
^<-^
```


Where:
```
^     - pointer state
a-->b - from a to b
h     - header
f     - footer
m     - memory region
```

### memalloc
The `memalloc` aligns requested size, and tries to find the first free block of sufficient volume.
  - If the block wasn't found, then
    - at least one page of memory is allocated by `sbrk`, and
      - if unsuccessfull -- the `NULL` is returned
    - corresponding meta-data is attached to that memory block

Then if the found block size is
  - equal to requested number of bytes, then
    - it's marked as in-use, and
    - the pointer to it's memory region is returned
  - greater than the requested number of bytes, then
    - if the remainder of memory will be sufficient to allocate another block with memory region of at least `CELL_SIZE` (`sizeof(void*)`), then
      - it becomes split in two blocks, and
      - the pointer to the requested memory region is returned
    - if the remainder wouldn't have enough space, then the whole block space is occupied

### memfree
The `memfree` function
  - finds next and previous blocks
  - checks whether they are not `NULL` and are `free` blocks
  - joins the block being freed with its neighbours depending on the above observations

### memrealloc
The `memrealloc` function appeared to be much more complicated than I've imagined at first, because of different corner cases that must be taken into account.

Firstly the `memrealloc` function deals with a degenerate input:
  - if the pointer provided to `memrealloc` is `NULL` then nothing is need to be done, and `NULL` value is returned
  - if the size parameter is zero then the pointer that was passed will be handed to `memfree` function

Otherwise, it computes all necessary information on geometry of the current block and its neighbours. That information is further used to make correct decisions on the memory reallocation.

After that, `memrealloc` branches depending on its requested-size parameter (that was previously adjusted to become a multiple of 8 bytes).

If the request size is less than the size of the current block, then we either
  - split the current block (if there is sufficient amount of space to further minimal memory segment allocation)
    - if there is another free block after the newly produced tail, then they are joined
  - or left it untouched, if the remainder wouldn't have enough space to allocate minimal memory segment
 
If the request size is greater than the size of current block, then there is four possible ways we can go:
  - if the next block is free and there will be enough space to satisfy the request, in case we'll join the current and next blocks
    - if after extension there will be left free space, and it is enough to allocate at least minimal memory segment then, we allocate the remainder as a free block
    - otherwise, the request occupies the next block completely
  - if the previous block is free and union of current and previous blocks will be sufficient to satisfy the request
    - it copies the current memory segment to the previous memory segment
    - and then it decides to split or occupy the space
  - if both previous and next blocks are free, and if unified together with the current block they will produce enough space to satisfy the request then
    - it copies the current memory segment to the previous memory segment
    - and then by the same logic as above decides on whether to split the resultant space in two, or occupy it completely
  - if it cannot unify with neighbouring blocks then there's nothing else it can do, except for new block allocation
    - if the new block was allocated successfully, it copies the current memory contents to that new block
    - and then it's calling `memfree` to free the "current" block

Pointer to reallocated memory segment is returned.

### cmemalloc
Does the same that `memalloc`, but initializes the memory segment with zeroes.

### memcopy

There is also a supplementary `memcopy` function that runs in two steps
  - first it copies `src` to `dst` by eight-byte chunks, and then 
  - it copies the remaining data byte-wise

### debug functions
There is also a couple of quick and dirty functions to visualize the memory blocks being allocated on the heap:
  - `void printHeap(void*)` - given valid `Header` prints all allocated blocks of memory that reside in the heap bounds
  - `void printBlock(void*)` - given valid `Header` pointer prints the block of memory
  - `void printMem(void*)` - prints four byte chunks of given memory segment in hexadecimal form
  - `void printBreak(void)` - prints current systems break of a process
  - `void printHex(size_t)` - prints hexadecimal representation of the given value
