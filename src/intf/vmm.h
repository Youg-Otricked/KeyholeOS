#pragma once
#include <stdint.h>
#include <stddef.h>
extern uint64_t read_cr3(void);          // no args, returns physical address
extern void write_cr3(uint64_t addr);    // takes physical address, no return
extern void invlpg(uint64_t virt_addr);  // takes virtual address, no return

void vmm_init();
int vmm_map(uint64_t virt, uint64_t phys, uint64_t flags);
void vmm_unmap(uint64_t virt);
uint64_t vmm_get_phys(uint64_t virt);

// Page flags
#define VMM_PRESENT  (1 << 0)
#define VMM_WRITABLE (1 << 1)
#define VMM_USER     (1 << 2)

