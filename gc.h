#ifndef GC_H
#define GC_H

#include <unistd.h>
#include <stddef.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct memory_block {
    size_t size;
    struct memory_block* next;
    int marked;           
    int free;
    int pinned;     
    char data[1]; //sample payload :)
} memory_block;

typedef struct heap_profiler {
    size_t total_allocated;
    size_t total_freed;
    size_t free_blocks;
    size_t total_fragments;
} heap_profiler;

extern heap_profiler profiler; 

void* gc_malloc(size_t size); 
void gc_free(void* ptr);
void gc_collect(void); 
void gc_pin(void* ptr); 
void gc_unpin(void* ptr); 

void print_profiler_stats(void); 
void print_all_blocks(void);

#endif // GC_H
