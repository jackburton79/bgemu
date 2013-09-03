#ifndef __UTILS_H
#define __UTILS_H

#include <stdio.h>
#include <string>

#include "IETypes.h"

#define SGN(x) ((x) > 0 ? 1 : ((x) == 0 ? 0 : (-1)))
#define ABS(x) ((x) > 0 ? (x) : (-x))

#ifdef __cplusplus
extern "C" {
#endif

char *trim(char *string);
void path_dos_to_unix(char *path);
FILE *fopen_case(const char *name, const char *flags);
const char *extension(const char *name);

#ifdef __cplusplus
}
#endif


#endif
