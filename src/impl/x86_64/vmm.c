#include "vmm.h"
#include "pmm.h"
static uint64_t* pml4;

void vmm_init() {
    pml4 = (uint64_t*)read_cr3();
}
uint64_t make_table() {
    uint64_t* new_table = (uint64_t*)pmm_alloc();
    for (int i = 0; i < 512; i++) new_table[i] = 0;
    return (uint64_t)new_table | VMM_PRESENT | VMM_WRITABLE;
}
int vmm_map(uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t l4_idx = (virt >> 39) & 0x1FF;
    uint64_t l3_idx = (virt >> 30) & 0x1FF;
    uint64_t l2_idx = (virt >> 21) & 0x1FF;
    uint64_t l1_idx = (virt >> 12) & 0x1FF;
    
    uint64_t* l4_table = pml4;
    
    if (!(l4_table[l4_idx] & VMM_PRESENT)) {
        l4_table[l4_idx] = make_table();
    }
    if (flags & VMM_USER) {
        l4_table[l4_idx] |= VMM_USER;
    }
    uint64_t* l3_table = (uint64_t*)(l4_table[l4_idx] & 0x000FFFFFFFFFF000);
    
    if (!(l3_table[l3_idx] & VMM_PRESENT)) {
        l3_table[l3_idx] = make_table();
    }
    if (flags & VMM_USER) {
        l3_table[l3_idx] |= VMM_USER;
    }
    uint64_t* l2_table = (uint64_t*)(l3_table[l3_idx] & 0x000FFFFFFFFFF000);
    
    if (!(l2_table[l2_idx] & VMM_PRESENT)) {
        l2_table[l2_idx] = make_table();
    }
    if (flags & VMM_USER) {
        l2_table[l2_idx] |= VMM_USER;
    }
    uint64_t* l1_table = (uint64_t*)(l2_table[l2_idx] & 0x000FFFFFFFFFF000);
    l1_table[l1_idx] = phys | flags;
    
    invlpg(virt);
    return 0;
}

void vmm_unmap(uint64_t virt) {
    uint64_t l4_idx = (virt >> 39) & 0x1FF;
    uint64_t l3_idx = (virt >> 30) & 0x1FF;
    uint64_t l2_idx = (virt >> 21) & 0x1FF;
    uint64_t l1_idx = (virt >> 12) & 0x1FF;
    
    uint64_t* l4_table = pml4;
    if (!(l4_table[l4_idx] & VMM_PRESENT)) return;
    
    uint64_t* l3_table = (uint64_t*)(l4_table[l4_idx] & 0x000FFFFFFFFFF000);
    if (!(l3_table[l3_idx] & VMM_PRESENT)) return;
    
    uint64_t* l2_table = (uint64_t*)(l3_table[l3_idx] & 0x000FFFFFFFFFF000);
    if (!(l2_table[l2_idx] & VMM_PRESENT)) return;
    
    uint64_t* l1_table = (uint64_t*)(l2_table[l2_idx] & 0x000FFFFFFFFFF000);
    
    l1_table[l1_idx] = 0;
    invlpg(virt);
}

uint64_t vmm_get_phys(uint64_t virt) {
    uint64_t l4_idx = (virt >> 39) & 0x1FF;
    uint64_t l3_idx = (virt >> 30) & 0x1FF;
    uint64_t l2_idx = (virt >> 21) & 0x1FF;
    uint64_t l1_idx = (virt >> 12) & 0x1FF;
    
    uint64_t* l4_table = pml4;
    if (!(l4_table[l4_idx] & VMM_PRESENT)) return 0;
    
    uint64_t* l3_table = (uint64_t*)(l4_table[l4_idx] & 0x000FFFFFFFFFF000);
    if (!(l3_table[l3_idx] & VMM_PRESENT)) return 0;
    
    uint64_t* l2_table = (uint64_t*)(l3_table[l3_idx] & 0x000FFFFFFFFFF000);
    if (!(l2_table[l2_idx] & VMM_PRESENT)) return 0;
    
    uint64_t* l1_table = (uint64_t*)(l2_table[l2_idx] & 0x000FFFFFFFFFF000);
    if (!(l1_table[l1_idx] & VMM_PRESENT)) return 0;
    
    return (l1_table[l1_idx] & 0x000FFFFFFFFFF000) | (virt & 0xFFF);
}