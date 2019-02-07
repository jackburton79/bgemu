#ifndef __UTILS_H
#define __UTILS_H

#include <stdio.h>
#include <string>


#ifdef __cplusplus
extern "C" {
#endif

const char* trim(char* string);
void path_dos_to_unix(char* path);
FILE* fopen_case(const char* name, const char* flags);
const char* extension(const char* name);
bool is_bit_set(int i, int bitPos);
void assert_size(size_t size, size_t controlValue);

#ifdef __cplusplus
}
#endif


#endif
