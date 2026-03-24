#pragma once
#include <stddef.h>
#include <stdint.h>
int strcmp(const char* a, const char* b);
int strncmp(const char* a, const char* b, int n);
int strlen(const char *s);
void strcpy(char *dest, const char *src);
char* strcat(char* dest, const char* src);
void strncpy(char* dest, const char* src, int n);
char* strjoin(const char* a, const char* b);