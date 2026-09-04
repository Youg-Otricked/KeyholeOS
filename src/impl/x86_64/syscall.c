#include "syscall.h"
#include "print.h"

int64_t sys_exit(uint64_t code) {
    print_str("Process exited with code ");
    print_int(code);
    print_char('\n');
    print_prompt();
    asm volatile("sti");
    for (;;) {asm volatile("hlt");}
    return 0;
}

int64_t sys_write(uint64_t fd, uint64_t buf, uint64_t len) {
    if (fd == 1) {
        char* str = (char*)buf;
        for (uint64_t i = 0; i < len; i++) {
            print_char(str[i]);
        }
        return len;
    }
    return -1;
}

int64_t handle_syscall(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    switch (num) {
        case 60: return sys_exit(arg1);
        case 1: return sys_write(arg1, arg2, arg3);
        default: return -1;
    }
}
void syscall_init() {
    uint64_t star = ((uint64_t)0x08 << 32) | ((uint64_t)0x20 << 48);
    wrmsr(0xC0000081, star);
    wrmsr(0xC0000082, (uint64_t)syscall_entry);
    wrmsr(0xC0000084, 0x200);
    uint64_t efer = rdmsr(0xC0000080);
    wrmsr(0xC0000080, efer | 1);
}
