#pragma once
#define PAGE_SIZE 4096
#include <stdint.h>
#include <stddef.h>

extern uint8_t* bitmap;
extern uint64_t total_pages;
extern uint64_t used_pages;
extern uint8_t _kernel_end;
struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

struct multiboot_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;  // 1 = available, 2 = reserved
    uint32_t zero;
};
void pmm_init(uint64_t mem_size);
void pmm_setup(uint64_t multiboot_addr);
void pmm_free_region(uint64_t addr, uint64_t len);
void pmm_reserve_region(uint64_t addr, uint64_t len);
void* pmm_alloc();
void pmm_free(void* addr);