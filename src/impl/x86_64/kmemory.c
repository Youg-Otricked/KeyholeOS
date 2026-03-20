#include "kmemory.h"
#include <stdint.h>
struct Block {
    size_t size;
    int free;
    struct Block* next;
};

static uint8_t heap[1024 * 1024];  // 1MB
static struct Block* free_list = NULL;

void heap_init() {
    free_list = (struct Block*)heap;
    free_list->size = sizeof(heap) - sizeof(struct Block);
    free_list->free = 1;
    free_list->next = NULL;
}

void* kmalloc(size_t size) {
    struct Block* curr = free_list;
    while (curr) {
        if (curr->free && curr->size >= size) {
            if (curr->size > size + sizeof(struct Block) + 16) {
                struct Block* new_block = (struct Block*)((uint8_t*)(curr + 1) + size);
                new_block->size = curr->size - size - sizeof(struct Block);
                new_block->free = 1;
                new_block->next = curr->next;
                curr->size = size;
                curr->next = new_block;
            }
            curr->free = 0;
            return (void*)(curr + 1);
        }
        curr = curr->next;
    }
    return NULL;
}

void kfree(void* ptr) {
    struct Block* block = (struct Block*)ptr - 1;
    block->free = 1;
}
void kmemcpy(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
}