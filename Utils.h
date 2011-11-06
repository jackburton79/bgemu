#ifndef __UTILS_H
#define __UTILS_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

void path_dos_to_unix(char *path);
FILE *fopen_case(const char *name, const char *flags);
int get_case_names(const char *name, char ***names);
void free_case_names(char **names, int num);
const char *extension(const char *name);
void check_objects_size();

#ifdef __cplusplus
}
#endif

#endif
