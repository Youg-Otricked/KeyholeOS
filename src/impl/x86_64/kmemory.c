#include "kmemory.h"
#include "vmm.h"
#include "pmm.h"
struct Block {
    size_t size;
    int free;
    struct Block* next;
};

uint64_t heap_start = 0x1000000;
uint64_t heap_end = 0x1000000;
uint64_t heap_max = 0x10000000;
static struct Block* free_list = NULL;

void heap_init() {
    // allocate first physical page
    uint64_t phys = (uint64_t)pmm_alloc();
    vmm_map(heap_start, phys, VMM_PRESENT | VMM_WRITABLE);
    heap_end = heap_start + PAGE_SIZE;
    free_list = (struct Block*)heap_start;
    free_list->size = PAGE_SIZE - sizeof(struct Block);
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
    if (heap_end >= heap_max) return NULL;
    size_t needed = size + sizeof(struct Block);
    size_t pages = (needed + PAGE_SIZE - 1) / PAGE_SIZE;
    
    uint64_t block_start = heap_end;
    for (size_t i = 0; i < pages; i++) {
        uint64_t phys = (uint64_t)pmm_alloc();
        if (phys == 0) return NULL;
        vmm_map(heap_end, phys, VMM_PRESENT | VMM_WRITABLE);
        heap_end += PAGE_SIZE;
    }
    struct Block* new_block = (struct Block*)block_start;
    new_block->size = (pages * PAGE_SIZE) - sizeof(struct Block);
    new_block->free = 1;
    new_block->next = NULL;
    struct Block* last = free_list;
    while (last->next) last = last->next;
    last->next = new_block;
    return kmalloc(size);
}

void kfree(void* ptr) {
    struct Block* block = (struct Block*)ptr - 1;
    block->free = 1;
    if (block->next && block->next->free) {
        block->size += sizeof(struct Block) + block->next->size;
        block->next = block->next->next;
    }
}
void kmemcpy(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
}