#pragma once
#include <stdint.h>

int64_t handle_syscall(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3);
int64_t sys_exit(uint64_t code);
int64_t sys_write(uint64_t fd, uint64_t buf, uint64_t len);
extern void syscall_entry(void);
extern void wrmsr(uint64_t msr, uint64_t value);
extern uint64_t rdmsr(uint64_t msr);
void syscall_init(void);