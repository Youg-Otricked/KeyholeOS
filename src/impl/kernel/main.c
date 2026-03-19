#include "print.h"
#include "io.h"
#include "pic.h"
struct idt_entry {
    uint16_t offset_low;    // lower 16 bits of handler address
    uint16_t selector;      // code segment selector (from GDT)
    uint8_t  ist;           // interrupt stack table (0 for now)
    uint8_t  type_attr;     // gate type + attributes
    uint16_t offset_mid;    // middle 16 bits of handler address
    uint32_t offset_high;   // upper 32 bits of handler address
    uint32_t zero;          // reserved
};
const char scancode_to_ascii[128] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,
    0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};
struct idt_entry idt[256];
void idt_set_entry(int index, uint64_t handler) {
    idt[index].offset_low  = handler & 0xFFFF;
    idt[index].selector    = 0x08;
    idt[index].ist         = 0;
    idt[index].type_attr   = 0x8E;
    idt[index].offset_mid  = (handler >> 16) & 0xFFFF;
    idt[index].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[index].zero        = 0;
}
struct idt_pointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct idt_pointer idtp = {
    .limit = sizeof(idt) - 1,
    .base  = (uint64_t)&idt
};
void interrupt_handler(uint64_t* regs) {
    uint64_t int_num = regs[15];
    // CPU exceptions (0-21)

    if (int_num < 32) {
        print_clear();
        print_set_color(PRINT_COLOR_RED, PRINT_COLOR_BLACK);
        switch (int_num) {
            case 0:  print_str("DIVIDE BY ZERO"); break;
            case 6:  print_str("INVALID OPCODE"); break;
            case 8:  print_str("DOUBLE FAULT"); break;
            case 13: print_str("GENERAL PROTECTION FAULT"); break;
            case 14: print_str("PAGE FAULT"); break;
            default: print_str("CPU EXCEPTION"); break;
        }
        while (1) {}
    }
    // Hardware exceptions
    if (int_num == 33) {
        print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
        const uint8_t scancode = inb(0x60);
        if (scancode < 128) {
            char c = scancode_to_ascii[scancode];
            if (c != 0) {
                print_char(c);
            }
        }
    }
    if (int_num >= 32) {
        pic_eoi(int_num - 32);
    }
    return;
}
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
// Hardware interupts
extern void isr32();
extern void isr33();
extern void isr34();
extern void isr35();
extern void isr36();
extern void isr37();
extern void isr38();
extern void isr39();
extern void isr40();
extern void isr41();
extern void isr42();
extern void isr43();
extern void isr44();
extern void isr45();
extern void isr46();
extern void isr47();
// load IDT from ASM
extern void load_idt(struct idt_pointer* idtp);

void kernel_main() {
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
    
    // Now remap
    pic_init();
    // set up IDT entries
    idt_set_entry(0, (uint64_t)isr0);
    idt_set_entry(1, (uint64_t)isr1);
    idt_set_entry(2, (uint64_t)isr2);
    idt_set_entry(3, (uint64_t)isr3);
    idt_set_entry(4, (uint64_t)isr4);
    idt_set_entry(5, (uint64_t)isr5);
    idt_set_entry(6, (uint64_t)isr6);
    idt_set_entry(7, (uint64_t)isr7);
    idt_set_entry(8, (uint64_t)isr8);
    idt_set_entry(9, (uint64_t)isr9);
    idt_set_entry(10, (uint64_t)isr10);
    idt_set_entry(11, (uint64_t)isr11);
    idt_set_entry(12, (uint64_t)isr12);
    idt_set_entry(13, (uint64_t)isr13);
    idt_set_entry(14, (uint64_t)isr14);
    idt_set_entry(15, (uint64_t)isr15);
    idt_set_entry(16, (uint64_t)isr16);
    idt_set_entry(17, (uint64_t)isr17);
    idt_set_entry(18, (uint64_t)isr18);
    idt_set_entry(19, (uint64_t)isr19);
    idt_set_entry(20, (uint64_t)isr20);
    idt_set_entry(21, (uint64_t)isr21);
    // Hardware
    idt_set_entry(32, (uint64_t)isr32);
    idt_set_entry(33, (uint64_t)isr33);
    idt_set_entry(34, (uint64_t)isr34);
    idt_set_entry(35, (uint64_t)isr35);
    idt_set_entry(36, (uint64_t)isr36);
    idt_set_entry(37, (uint64_t)isr37);
    idt_set_entry(38, (uint64_t)isr38);
    idt_set_entry(39, (uint64_t)isr39);
    idt_set_entry(40, (uint64_t)isr40);
    idt_set_entry(41, (uint64_t)isr41);
    idt_set_entry(42, (uint64_t)isr42);
    idt_set_entry(43, (uint64_t)isr43);
    idt_set_entry(44, (uint64_t)isr44);
    idt_set_entry(45, (uint64_t)isr45);
    idt_set_entry(46, (uint64_t)isr46);
    idt_set_entry(47, (uint64_t)isr47);
    load_idt(&idtp);
    print_clear();
    print_set_color(PRINT_COLOR_YELLOW, PRINT_COLOR_BLACK);
    print_str("Welcome to KeyholeOS");
}