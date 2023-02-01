CC=gcc
CFLAGS= -ggdb -Wall -Wextra -I. -O2

main: testMemalloc.c memalloc.c debugHeap.c
	$(CC) $(CFLAGS) -o $@ $^
