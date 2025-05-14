//works only on unix/linux systems as it uses syscalls specific to them.

#define _POSIX_C_SOURCE 199309L
#include "gc.h"

typedef struct heap_profiler {
    size_t total_allocated;
    size_t total_freed;
    size_t free_blocks;
    size_t total_fragments;
} heap_profiler;

//stores top of stack pointer for gc scanning
extern char **environ;

heap_profiler profiler = {0};

// Simulated list that tracks all memory blocks.
static memory_block* free_list = NULL;

//could be used to benchmark time taken to execute some func
void benchmark(const char* label, void (*func)()) {
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    
    //calling the function from the reference given
    func();
    clock_gettime(CLOCK_MONOTONIC, &end);

    printf("%s took %.3f ms\n", label, 
        (end.tv_sec - start.tv_sec) * 1e3 +
        (end.tv_nsec - start.tv_nsec) / 1e6);
}

// pin an object to prevent it getting collected by gc.
void gc_pin(void* ptr) {
    memory_block* block = (memory_block*)((char*)ptr - sizeof(memory_block));
    block->pinned = 1;
}

// unpin an object to allow it to get collected by gc.
void gc_unpin(void* ptr) {
    memory_block* block = (memory_block*)((char*)ptr - sizeof(memory_block));
    block->pinned = 0;
}

// malloc function from scratch
void* gc_malloc(size_t size) {
    size_t total_size = sizeof(memory_block) + size;
    memory_block* block = free_list;
    memory_block* prev = NULL;

    while (block) {
        if (block->free && block->size >= total_size) {
            block->free = 0;
            block->marked = 0;

            // split the remaining space into a new memory block.
            if (block->size > total_size + sizeof(memory_block)) {
                memory_block* new_block = (memory_block*)((char*)block + total_size);
                new_block->size = block->size - total_size;
                new_block->free = 1;
                new_block->marked = 0;
                new_block->next = block->next;

                block->size = total_size;
                block->next = new_block;
            }

            profiler.total_allocated += total_size;
            profiler.free_blocks--;

            return (void*)(block + 1);
        }

        prev = block;
        block = block->next;
    }

    // Request OS to increase heap memory and return a pointer after allocating memory.
    block = (memory_block*)mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (block == (void*)-1) {
        return NULL;
    }

    block->size = total_size;
    block->free = 0;
    block->marked = 0;
    block->next = NULL;

    if (!free_list)
        free_list = block;
    else    
        prev->next = block;

    profiler.total_allocated += total_size;

    return (void*)(block + 1);
}

// merges two consecutive memory blocks to prevent fragmentation.
void merge_free_blocks() {
    memory_block* curr = free_list;

    while (curr && curr->next) {
        if (curr->free && curr->next->free) {
            curr->size += curr->next->size + sizeof(memory_block);
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

//deallocate memory from the heap.
void gc_free(void *ptr) {
    memory_block* block = free_list;
    memory_block* block_ptr = (memory_block*)((char*)ptr - sizeof(memory_block));

    while (block) {
        if (block == block_ptr) {

            if (block->pinned) {
                printf("Attempted to free a pinned block, skipping.\n");
                return;
            }

            block->free = 1;
            block->marked = 0;
            profiler.total_freed += block->size;
            profiler.free_blocks++;

            break;
        }
        block = block->next;
    }

    merge_free_blocks(); 
}


void mark(void* ptr) {
    memory_block* curr = free_list;
    while (curr) {
        if ((void*)(curr + 1) == ptr) { 
            curr->marked = 1;
            break;
        }
        curr = curr->next;
    }
}

int is_valid_heap_pointer(void *ptr) {
    if (ptr == NULL) return 0;

    memory_block* block = (memory_block*)((char*)ptr - sizeof(memory_block));
    memory_block* curr = free_list;

    while (curr) {
        if (curr == block) return 1;
        curr = curr->next;
    }

    return 0;
}

// go through all the stack variables of the current stack pointer
void mark_from_stack() {
    void* stack_top = (void*)&environ;
    void* stack_bottom = __builtin_frame_address(0);

    void** start = (void**)stack_bottom;
    void** end = (void**)stack_top;

    for (void** p = start; p < end; ++p) {
        void* candidate = *p;
        if (candidate && is_valid_heap_pointer(candidate))
            mark(candidate);
    }
}

void sweep() {
    memory_block* curr = free_list;
    while (curr) {
        if (!curr->marked && !curr->free && !curr->pinned) {
            curr->free = 1;
        }
        curr = curr->next;
    }
}

void gc_collect() {
    mark_from_stack();
    sweep();
}

void print_profiler_stats() {
    printf("Heap Profiler Stats:\n");
    printf("Total Allocated: %zu bytes\n", profiler.total_allocated);
    printf("Total Freed: %zu bytes\n", profiler.total_freed);
    printf("Free Blocks: %zu\n", profiler.free_blocks);
}

void print_all_blocks() {
    memory_block* curr = free_list;
    printf("=== Memory Blocks ===\n");
    while (curr) {
        printf("Block at %p | Size: %zu | Free: %d | Marked: %d\n",
               (void*)curr, curr->size, curr->free, curr->marked);
        curr = curr->next;
    }
    printf("=====================\n");
}
