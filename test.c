//test to see if its working as intended.

#include "gc.c"

int main() {

    printf("== Initial GC and Heap Profiler Test ==\n");

    int *a = (int*)gc_malloc(sizeof(int));
    int *b = (int*)gc_malloc(sizeof(int));
    int *c = (int*)gc_malloc(sizeof(int));

    *a = 10;
    *b = 20;
    *c = 30;

    printf("a: %d, b: %d, c: %d\n", *a, *b, *c);

    gc_collect();
    print_profiler_stats();
    
    
    gc_free(a);
    gc_free(b);
    gc_free(c);

    gc_collect();
    print_profiler_stats();

    print_all_blocks();

    printf("\n== Pinning and Unpinning Test ==\n");
    int *d = (int*)gc_malloc(sizeof(int));
    *d = 42;
    
    gc_pin(d);
    printf("Pinned object d: %d\n", *d);

    gc_free(d);
    gc_collect();  
    print_all_blocks();

    gc_unpin(d);
    gc_free(d);
    gc_collect();
    print_all_blocks();

    return 0;
}
