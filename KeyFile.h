#ifndef __KEYFILE_H
#define __KEYFILE_H

#include <string>
#include "Types.h"

#define KEY_SIGNATURE "KEY "
#define KEY_VERSION "V1  "

#define RES_BIF_FILE_INDEX_MASK 0x00003FFF
#define RES_TILESET_INDEX_MASK 0x000FC000

#define RES_BIF_INDEX(x) ((x) >> 20)
#define RES_BIF_FILE_INDEX(x) ((x) & RES_BIF_FILE_INDEX_MASK)
#define RES_TILESET_INDEX(x) ((((x) & RES_TILESET_INDEX_MASK) >> 14) - 1)

struct KeyFileEntry {
	uint32 length;
	uint16 location;
	std::string name;
};


struct KeyResEntry {
	res_ref name;
	uint16 type;
	uint32 key;
};

#endif
