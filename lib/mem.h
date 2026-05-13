#ifndef MEM_H_FILE
#define MEM_H_FILE

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#if __STDC_VERSION__ < 202311L
#include <stdalign.h>
#endif

struct MEM_ARENA {
    void *data;
    size_t size;
    size_t alloc;
};

static inline void mem_clear(struct MEM_ARENA *mem) { mem->alloc = 0; }
static inline size_t mem_save(struct MEM_ARENA *mem) { return mem->alloc; }
static inline void mem_restore(struct MEM_ARENA *mem, size_t saved) { mem->alloc = saved; }

static inline void mem_init(struct MEM_ARENA *mem, void *data, size_t size)
{
    mem->data = data;
    mem->size = size;
    mem->alloc = 0;
}

static inline void *mem_alloc(struct MEM_ARENA *mem, size_t size, size_t align)
{
    size_t base = ((mem->alloc + align - 1) / align) * align;
    if (base + size > mem->size) {
        printf("ERROR: out of arena memory allocating %zu bytes (align %zu), %zu/%zu in arena\n",
               size, align, mem->alloc, mem->size);
        fflush(stdout);
        while (1);
    }
    void *p = ((char *)mem->data) + base;
    mem->alloc = base + size;
    return p;
}

#define mem_add(mem, type) mem_alloc((mem), sizeof(type), alignof(type))
#define mem_add_n(mem, type, num) mem_alloc((mem), (num)*sizeof(type), alignof(type))

#endif /* MEM_H_FILE */
