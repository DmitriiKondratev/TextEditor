#ifndef STRING_H_INCLUDED
#define STRING_H_INCLUDED

#define BASE_STRING_SIZE 50

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

typedef struct String_tag {
    size_t size;
    size_t len;
    char* data;
} String;

String* CreateString(const char* src);
void DestroyString(String** ppStr);

int ReserveSize(String* str, size_t size);

int AddString(String* str, const char* src);

size_t PrintString(const String* str);

#endif // STRING_H_INCLUDED