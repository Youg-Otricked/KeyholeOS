#include "tss.h"


static struct TSS tss;

void tss_init(void) {
    uint8_t* ptr = (uint8_t*)&tss;
    for (int i = 0; i < sizeof(struct TSS); i++) {
        ptr[i] = 0;
    }
    extern uint8_t stack_top;
    tss.rsp0 = (uint64_t)&stack_top;
    tss.iopb_offset = sizeof(struct TSS);
}
void tss_install(void) {
    tss_init();
    uint64_t tss_addr = (uint64_t)&tss;
    uint16_t tss_limit = sizeof(struct TSS) - 1;
    extern uint8_t gdt64;
    uint64_t* gdt = (uint64_t*)&gdt64;
    gdt[2] = (tss_limit & 0xFFFF) |
             ((tss_addr & 0xFFFF) << 16) |
             ((tss_addr >> 16 & 0xFF) << 32) |
             ((uint64_t)0x89 << 40) |
             ((uint64_t)(tss_limit >> 16 & 0xF) << 48) |
             ((tss_addr >> 24 & 0xFF) << 56);
    gdt[3] = (tss_addr >> 32) & 0xFFFFFFFF;
    load_tss(0x10);
}