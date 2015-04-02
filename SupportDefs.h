/*
 * SupportDefs.h
 *
 *  Created on: 24/ott/2011
 *      Author: stefano
 */

#ifndef __SUPPORTDEFS_H
#define __SUPPORTDEFS_H

typedef int status_t;
typedef unsigned long long uint64;
typedef int int32;
typedef unsigned int uint32;
typedef signed int sint32;
typedef short int16;
typedef unsigned short uint16;
typedef signed short sint16;
typedef char int8;
typedef unsigned char uint8;
typedef signed char sint8;

#include <stdio.h>

#include <string>
#include <vector>

typedef std::vector<std::string> StringList;
typedef std::vector<std::string>::iterator StringListIterator;

#endif /* __SUPPORTDEFS_H */
