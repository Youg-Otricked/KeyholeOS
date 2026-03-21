#include "print.h"
#include "io.h"
#include "pic.h"
#include "string.h"
#include "vfs.h"
#include "kmemory.h"
const char scancode_to_ascii[128] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,
    0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};
const char scancode_to_ascii_shift[128] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,
    0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};
static int shift_held = 0;
struct idt_entry {
    uint16_t offset_low;    // lower 16 bits of handler address
    uint16_t selector;      // code segment selector (from GDT)
    uint8_t  ist;           // interrupt stack table (0 for now)
    uint8_t  type_attr;     // gate type + attributes
    uint16_t offset_mid;    // middle 16 bits of handler address
    uint32_t offset_high;   // upper 32 bits of handler address
    uint32_t zero;          // reserved
};
struct idt_pointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));
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
char input_buffer[256];
int input_pos = 0;
struct idt_pointer idtp = {
    .limit = sizeof(idt) - 1,
    .base  = (uint64_t)&idt
};
struct ColorEntry {
    const char* name;
    uint8_t value;
};

struct ColorEntry colors[] = {
    {"black", 0}, {"blue", 1}, {"green", 2}, {"cyan", 3},
    {"red", 4}, {"magenta", 5}, {"brown", 6}, {"gray", 7},
    {"dgray", 8}, {"lblue", 9}, {"lgreen", 10}, {"lcyan", 11},
    {"lred", 12}, {"pink", 13}, {"yellow", 14}, {"white", 15}
};
// load IDT from ASM
extern void load_idt(struct idt_pointer* idtp);
static int extended = 0;
#define HISTORY_SIZE 16
static char history[HISTORY_SIZE][256];
static int history_count = 0;   // total commands ever
static int history_pos = 0;     // current navigation position
static int page_start = 0;      // which command the array starts at
#define PIT_FREQ 100
#define PIT_DIVISOR 1193182 / PIT_FREQ
void history_load_page(int start) {
    // read .ksh_history, find lines starting at 'start'
    char* data;
    int result = vfs_cat("/home/root/.ksh_history", &data);
    if (result != 0 || data == NULL) return;
    
    int line = 0;
    int loaded = 0;
    int pos = 0;
    
    for (int i = 0; data[i] != '\0' && loaded < HISTORY_SIZE; i++) {
        if (data[i] == '\n') {
            if (line >= start) {
                history[loaded][pos] = '\0';
                loaded++;
                pos = 0;
            }
            line++;
        } else if (line >= start) {
            history[loaded][pos++] = data[i];
        }
    }
    page_start = start;
}
void cmd_color(char* name) {
    for (int i = 0; i < 16; i++) {
        if (strcmp(colors[i].name, name) == 0) {
            print_set_color(colors[i].value, PRINT_COLOR_BLACK);
            return;
        }
    }
    print_str("Unknown color\n");
}
void cmd_reboot() {
    uint8_t good = 0x02;
    while (good & 0x02) {
        good = inb(0x64);
    }
    outb(0x64, 0xFE);
    struct idt_pointer null_idt = { .limit = 0, .base = 0 };
    load_idt(&null_idt);
    asm volatile("int $0x00");
}
static uint64_t sudo_expires = 0;
static volatile uint64_t ticks = 0;
void execute(char* input) {
    super_sudo_mode = 0;
    char* commands[] = {"clear", "help", "echo", "uname", "color", "reboot", "panic", "rm", "cd", "ls", "mkdir", "touch", "cat", "pwd", "uptime", "rmdir", "mv", "cp"};
    const int num_commands = sizeof(commands) / sizeof(commands[0]);
    char tokens[16][64];  // 16 tokens, each up to 64 chars
    int token_count = 0;
    int pos = 0;
    for (int i = 0; i < 16; i++) {
        tokens[i][0] = '\0';
    }
    for (int i = 0; input[i] != '\0'; i++) {
        if (input[i] == ' ') {
            tokens[token_count][pos] = '\0';
            token_count++;
            pos = 0;
        } else {
            tokens[token_count][pos] = input[i];
            pos++;
        }
    }
    tokens[token_count][pos] = '\0';
    token_count++;
    int redirect = 0;
    char* redirect_file = NULL;
    for (int i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], ">>") == 0 && i + 1 < token_count) {
            redirect = 2;
            redirect_file = tokens[i + 1];
            token_count = i;
            break;
        } else if (strcmp(tokens[i], ">") == 0 && i + 1 < token_count) {
            redirect = 1;
            redirect_file = tokens[i + 1];
            token_count = i;
            break;
        }
    }
    if (sudo_expires > 0 && ticks > sudo_expires) {
        sudo_mode = 0;
        sudo_expires = 0;
    }
    if (input[0] != '\0' && !sudo_mode) {
        strcpy(history[history_count % HISTORY_SIZE], input);
        history_count++;
        history_pos = history_count;
        
        char history_entry[256];
        strcpy(history_entry, input);
        strcat(history_entry, "\n");
        vfs_write("/home/root/.ksh_history", history_entry, 1);
    }
    if (strcmp(tokens[0], "sudo") == 0) {
        if (token_count < 2) {
            print_str("sudo: missing command\n");
            return;
        }
        sudo_mode = 1;
        sudo_expires = ticks + (PIT_FREQ * 60);
        char sudo_input[256];
        sudo_input[0] = '\0';
        for (int i = 1; i < token_count; i++) {
            if (i > 1) strcat(sudo_input, " ");
            strcat(sudo_input, tokens[i]);
        }
        execute(sudo_input);
    } else if (strcmp(tokens[0], "clear") == 0) {
        print_clear();
    } else if (strcmp(tokens[0], "echo") == 0) {
        char output[512];
        output[0] = '\0';
        for (int i = 1; i < token_count; i++) {
            if (i > 1) strcat(output, " ");
            strcat(output, tokens[i]);
        }
        
        if (redirect) {
            int result = vfs_write(redirect_file, output, redirect == 2);
            if (result == -1) {
                print_str("echo: no such file or directory: ");
                print_str(redirect_file);
                print_char('\n');
            } else if (result == -2) {
                print_str("echo: is a directory: ");
                print_str(redirect_file);
                print_char('\n');
            } else if (result == -3) {
                print_str("echo: permission denied: ");
                print_str(redirect_file);
                print_char('\n');
            }
        } else {
            print_str(output);
            print_char('\n');
        }
    } else if (strcmp(tokens[0], "help") == 0) {
        print_str("Commands: ");
        for (int i = 0; i < num_commands; i++) {
            print_str(commands[i]);
            print_char(' ');
        }
        print_char('\n');
    } else if (strcmp(tokens[0], "uname") == 0) {
        print_str("KeyholeOS V0.1.1:beta\n");
    } else if (strcmp(tokens[0], "reboot") == 0) {
        cmd_reboot();
    } else if (strcmp(tokens[0], "panic") == 0) {
        if (!sudo_mode) {
            print_str("panic: requires `sudo`\n");
        }
        print_str("Kernel panic - system halted\n");
        volatile int x = 1 / 0;
    } else if (strcmp(tokens[0], "color") == 0) {
        if (token_count < 2) {
            print_str("color takes 1 arguments (color <color>)\n");
        } else {
            cmd_color(tokens[1]);
        }
    } else if (strcmp(tokens[0], "pwd") == 0) {
        print_str(vfs_pwd());
        print_char('\n');
    } else if (strcmp(tokens[0], "cd") == 0) {
        if (token_count < 2) {
            print_str("cd takes 1 arguments (cd <path>)\n");
        } else {
            int result = vfs_cd(tokens[1]);
            if (result == -1) {
                print_str("cd: no such file or directory: ");
                print_str(tokens[1]);
                print_char('\n');
            } else if (result == -2) {
                print_str("cd: not a directory: ");
                print_str(tokens[1]);
                print_char('\n');
            }
        }
    } else if (strcmp(tokens[0], "rm") == 0) {
        if (token_count < 2) {
            print_str("rm: missing operand\n");
        } else {
            int recursive = 0;
            char* path = NULL;
            for (int i = 1; i < token_count; i++) {
                if (strcmp(tokens[i], "-r") == 0 || strcmp(tokens[i], "-rf") == 0) {
                    recursive = 1;
                } else if (strcmp(tokens[i], "--no-preserve-root") == 0) {
                    super_sudo_mode = 1;
                } else {
                    path = tokens[i];
                }
            }
            if (path == NULL) {
                print_str("rm: missing operand\n");
            } else {
                int result;
                if (recursive) {
                    result = vfs_rm_r(path);
                } else {
                    result = vfs_rm(path);
                }
                if (result == -1) {
                    print_str("rm: no such file or directory: ");
                    print_str(path);
                    print_char('\n');
                } else if (result == -2) {
                    print_str("rm: is a directory (use -r): ");
                    print_str(path);
                    print_char('\n');
                } else if (result == -3) {
                    print_str("rm: permission denied: ");
                    print_str(path);
                    print_char('\n');
                } else if (result == -4) {
                    print_str("rm: cannot remove `");
                    print_str(path);
                    print_str("`: resource busy\n");
                } else if (result == -5) {
                    print_str("rm: it is dangerous to operate recursively on '/'\nrm: use --no-preserve-root to override this failsafe\n");
                } 
            }
        }
    } else if (strcmp(tokens[0], "cat") == 0) {
        if (token_count < 2) {
            print_str("cat takes 1 arguments (cat <path>)\n");
            return;
        }
        char* data;
        int result = vfs_cat(tokens[1], &data);
        if (result == -1) {
            print_str("cat: ");
            print_str(tokens[1]);
            print_str(": No such file or directory\n");
        } else if (result == -2) {
            print_str("cat: ");
            print_str(tokens[1]);
            print_str(": Is a directory\n");
        } else if (data != NULL) {
            print_str(data);
            print_char('\n');
        }
    } else if (strcmp(tokens[0], "ls") == 0) {
        int show_all = 0;
        char* path = NULL;
        for (int i = 1; i < token_count; i++) {
            if (strcmp(tokens[i], "-a") == 0) show_all = 1;
            else path = tokens[i];
        }
        int recursive = 0;
        for (int i = 1; i < token_count; i++) {
            if (strcmp(tokens[i], "-R") == 0) recursive = 1;
        }

        if (recursive) {
            if (strcmp(path, "") || strcmp(path, " ")) {
                path = ".";
            }
            struct Node* dir = path ? resolve_path(path) : current_dir;
            if (dir == NULL || dir->type != VFS_DIRECTORY) {
                print_str("ls: not a directory\n");
            } else {
                ls_recursive(dir, path ? path : ".", show_all);
            }
        } else {
            struct Node* child = vfs_ls(path);
            if (child == NULL && token_count > 1) {
                struct Node* node = resolve_path(tokens[1]);
                if (node == NULL) {
                    print_str("ls: cannot access '");
                    print_str(tokens[1]);
                    print_str("': No such file or directory\n");
                }
            } else {
                uint8_t saved = color;
                while (child != NULL) {
                    if (child->type == VFS_DIRECTORY) {
                        print_set_color(PRINT_COLOR_BLUE, PRINT_COLOR_BLACK);
                    }
                    if (child->name[0] != '.' || show_all) {
                        print_str(child->name);
                        print_set_color(saved & 0x0F, saved >> 4);
                        print_char(' ');
                    }
                    child = child->next_sibling;
                }
                print_char('\n');
                color = saved;
            }
        }
    } else if (strcmp(tokens[0], "mkdir") == 0) {
        if (token_count < 2) {
            print_str("mkdir: missing operand\n");
        } else {
            struct Node* result = vfs_mkdir(tokens[1]);
            if (result == NULL) {
                print_str("mkdir: cannot create directory '");
                print_str(tokens[1]);
                print_str("': No such file or directory\n");
            }
        }
    } else if (strcmp(tokens[0], "touch") == 0) {
        if (token_count < 2) {
            print_str("touch: missing operand\n");
        } else {
            struct Node* result = vfs_touch(tokens[1]);
            if (result == NULL) {
                print_str("touch: cannot touch '");
                print_str(tokens[1]);
                print_str("': No such file or directory\n");
            }
        }
    } else if (strcmp(tokens[0], "history") == 0) {
        char* data;
        int result = vfs_cat("/home/root/.ksh_history", &data);
        if (result == 0 && data != NULL) {
            print_str(data);
        }
    } else if (strcmp(tokens[0], "uptime") == 0) {
        uint64_t seconds = ticks / PIT_FREQ;
        uint64_t minutes = seconds / 60;
        seconds = seconds % 60;
        print_int(minutes);
        print_char(':');
        print_int(seconds);
        print_char('\n');
    } else if (strcmp(tokens[0], "rmdir") == 0) {
        if (token_count < 2) {
            print_str("rmdir: missing operand\n");
        } else {
            int result = vfs_rmdir(tokens[1]);
            if (result == -1) {
                print_str("rmdir: no such directory: ");
                print_str(tokens[1]);
                print_char('\n');
            } else if (result == -2) {
                print_str("rmdir: not a directory: ");
                print_str(tokens[1]);
                print_char('\n');
            } else if (result == -3) {
                print_str("rmdir: directory not empty: ");
                print_str(tokens[1]);
                print_char('\n');
            } else if (result == -4) {
                print_str("rmdir: permission denied: ");
                print_str(tokens[1]);
                print_char('\n');
            } else if (result == -5) {
                print_str("rmdir: cannot remove `");
                print_str(tokens[1]);
                print_str("`: resource busy\n");
            }
        }
    } else if (strcmp(tokens[0], "mv") == 0) {
        if (token_count < 3) {
            print_str("mv: missing operand (mv <src> <dest>)\n");
        } else {
            int result = vfs_mv(tokens[1], tokens[2]);
            if (result == -1) {
                print_str("mv: no such file or directory: ");
                print_str(tokens[1]);
                print_char('\n');
            } else if (result == -3) {
                print_str("mv: permission denied: ");
                print_str(tokens[1]);
                print_char('\n');
            } else if (result == -4) {
                print_str("mv: permission denied (destination): ");
                print_str(tokens[2]);
                print_char('\n');
            }
        }
    } else if (strcmp(tokens[0], "cp") == 0) {
        if (token_count < 3) {
            print_str("cp: missing operand (cp <src> <dest>)\n");
        } else {
            int recursive = 0;
            char* src = NULL;
            char* dest = NULL;
            for (int i = 1; i < token_count; i++) {
                if (strcmp(tokens[i], "-r") == 0) recursive = 1;
                else if (src == NULL) src = tokens[i];
                else dest = tokens[i];
            }
            if (src == NULL || dest == NULL) {
                print_str("cp: missing operand (cp <src> <dest>)\n");
            } else {
                int result;
                if (recursive) {
                    result = vfs_cp_r(src, dest);
                } else {
                    result = vfs_cp(src, dest);
                }
                if (result == -1) {
                    print_str("cp: no such file or directory: ");
                    print_str(src);
                    print_char('\n');
                } else if (result == -2) {
                    print_str("cp: is a directory (use -r): ");
                    print_str(src);
                    print_char('\n');
                } else if (result == -3) {
                    print_str("cp: permission denied: ");
                    print_str(dest);
                    print_char('\n');
                }
            }
        }
    } else {
        print_str("Unknown command: ");
        print_str(input);
        print_char('\n');
        return;
    }
}
int caps_lock = 0;
int ctrl_held = 0;
void interrupt_handler(uint64_t* regs) {
    uint64_t int_num = regs[15];
    // CPU exceptions (0-21)

    if (int_num < 32) {
        print_set_color(PRINT_COLOR_RED, PRINT_COLOR_BLACK);
        switch (int_num) {
            case 0:  print_str("DIVIDE BY ZERO"); break;
            case 6:  print_str("INVALID OPCODE"); break;
            case 8:  print_str("DOUBLE FAULT"); break;
            case 13: print_str("GENERAL PROTECTION FAULT"); break;
            case 14: print_str("PAGE FAULT"); break;
            default: print_str("CPU EXCEPTION"); break;
        }
        print_str("\nSystem halted. Reboot to continue.\n");
        while (1) { asm volatile("hlt"); }
    }
    if (int_num == 32) {  // timer
        ticks++;
    }
    if (int_num == 33) {
        const uint8_t scancode = inb(0x60);
        if (scancode == 0x3A) {  // caps lock
            caps_lock = !caps_lock;
            pic_eoi(int_num - 32);
            return;
        }
        if (scancode == 0x2A || scancode == 0x36) {
            shift_held = 1;
            pic_eoi(int_num - 32);
            return;
        }
        if (scancode == 0x1D) {
            ctrl_held = 1;
            pic_eoi(int_num - 32);
            return;
        }
        if ((scancode & 0x7F) == 0x1D && (scancode & 0x80)) {
            ctrl_held = 0;
            pic_eoi(int_num - 32);
            return;
        }
        if (scancode == 0xAA || scancode == 0xB6) {
            shift_held = 0;
            pic_eoi(int_num - 32);
            return;
        }
        if (scancode == 0xE0) {
            extended = 1;
            pic_eoi(int_num - 32);
            return;
        }
        if (scancode & 0x80) {
            pic_eoi(int_num - 32);
            return;
        }
        if (ctrl_held && scancode < 128) {
            if (scancode == 0x2E) {
                print_str("^C\n");
                input_pos = 0;
                input_buffer[0] = '\0';
                print_prompt();
                pic_eoi(int_num - 32);
                return;
            }
            if (scancode == 0x26) {  // l = clear screen
                print_clear();
                print_prompt();
                pic_eoi(int_num - 32);
                return;
            }
            pic_eoi(int_num - 32);
            return;
        }
        if (extended) {
            extended = 0;
            if (scancode == 0x1D) {  // right ctrl press
                ctrl_held = 1;
                pic_eoi(int_num - 32);
                return;
            }
            if (scancode == 0x9D) {  // right ctrl release
                ctrl_held = 0;
                pic_eoi(int_num - 32);
                return;
            }
            if (scancode == 0x48) {  // up arrow
                if (history_pos > 0) {
                    history_pos--;
                    if (history_pos < page_start) {
                        int new_start = history_pos - HISTORY_SIZE + 1;
                        if (new_start < 0) new_start = 0;
                        history_load_page(new_start);
                    }
                    while (input_pos > 0) { input_pos--; print_clear_char(); }
                    strcpy(input_buffer, history[(history_pos - page_start) % HISTORY_SIZE]);
                    input_pos = strlen(input_buffer);
                    print_str(input_buffer);
                }
            } else if (scancode == 0x50) {  // down arrow
                if (history_pos < history_count - 1) {
                    history_pos++;
                    if (history_pos >= page_start + HISTORY_SIZE) {
                        history_load_page(history_pos);
                    }
                    while (input_pos > 0) { input_pos--; print_clear_char(); }
                    strcpy(input_buffer, history[(history_pos - page_start) % HISTORY_SIZE]);
                    input_pos = strlen(input_buffer);
                    print_str(input_buffer);
                } else if (history_pos == history_count - 1) {
                    history_pos = history_count;
                    while (input_pos > 0) { input_pos--; print_clear_char(); }
                    input_buffer[0] = '\0';
                    input_pos = 0;
                }
            }
            pic_eoi(int_num - 32);
            return;
        }
        if (scancode < 128) {
            char c = shift_held ? scancode_to_ascii_shift[scancode] : scancode_to_ascii[scancode];

            if (caps_lock && c >= 'a' && c <= 'z') {
                c -= 32;
            } else if (caps_lock && c >= 'A' && c <= 'Z') {
                c += 32;
            }
            if (scancode == 0x0E) {
                if (input_pos > 0) {
                    input_pos--;
                    print_clear_char();
                }
            }
            else if (c != 0) {
                if (c == '\n') {
                    input_buffer[input_pos] = '\0';
                    print_char('\n');
                    execute(input_buffer);
                    input_pos = 0;
                    print_prompt();
                } else if (c != 0) {
                    input_buffer[input_pos++] = c;
                    print_char(c);
                }
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
    heap_init();
    init_filesystem();
    outb(0x43, 0x36);
    outb(0x40, PIT_DIVISOR & 0xFF);
    outb(0x40, (PIT_DIVISOR >> 8) & 0xFF);
    print_set_color(PRINT_COLOR_YELLOW, PRINT_COLOR_BLACK);
    print_str("Welcome to KeyholeOS\n");
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
    print_prompt();
}