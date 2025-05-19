#ifndef STRING_H
#define STRING_H

#include "stdint.h"

uint64_t strlen(const char* str);
void strcpy(char* dest, const char* src);
void strncpy(char* dest, const char* src, uint64_t size);
int8_t strcmp(const char* str1, const char* str2);
int8_t strncmp(const char* str1, const char* str2, uint64_t len);
uint64_t strHash(const char* str);

#endif
