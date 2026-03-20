#include "print.h"
#include "vfs.h"
#include "string.h"
const static size_t NUM_COLS = 80;
const static size_t NUM_ROWS = 25;

struct Char {
    uint8_t character;
    uint8_t color;
};

struct Char* buffer = (struct Char*) 0xb8000;
size_t col = 0;
size_t row = 0;
uint8_t color = PRINT_COLOR_WHITE | PRINT_COLOR_BLACK << 4;

void clear_row(size_t curr_row) {
    struct Char empty = (struct Char) {
        character: ' ',
        color: color,
    };

    for (size_t col = 0; col < NUM_COLS; col++) {
        buffer[col + NUM_COLS * curr_row] = empty;
    }
    col = 0;
    row = 0;
}
void clear_col(size_t curr_row) {
    struct Char empty = (struct Char) {
        character: ' ',
        color: color,
    };
    buffer[col + NUM_COLS * curr_row] = empty;
}

void print_clear() {
    for (size_t i = 0; i < NUM_ROWS; i++) {
        clear_row(i);
    }
}
void print_prompt() {
    uint8_t saved_color = color;
    
    print_set_color(PRINT_COLOR_GREEN, PRINT_COLOR_BLACK);
    print_str("root@keyhole");
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
    print_str(":");
    print_set_color(PRINT_COLOR_BLUE, PRINT_COLOR_BLACK);
    
    char* pwd = vfs_pwd();
    if (strncmp(pwd, "/home/root", 10) == 0) {
        print_char('~');
        print_str(pwd + 10);
    } else {
        print_str(pwd);
    }
    
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
    print_str("$ ");
    
    color = saved_color;
}
void print_clear_char() {
    col--;
    clear_col(row);
}
void print_newline() {
    col = 0;

    if (row < NUM_ROWS - 1) {
        row++;
        return;
    }

    for (size_t crow = 1; crow < NUM_ROWS; crow++) {
        for (size_t col = 0; col < NUM_COLS; col++) {
            struct Char character = buffer[col + NUM_COLS * crow];
            buffer[col + NUM_COLS * (crow - 1)] = character;
        }
    }

    clear_row(NUM_ROWS - 1);
    row = NUM_ROWS - 1;
}

void print_char(char character) {
    if (character == '\n') {
        print_newline();
        return;
    }

    if (col >= NUM_COLS) {
        print_newline();
    }

    buffer[col + NUM_COLS * row] = (struct Char) {
        character: (uint8_t) character,
        color: color,
    };

    col++;
}

void print_str(char* str) {
    for (size_t i = 0; 1; i++) {
        char character = (uint8_t) str[i];

        if (character == '\0') {
            return;
        }

        print_char(character);
    }
}

void print_set_color(uint8_t foreground, uint8_t background) {
    color = foreground + (background << 4);
}