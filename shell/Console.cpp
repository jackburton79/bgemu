/*
 * Copyright 2010-2022, Stefano Ceccherini
 *
 * Copyright 2005-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "Console.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>


#define CHAR_WIDTH 6
#define CHAR_HEIGHT 12

static unsigned char sFonts[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* ' ' */
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' ' */
0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x08, 0x00, 0x00, 0x00, /* '!' */
0x00, 0x14, 0x14, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* '"' */
0x00, 0x00, 0x14, 0x14, 0x3e, 0x14, 0x3e, 0x14, 0x14, 0x00, 0x00, 0x00, /* '#' */
0x00, 0x00, 0x08, 0x3c, 0x0a, 0x1c, 0x28, 0x1e, 0x08, 0x00, 0x00, 0x00, /* '$' */
0x00, 0x00, 0x06, 0x26, 0x10, 0x08, 0x04, 0x32, 0x30, 0x00, 0x00, 0x00, /* '%' */
0x00, 0x00, 0x1c, 0x02, 0x02, 0x04, 0x2a, 0x12, 0x2c, 0x00, 0x00, 0x00, /* '&' */
0x00, 0x18, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ''' */
0x20, 0x10, 0x10, 0x08, 0x08, 0x08, 0x08, 0x08, 0x10, 0x10, 0x20, 0x00, /* '(' */
0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x08, 0x08, 0x04, 0x04, 0x02, 0x00, /* ')' */
0x00, 0x00, 0x00, 0x08, 0x2a, 0x1c, 0x2a, 0x08, 0x00, 0x00, 0x00, 0x00, /* '*' */
0x00, 0x00, 0x00, 0x08, 0x08, 0x3e, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, /* '+' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x08, 0x04, 0x00, /* ',' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* '-' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, /* '.' */
0x20, 0x20, 0x10, 0x10, 0x08, 0x08, 0x04, 0x04, 0x02, 0x02, 0x00, 0x00, /* '/' */
0x00, 0x1c, 0x22, 0x32, 0x2a, 0x26, 0x22, 0x22, 0x1c, 0x00, 0x00, 0x00, /* '0' */
0x00, 0x08, 0x0c, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, /* '1' */
0x00, 0x1c, 0x22, 0x20, 0x10, 0x08, 0x04, 0x02, 0x3e, 0x00, 0x00, 0x00, /* '2' */
0x00, 0x1c, 0x22, 0x20, 0x18, 0x20, 0x20, 0x22, 0x1c, 0x00, 0x00, 0x00, /* '3' */
0x00, 0x20, 0x30, 0x28, 0x24, 0x22, 0x3e, 0x20, 0x20, 0x00, 0x00, 0x00, /* '4' */
0x00, 0x3e, 0x02, 0x02, 0x1e, 0x20, 0x20, 0x22, 0x1c, 0x00, 0x00, 0x00, /* '5' */
0x00, 0x18, 0x04, 0x02, 0x1e, 0x22, 0x22, 0x22, 0x1c, 0x00, 0x00, 0x00, /* '6' */
0x00, 0x3e, 0x22, 0x20, 0x20, 0x10, 0x10, 0x08, 0x08, 0x00, 0x00, 0x00, /* '7' */
0x00, 0x1c, 0x22, 0x22, 0x1c, 0x22, 0x22, 0x22, 0x1c, 0x00, 0x00, 0x00, /* '8' */
0x00, 0x1c, 0x22, 0x22, 0x22, 0x3c, 0x20, 0x10, 0x0c, 0x00, 0x00, 0x00, /* '9' */
0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, /* ':' */
0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x08, 0x04, 0x00, /* ';' */
0x00, 0x00, 0x00, 0x30, 0x0c, 0x03, 0x0c, 0x30, 0x00, 0x00, 0x00, 0x00, /* '<' */
0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, /* '=' */
0x00, 0x00, 0x00, 0x03, 0x0c, 0x30, 0x0c, 0x03, 0x00, 0x00, 0x00, 0x00, /* '>' */
0x00, 0x1c, 0x22, 0x20, 0x10, 0x08, 0x08, 0x00, 0x08, 0x00, 0x00, 0x00, /* '?' */
0x00, 0x00, 0x1c, 0x22, 0x3a, 0x3a, 0x1a, 0x02, 0x1c, 0x00, 0x00, 0x00, /* '@' */
0x00, 0x00, 0x08, 0x14, 0x22, 0x22, 0x3e, 0x22, 0x22, 0x00, 0x00, 0x00, /* 'A' */
0x00, 0x00, 0x1e, 0x22, 0x22, 0x1e, 0x22, 0x22, 0x1e, 0x00, 0x00, 0x00, /* 'B' */
0x00, 0x00, 0x1c, 0x22, 0x02, 0x02, 0x02, 0x22, 0x1c, 0x00, 0x00, 0x00, /* 'C' */
0x00, 0x00, 0x0e, 0x12, 0x22, 0x22, 0x22, 0x12, 0x0e, 0x00, 0x00, 0x00, /* 'D' */
0x00, 0x00, 0x3e, 0x02, 0x02, 0x1e, 0x02, 0x02, 0x3e, 0x00, 0x00, 0x00, /* 'E' */
0x00, 0x00, 0x3e, 0x02, 0x02, 0x1e, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, /* 'F' */
0x00, 0x00, 0x1c, 0x22, 0x02, 0x32, 0x22, 0x22, 0x3c, 0x00, 0x00, 0x00, /* 'G' */
0x00, 0x00, 0x22, 0x22, 0x22, 0x3e, 0x22, 0x22, 0x22, 0x00, 0x00, 0x00, /* 'H' */
0x00, 0x00, 0x3e, 0x08, 0x08, 0x08, 0x08, 0x08, 0x3e, 0x00, 0x00, 0x00, /* 'I' */
0x00, 0x00, 0x38, 0x20, 0x20, 0x20, 0x22, 0x22, 0x1c, 0x00, 0x00, 0x00, /* 'J' */
0x00, 0x00, 0x22, 0x12, 0x0a, 0x06, 0x0a, 0x12, 0x22, 0x00, 0x00, 0x00, /* 'K' */
0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x3e, 0x00, 0x00, 0x00, /* 'L' */
0x00, 0x00, 0x22, 0x36, 0x2a, 0x2a, 0x22, 0x22, 0x22, 0x00, 0x00, 0x00, /* 'M' */
0x00, 0x00, 0x22, 0x26, 0x26, 0x2a, 0x32, 0x32, 0x22, 0x00, 0x00, 0x00, /* 'N' */
0x00, 0x00, 0x1c, 0x22, 0x22, 0x22, 0x22, 0x22, 0x1c, 0x00, 0x00, 0x00, /* 'O' */
0x00, 0x00, 0x1e, 0x22, 0x22, 0x1e, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, /* 'P' */
0x00, 0x00, 0x1c, 0x22, 0x22, 0x22, 0x22, 0x22, 0x1c, 0x30, 0x00, 0x00, /* 'Q' */
0x00, 0x00, 0x1e, 0x22, 0x22, 0x1e, 0x0a, 0x12, 0x22, 0x00, 0x00, 0x00, /* 'R' */
0x00, 0x00, 0x1c, 0x22, 0x02, 0x1c, 0x20, 0x22, 0x1c, 0x00, 0x00, 0x00, /* 'S' */
0x00, 0x00, 0x3e, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, /* 'T' */
0x00, 0x00, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x1c, 0x00, 0x00, 0x00, /* 'U' */
0x00, 0x00, 0x22, 0x22, 0x22, 0x14, 0x14, 0x08, 0x08, 0x00, 0x00, 0x00, /* 'V' */
0x00, 0x00, 0x22, 0x22, 0x22, 0x2a, 0x2a, 0x36, 0x22, 0x00, 0x00, 0x00, /* 'W' */
0x00, 0x00, 0x22, 0x22, 0x14, 0x08, 0x14, 0x22, 0x22, 0x00, 0x00, 0x00, /* 'X' */
0x00, 0x00, 0x22, 0x22, 0x14, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, /* 'Y' */
0x00, 0x00, 0x3e, 0x20, 0x10, 0x08, 0x04, 0x02, 0x3e, 0x00, 0x00, 0x00, /* 'Z' */
0x38, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x38, 0x00, /* '[' */
0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x10, 0x10, 0x20, 0x20, 0x00, 0x00, /* '\' */
0x0e, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0e, 0x00, /* ']' */
0x00, 0x08, 0x14, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* '^' */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, /* '_' */
0x00, 0x0c, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* '`' */
0x00, 0x00, 0x00, 0x00, 0x3c, 0x22, 0x22, 0x32, 0x2c, 0x00, 0x00, 0x00, /* 'a' */
0x00, 0x02, 0x02, 0x02, 0x1e, 0x22, 0x22, 0x22, 0x1e, 0x00, 0x00, 0x00, /* 'b' */
0x00, 0x00, 0x00, 0x00, 0x3c, 0x02, 0x02, 0x02, 0x3c, 0x00, 0x00, 0x00, /* 'c' */
0x00, 0x20, 0x20, 0x20, 0x3c, 0x22, 0x22, 0x22, 0x3c, 0x00, 0x00, 0x00, /* 'd' */
0x00, 0x00, 0x00, 0x00, 0x1c, 0x22, 0x3e, 0x02, 0x1c, 0x00, 0x00, 0x00, /* 'e' */
0x00, 0x38, 0x04, 0x04, 0x1e, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, /* 'f' */
0x00, 0x00, 0x00, 0x00, 0x3c, 0x22, 0x22, 0x22, 0x3c, 0x20, 0x20, 0x1c, /* 'g' */
0x00, 0x02, 0x02, 0x02, 0x1e, 0x22, 0x22, 0x22, 0x22, 0x00, 0x00, 0x00, /* 'h' */
0x00, 0x08, 0x08, 0x00, 0x0c, 0x08, 0x08, 0x08, 0x1c, 0x00, 0x00, 0x00, /* 'i' */
0x00, 0x10, 0x10, 0x00, 0x1c, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0e, /* 'j' */
0x00, 0x02, 0x02, 0x02, 0x12, 0x0a, 0x06, 0x0a, 0x12, 0x00, 0x00, 0x00, /* 'k' */
0x00, 0x0c, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x1c, 0x00, 0x00, 0x00, /* 'l' */
0x00, 0x00, 0x00, 0x00, 0x16, 0x2a, 0x2a, 0x2a, 0x22, 0x00, 0x00, 0x00, /* 'm' */
0x00, 0x00, 0x00, 0x00, 0x1a, 0x26, 0x22, 0x22, 0x22, 0x00, 0x00, 0x00, /* 'n' */
0x00, 0x00, 0x00, 0x00, 0x1c, 0x22, 0x22, 0x22, 0x1c, 0x00, 0x00, 0x00, /* 'o' */
0x00, 0x00, 0x00, 0x00, 0x1e, 0x22, 0x22, 0x22, 0x1e, 0x02, 0x02, 0x02, /* 'p' */
0x00, 0x00, 0x00, 0x00, 0x3c, 0x22, 0x22, 0x22, 0x3c, 0x20, 0x20, 0x20, /* 'q' */
0x00, 0x00, 0x00, 0x00, 0x1a, 0x06, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, /* 'r' */
0x00, 0x00, 0x00, 0x00, 0x3c, 0x02, 0x1c, 0x20, 0x1e, 0x00, 0x00, 0x00, /* 's' */
0x00, 0x08, 0x08, 0x08, 0x3e, 0x08, 0x08, 0x08, 0x30, 0x00, 0x00, 0x00, /* 't' */
0x00, 0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x32, 0x2c, 0x00, 0x00, 0x00, /* 'u' */
0x00, 0x00, 0x00, 0x00, 0x36, 0x14, 0x14, 0x08, 0x08, 0x00, 0x00, 0x00, /* 'v' */
0x00, 0x00, 0x00, 0x00, 0x22, 0x2a, 0x2a, 0x2a, 0x14, 0x00, 0x00, 0x00, /* 'w' */
0x00, 0x00, 0x00, 0x00, 0x22, 0x14, 0x08, 0x14, 0x22, 0x00, 0x00, 0x00, /* 'x' */
0x00, 0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x22, 0x3c, 0x20, 0x20, 0x1c, /* 'y' */
0x00, 0x00, 0x00, 0x00, 0x3e, 0x10, 0x08, 0x04, 0x3e, 0x00, 0x00, 0x00, /* 'z' */
0x20, 0x10, 0x10, 0x10, 0x10, 0x08, 0x10, 0x10, 0x10, 0x10, 0x20, 0x00, /* '{' */
0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, /* '|' */
0x02, 0x04, 0x04, 0x04, 0x04, 0x08, 0x04, 0x04, 0x04, 0x04, 0x02, 0x00, /* '}' */
0x00, 0x04, 0x2a, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* '~' */
0x00, 0x00, 0x00, 0x08, 0x08, 0x14, 0x14, 0x22, 0x3e, 0x00, 0x00, 0x00, /* '' */
};


#define MAX_ARGS 8

#define FMASK 0x0f
#define BMASK 0x70

typedef enum {
	CONSOLE_STATE_NORMAL = 0,
	CONSOLE_STATE_GOT_ESCAPE,
	CONSOLE_STATE_SEEN_BRACKET,
	CONSOLE_STATE_NEW_ARG,
	CONSOLE_STATE_PARSING_ARG,
} console_state;

struct console_info {
	void*	frame_buffer;
	int32	width;
	int32	height;
	int32	depth;
	int32	bytes_per_pixel;
	int32	bytes_per_row;

	int32	columns;
	int32	rows;
	int32	cursor_x;
	int32	cursor_y;

	uint8	attr;
	bool	bright_attr;
	bool	reverse_attr;
	int32	in_command_rows;
	bool	paging;
	bool	ignore_output;

	// state machine
	console_state state;
	int32	arg_count;
	int32	args[MAX_ARGS];
};

// Palette is (white and black are exchanged):
//	0 - white, 1 - blue, 2 - green, 3 - cyan, 4 - red, 5 - magenta, 6 - yellow,
//  7 - black
//  8-15 - same but bright (we're ignoring those)

static GFX::Color sColors[8] = {
		{ 0xff, 0xff, 0xff, 0x00 }, // white
		{ 0x33, 0x66, 0x98,	0x00 }, // blue
		{ 0x4e, 0x9a, 0x00,	0x00 }, // green
		{ 0x06, 0x98, 0x9a,	0x00 }, // cyan
		{ 0xcc, 0x00, 0x00,	0x00 }, // red
		{ 0x73, 0x44, 0x7b,	0x00 }, // magenta
		{ 0xda, 0xa8, 0x00,	0x00 }, // yellow
		{ 0x00, 0x00, 0x00,	0x00 } // black
};


static inline uint8
foreground_color(uint8 attr)
{
	return attr & 0x7;
}


static inline uint8
background_color(uint8 attr)
{
	return (attr >> 4) & 0x7;
}


static inline bool
in_command_invocation()
{
	return true;
}


Console::Console(const GFX::rect& rect)
	:
	fRect(rect),
	fShowing(false)
{
	GFX::rect consoleRect = Rect();
	fBitmap = new Bitmap(consoleRect.w, consoleRect.h, 8);
	fBitmap->SetColors(sColors, 0, 8);
	fDesc = new console_info;
	fDesc->frame_buffer = fBitmap->Pixels();
	fDesc->depth = 8;
	fDesc->width = consoleRect.w;
	fDesc->height = consoleRect.h;
	fDesc->bytes_per_pixel = (fDesc->depth + 7) / 8;
	fDesc->bytes_per_row = fBitmap->Pitch();
	fDesc->columns = fDesc->width / CHAR_WIDTH;
	fDesc->rows = fDesc->height / CHAR_HEIGHT;
	//fDesc->cursor_x = -1;
	//fDesc->cursor_y = -1;
	fDesc->attr = 0x0f;
	fDesc->ignore_output = false;
	fDesc->cursor_x = fDesc->cursor_y = 0;
	fDesc->state = CONSOLE_STATE_NORMAL;
	fDesc->paging = false;
}


Console::~Console()
{
	if (fBitmap != NULL)
		fBitmap->Release();
	delete fDesc;
}


void
Console::_RenderGlyph(int32 x, int32 y, uint8 glyph, uint8 attr)
{
	// we're ASCII only
	if (glyph > 127)
		glyph = 127;

	uint8* base = (uint8*)((uint8*)fDesc->frame_buffer
		+ fDesc->bytes_per_row * y * CHAR_HEIGHT
		+ x * CHAR_WIDTH * fDesc->bytes_per_pixel);
	uint8 color = foreground_color(attr);
	uint8 backgroundColor = background_color(attr);

	for (y = 0; y < CHAR_HEIGHT; y++) {
		uint8 bits = sFonts[CHAR_HEIGHT * glyph + y];
		for (x = 0; x < CHAR_WIDTH; x++) {
			if (bits & 1)
				base[x] = color;
			else {
				base[x] = backgroundColor;
			}
			bits >>= 1;
		}

		base += fDesc->bytes_per_row;
	}
}


void
Console::_DrawCursor(int32 x, int32 y)
{
	if (x < 0 || y < 0)
		return;

	x *= CHAR_WIDTH * fDesc->bytes_per_pixel;
	y *= CHAR_HEIGHT;
	int32 endX = x + CHAR_WIDTH * fDesc->bytes_per_pixel;
	int32 endY = y + CHAR_HEIGHT;
	uint8* base = (uint8*)((uint8*)fDesc->frame_buffer + y * fDesc->bytes_per_row);

	for (; y < endY; y++) {
		for (int32 x2 = x; x2 < endX; x2++)
			base[x2] = ~base[x2];

		base += fDesc->bytes_per_row;
	}
}


void
Console::_MoveCursor(int32 x, int32 y)
{
	_DrawCursor(fDesc->cursor_x, fDesc->cursor_y);
	_DrawCursor(x, y);

	fDesc->cursor_x = x;
	fDesc->cursor_y = y;
}


void
Console::_PutGlyph(int32 x, int32 y, uint8 glyph, uint8 attr)
{
	if (x >= fDesc->columns || y >= fDesc->rows)
		return;

	_RenderGlyph(x, y, glyph, attr);
}


void
Console::_FillGlyph(int32 x, int32 y, int32 width,
		int32 height, uint8 glyph, uint8 attr)
{
	if (x >= fDesc->columns || y >= fDesc->rows)
		return;

	int32 left = x + width;
	if (left > fDesc->columns)
		left = fDesc->columns;

	int32 bottom = y + height;
	if (bottom > fDesc->rows)
		bottom = fDesc->rows;

	for (; y < bottom; y++) {
		for (int32 x2 = x; x2 < left; x2++) {
			_RenderGlyph(x2, y, glyph, attr);
		}
	}
}


void
Console::Clear(uint8 attr)
{
	switch (fDesc->bytes_per_pixel) {
		case 1:
			memset(fDesc->frame_buffer,
				background_color(attr),
				size_t(fDesc->height) * size_t(fDesc->bytes_per_row));
			break;
		default:
		{
			uint8* base = (uint8*)fDesc->frame_buffer;
			uint8 color = background_color(attr);

			for (int32 y = 0; y < fDesc->height; y++) {
				for (int32 x = 0; x < fDesc->width; x++) {
					base[x] = color;
				}
				base += fDesc->bytes_per_row;
			}
			break;
		}
	}

	fDesc->cursor_x = -1;
	fDesc->cursor_y = -1;
}



void
Console::Toggle()
{
	fShowing = !fShowing;
}


bool
Console::IsActive() const
{
	return fShowing;
}


void
Console::Draw()
{
	GraphicsEngine* gfx = GraphicsEngine::Get();
	if (fShowing) {
		GFX::rect consoleRect = Rect();
		fBitmap->SetAlpha(150);
		gfx->BlitToScreen(fBitmap, NULL, &consoleRect);
	}
}


GFX::rect
Console::Rect() const
{
	return fRect;
}


void
Console::HideCursor()
{
	_MoveCursor(-1, -1);
}


/*!	Scroll from the cursor line up to the top of the scroll region up one
	line.
*/
void
Console::ScrollUp()
{
	// move the screen up one
	GFX::rect rect = Rect();
	rect.y -= CHAR_HEIGHT;
	GraphicsEngine::BlitBitmap(fBitmap, NULL, fBitmap, &rect);

	// clear the bottom line
	_FillGlyph(0, 0, fDesc->columns, 1, ' ', fDesc->attr);
}



void
Console::_NextLine()
{
#if 0
	if (fDesc->cursor_y == fDesc->rows - 1)
		ScrollUp();
	else
	 	fDesc->cursor_y++;
#else
	if (in_command_invocation())
		fDesc->in_command_rows++;
	else
		fDesc->in_command_rows = 0;

#endif
	if (fDesc->cursor_y == fDesc->rows - 1) {
		fDesc->cursor_y = 0;
		_FillGlyph(0, 0, fDesc->columns, 2, ' ', fDesc->attr);
	} else
		fDesc->cursor_y++;

	if (fDesc->cursor_y + 2 < fDesc->rows) {
		_FillGlyph(0, (fDesc->cursor_y + 2) % fDesc->rows, fDesc->columns,
			1, ' ', fDesc->attr);
	}

	fDesc->cursor_x = 0;
}


void
Console::EraseLine(erase_line_mode mode)
{
	switch (mode) {
		case LINE_ERASE_WHOLE:
			_FillGlyph(0, fDesc->cursor_y, fDesc->columns, 1, ' ',
				fDesc->attr);
			break;
		case LINE_ERASE_LEFT:
			_FillGlyph(0, fDesc->cursor_y, fDesc->cursor_x + 1, 1, ' ',
				fDesc->attr);
			break;
		case LINE_ERASE_RIGHT:
			_FillGlyph(fDesc->cursor_x, fDesc->cursor_y, fDesc->columns
				- fDesc->cursor_x, 1, ' ', fDesc->attr);
			break;
	}
}


void
Console::_BackSpace()
{
	if (fDesc->cursor_x <= 0)
		return;

	fDesc->cursor_x--;
	_PutGlyph(fDesc->cursor_x, fDesc->cursor_y, ' ', fDesc->attr);
}


void
Console::PutCharacter(char c)
{
	if (++fDesc->cursor_x >= fDesc->columns) {
		_NextLine();
		fDesc->cursor_x++;
	}

	_PutGlyph(fDesc->cursor_x - 1, fDesc->cursor_y, c, fDesc->attr);
}


void
Console::_SetVt100Attributes(int32 *args, int32 argCount)
{
	if (argCount == 0) {
		// that's the default (attributes off)
		argCount++;
		args[0] = 0;
	}

	for (int32 i = 0; i < argCount; i++) {
		switch (args[i]) {
			case 0: // reset
				fDesc->attr = 0x0f;
				fDesc->bright_attr = true;
				fDesc->reverse_attr = false;
				break;
			case 1: // bright
				fDesc->bright_attr = true;
				fDesc->attr |= 0x08; // set the bright bit
				break;
			case 2: // dim
				fDesc->bright_attr = false;
				fDesc->attr &= ~0x08; // unset the bright bit
				break;
			case 4: // underscore we can't do
				break;
			case 5: // blink
				fDesc->attr |= 0x80; // set the blink bit
				break;
			case 7: // reverse
				fDesc->reverse_attr = true;
				fDesc->attr = ((fDesc->attr & BMASK) >> 4)
					| ((fDesc->attr & FMASK) << 4);
				if (fDesc->bright_attr)
					fDesc->attr |= 0x08;
				break;
			case 8: // hidden?
				break;

			/* foreground colors */
			case 30: // black
			case 31: // red
			case 32: // green
			case 33: // yellow
			case 34: // blue
			case 35: // magenta
			case 36: // cyan
			case 37: // white
			{
				const uint8 colors[] = {0, 4, 2, 6, 1, 5, 3, 7};
				fDesc->attr = (fDesc->attr & ~FMASK) | colors[args[i] - 30]
					| (fDesc->bright_attr ? 0x08 : 0);
				break;
			}

			/* background colors */
			case 40: fDesc->attr = (fDesc->attr & ~BMASK) | (0 << 4); break; // black
			case 41: fDesc->attr = (fDesc->attr & ~BMASK) | (4 << 4); break; // red
			case 42: fDesc->attr = (fDesc->attr & ~BMASK) | (2 << 4); break; // green
			case 43: fDesc->attr = (fDesc->attr & ~BMASK) | (6 << 4); break; // yellow
			case 44: fDesc->attr = (fDesc->attr & ~BMASK) | (1 << 4); break; // blue
			case 45: fDesc->attr = (fDesc->attr & ~BMASK) | (5 << 4); break; // magenta
			case 46: fDesc->attr = (fDesc->attr & ~BMASK) | (3 << 4); break; // cyan
			case 47: fDesc->attr = (fDesc->attr & ~BMASK) | (7 << 4); break; // white
		}
	}
}


bool
Console::_ProcessVt100Command(const char c, bool seenBracket, int32 *args,
	int32 argCount)
{
	bool ret = true;

//	kprintf("process_vt100_command: c '%c', argCount %ld, arg[0] %ld, arg[1] %ld, seenBracket %d\n",
//		c, argCount, args[0], args[1], seenBracket);

	if (seenBracket) {
		switch (c) {
			case 'H':	// set cursor position
			case 'f':
			{
				int32 row = argCount > 0 ? args[0] : 1;
				int32 col = argCount > 1 ? args[1] : 1;
				if (row > 0)
					row--;
				if (col > 0)
					col--;
				_MoveCursor(col, row);
				break;
			}
			case 'A':	// move up
			{
				int32 deltaY = argCount > 0 ? -args[0] : -1;
				if (deltaY == 0)
					deltaY = -1;
				_MoveCursor(fDesc->cursor_x, fDesc->cursor_y + deltaY);
				break;
			}
			case 'e':
			case 'B':	// move down
			{
				int32 deltaY = argCount > 0 ? args[0] : 1;
				if (deltaY == 0)
					deltaY = 1;
				_MoveCursor(fDesc->cursor_x, fDesc->cursor_y + deltaY);
				break;
			}
			case 'D':	// move left
			{
				int32 deltaX = argCount > 0 ? -args[0] : -1;
				if (deltaX == 0)
					deltaX = -1;
				_MoveCursor(fDesc->cursor_x + deltaX, fDesc->cursor_y);
				break;
			}
			case 'a':
			case 'C':	// move right
			{
				int32 deltaX = argCount > 0 ? args[0] : 1;
				if (deltaX == 0)
					deltaX = 1;
				_MoveCursor(fDesc->cursor_x + deltaX, fDesc->cursor_y);
				break;
			}
			case '`':
			case 'G':	// set X position
			{
				int32 newX = argCount > 0 ? args[0] : 1;
				if (newX > 0)
					newX--;
				_MoveCursor(newX, fDesc->cursor_y);
				break;
			}
			case 'd':	// set y position
			{
				int32 newY = argCount > 0 ? args[0] : 1;
				if (newY > 0)
					newY--;
				_MoveCursor(fDesc->cursor_x, newY);
				break;
			}
#if 0
			case 's':	// save current cursor
				save_cur(console, false);
				break;
			case 'u':	// restore cursor
				restore_cur(console, false);
				break;
			case 'r':	// set scroll region
			{
				int32 low = argCount > 0 ? args[0] : 1;
				int32 high = argCount > 1 ? args[1] : sScreen.lines;
				if (low <= high)
					set_scroll_region(console, low - 1, high - 1);
				break;
			}
			case 'L':	// scroll virtual down at cursor
			{
				int32 lines = argCount > 0 ? args[0] : 1;
				while (lines > 0) {
					scrdown(console);
					lines--;
				}
				break;
			}
			case 'M':	// scroll virtual up at cursor
			{
				int32 lines = argCount > 0 ? args[0] : 1;
				while (lines > 0) {
					scrup(console);
					lines--;
				}
				break;
			}
#endif
			case 'K':
				if (argCount == 0 || args[0] == 0) {
					// erase to end of line
					EraseLine(LINE_ERASE_RIGHT);
				} else if (argCount > 0) {
					if (args[0] == 1)
						EraseLine(LINE_ERASE_LEFT);
					else if (args[0] == 2)
						EraseLine(LINE_ERASE_WHOLE);
				}
				break;
#if 0
			case 'J':
				if (argCount == 0 || args[0] == 0) {
					// erase to end of screen
					erase_screen(console, SCREEN_ERASE_DOWN);
				} else {
					if (args[0] == 1)
						erase_screen(console, SCREEN_ERASE_UP);
					else if (args[0] == 2)
						erase_screen(console, SCREEN_ERASE_WHOLE);
				}
				break;
#endif
			case 'm':
				if (argCount >= 0)
					_SetVt100Attributes(args, argCount);
				break;
			default:
				ret = false;
				break;
		}
	} else {
		switch (c) {
#if 0
			case 'c':
				reset_console(console);
				break;
			case 'D':
				rlf(console);
				break;
			case 'M':
				lf(console);
				break;
			case '7':
				save_cur(console, true);
				break;
			case '8':
				restore_cur(console, true);
				break;
#endif
			default:
				ret = false;
				break;
		}
	}

	return ret;
}


void
Console::_ParseCharacter(char c)
{
	switch (fDesc->state) {
		case CONSOLE_STATE_NORMAL:
			// just output the stuff
			switch (c) {
				case '\n':
					_NextLine();
					break;
				case 0x8:
					_BackSpace();
					break;
				case '\t':
					// TODO: real tab...
					fDesc->cursor_x = (fDesc->cursor_x + 8) & ~7;
					if (fDesc->cursor_x >= fDesc->columns)
						_NextLine();
					break;

				case '\r':
				case '\0':
				case '\a': // beep
					break;

				case 0x1b:
					// escape character
					fDesc->arg_count = 0;
					fDesc->state = CONSOLE_STATE_GOT_ESCAPE;
					break;
				default:
					PutCharacter(c);
			}
			break;
		case CONSOLE_STATE_GOT_ESCAPE:
			// look for either commands with no argument, or the '[' character
			switch (c) {
				case '[':
					fDesc->state = CONSOLE_STATE_SEEN_BRACKET;
					break;
				default:
					fDesc->args[0] = 0;
					_ProcessVt100Command(c, false, fDesc->args, 0);
					fDesc->state = CONSOLE_STATE_NORMAL;
					break;
			}
			break;
		case CONSOLE_STATE_SEEN_BRACKET:
			switch (c) {
				case '0'...'9':
					fDesc->arg_count = 0;
					fDesc->args[0] = c - '0';
					fDesc->state = CONSOLE_STATE_PARSING_ARG;
					break;
				case '?':
					// private DEC mode parameter follows - we ignore those
					// anyway
					break;
				default:
					_ProcessVt100Command(c, true, fDesc->args, 0);
					fDesc->state = CONSOLE_STATE_NORMAL;
					break;
			}
			break;
		case CONSOLE_STATE_NEW_ARG:
			switch (c) {
				case '0'...'9':
					if (++fDesc->arg_count == MAX_ARGS) {
						fDesc->state = CONSOLE_STATE_NORMAL;
						break;
					}
					fDesc->args[fDesc->arg_count] = c - '0';
					fDesc->state = CONSOLE_STATE_PARSING_ARG;
					break;
				default:
					_ProcessVt100Command(c, true, fDesc->args,
						fDesc->arg_count + 1);
					fDesc->state = CONSOLE_STATE_NORMAL;
					break;
			}
			break;
		case CONSOLE_STATE_PARSING_ARG:
			// parse args
			switch (c) {
				case '0'...'9':
					fDesc->args[fDesc->arg_count] *= 10;
					fDesc->args[fDesc->arg_count] += c - '0';
					break;
				case ';':
					fDesc->state = CONSOLE_STATE_NEW_ARG;
					break;
				default:
					_ProcessVt100Command(c, true, fDesc->args,
						fDesc->arg_count + 1);
					fDesc->state = CONSOLE_STATE_NORMAL;
					break;
			}
	}
}


bool
Console::IsPagingEnabled() const
{
	return fDesc->paging;
}


void
Console::SetPaging(bool enabled)
{
	fDesc->paging = enabled;
}


void
Console::ClearScreen()
{
	Clear(fDesc->attr);
	_MoveCursor(0, 0);
}


void
Console::PutChar(char c)
{
	if (fDesc->ignore_output
		&& in_command_invocation())
		return;

	fDesc->ignore_output = false;
	HideCursor();

	_ParseCharacter(c);

	_MoveCursor(fDesc->cursor_x, fDesc->cursor_y);
}


void
Console::Puts(const char *text)
{
	if (fDesc->ignore_output && in_command_invocation())
		return;

	fDesc->ignore_output = false;
	//HideCursor();

	while (text[0] != '\0') {
		_ParseCharacter(text[0]);
		text++;
	}

	_MoveCursor(fDesc->cursor_x, fDesc->cursor_y);
}
