#pragma once
#include <stdint.h>

void io_wait();
void pic_init();
void pic_eoi(uint8_t irq);