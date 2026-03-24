#pragma once
#include <stddef.h>
#include <stdint.h>
extern uint64_t heap_start;
extern uint64_t heap_end;
extern uint64_t heap_max;
void heap_init();

void* kmalloc(size_t size);

void kfree(void* ptr);
void kmemcpy(void* dest, const void* src, size_t n);