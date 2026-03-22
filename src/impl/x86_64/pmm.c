#include "pmm.h"
#include "print.h"

uint8_t* bitmap;
uint64_t total_pages;
uint64_t used_pages;


void pmm_init(uint64_t mem_size) {
    total_pages = mem_size / PAGE_SIZE;
    uint64_t bitmap_size = total_pages / 8;
    bitmap = (uint8_t*)0x300000;
    for (uint64_t i = 0; i < bitmap_size; i++) {
        bitmap[i] = 0xFF;
    }
    used_pages = total_pages;
}
void pmm_setup(uint64_t multiboot_addr) {
    uint64_t total_memory = 0;
    struct multiboot_tag* tag = (struct multiboot_tag*)(multiboot_addr + 8);
    
    while (tag->type != 0) {
        if (tag->type == 6) {
            uint32_t entry_size = *(uint32_t*)((uint8_t*)tag + 8);
            uint8_t* ptr = (uint8_t*)tag + 16;
            uint8_t* end = (uint8_t*)tag + tag->size;
            
            while (ptr < end) {
                struct multiboot_mmap_entry* entry = (struct multiboot_mmap_entry*)ptr;
                if (entry->type == 1) {
                    uint64_t region_end = entry->addr + entry->len;
                    if (region_end > total_memory) {
                        total_memory = region_end;
                    }
                }
                ptr += entry_size;
            }
        }
        tag = (struct multiboot_tag*)((uint8_t*)tag + ((tag->size + 7) & ~7));
    }
    pmm_init(total_memory);
    tag = (struct multiboot_tag*)(multiboot_addr + 8);
    while (tag->type != 0) {
        if (tag->type == 6) {
            uint32_t entry_size = *(uint32_t*)((uint8_t*)tag + 8);
            uint8_t* ptr = (uint8_t*)tag + 16;
            uint8_t* end = (uint8_t*)tag + tag->size;
            
            while (ptr < end) {
                struct multiboot_mmap_entry* entry = (struct multiboot_mmap_entry*)ptr;
                if (entry->type == 1) {
                    pmm_free_region(entry->addr, entry->len);
                }
                ptr += entry_size;
            }
        }
        tag = (struct multiboot_tag*)((uint8_t*)tag + ((tag->size + 7) & ~7));
    }
    pmm_reserve_region(0, 0x100000);
    pmm_reserve_region(0x100000, 0x200000);
}
void pmm_free_region(uint64_t addr, uint64_t len) {
    uint64_t start_page = addr / PAGE_SIZE;
    uint64_t num_pages = len / PAGE_SIZE;
    
    for (uint64_t i = 0; i < num_pages; i++) {
        uint64_t page = start_page + i;
        bitmap[page / 8] &= ~(1 << (page % 8));
        used_pages--;
    }
}
void pmm_reserve_region(uint64_t addr, uint64_t len) {
    uint64_t start_page = addr / PAGE_SIZE;
    uint64_t num_pages = (len + PAGE_SIZE - 1) / PAGE_SIZE;
    
    for (uint64_t i = 0; i < num_pages; i++) {
        uint64_t page = start_page + i;
        bitmap[page / 8] |= (1 << (page % 8));
        used_pages++;
    }
}
void* pmm_alloc() {
    for (uint64_t i = 0; i < total_pages / 8; i++) {
        if (bitmap[i] != 0xFF) {
            for (int j = 0; j < 8; j++) {
                if (!(bitmap[i] & (1 << j))) {
                    bitmap[i] |= (1 << j);
                    used_pages++;
                    return (void*)((i * 8 + j) * PAGE_SIZE);
                }
            }
        }
    }
    return NULL;
}
void pmm_free(void* addr) {
    uint64_t page = (uint64_t)addr / PAGE_SIZE;
    bitmap[page / 8] &= ~(1 << (page % 8));
    used_pages--;
}