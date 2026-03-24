#include "string.h"
#include "kmemory.h"
int strcmp(const char* a, const char* b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a - *b;
}
int strncmp(const char* a, const char* b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) return a[i] - b[i];
        if (a[i] == '\0') return 0;
    }
    return 0;
}
int strlen(const char *s) {
    int i = 0;
    while (s[i] != '\0') {
        i++;
    }
    return i;
}
void strcpy(char *dest, const char *src) {
    int i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}
void strncpy(char* dest, const char* src, int n) {
    int i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}
char* strcat(char* dest, const char* src) {
    size_t len = strlen(dest);
    size_t i = 0;
    while (src[i] != '\0') {
        dest[len + i] = src[i];
        i++;
    }
    dest[len + i] = '\0';
    return dest;
}
char* strjoin(const char* a, const char* b) {
    size_t len_a = strlen(a);
    size_t len_b = strlen(b);
    char* result = (char*)kmalloc(len_a + len_b + 1);
    kmemcpy(result, a, len_a);
    kmemcpy(result + len_a, b, len_b);
    result[len_a + len_b] = '\0';
    return result;
}