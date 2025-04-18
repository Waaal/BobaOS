#ifndef STRING_H
#define STRING_H

#include "stdint.h"

uint64_t strlen(const char* str);
void strcpy(char* dest, const char* src);
int8_t strcmp(const char* str1, const char* str2);

#endif
