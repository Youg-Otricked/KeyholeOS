#include "userspacetest.h"
void user_function() {
    // sys_write(1, "Hello from ring 3!\n", 19)
    asm volatile(
        "mov $1, %%rax\n"    // syscall 1 = write
        "mov $1, %%rdi\n"    // fd = stdout
        "mov %0, %%rsi\n"    // buffer
        "mov $19, %%rdx\n"   // length
        "int $0x80\n"
        :
        : "r"("Hello from ring 3!\n")
        : "rax", "rdi", "rsi", "rdx"
    );
    
    // sys_exit(0)
    asm volatile(
        "mov $60, %%rax\n"    // syscall 0 = exit
        "mov $0, %%rdi\n"    // exit code 0
        "int $0x80\n" /// WHY GPF ON syscall AAAAAAAAAAAAAAAA
        :
        :
        : "rax", "rdi"
    );
}