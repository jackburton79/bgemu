#ifndef __TYPES_H
#define __TYPES_H

#include <cstring>
#include <iostream>

#include "SupportDefs.h"

#define TILE_HEIGHT 64
#define TILE_WIDTH 64

enum resource_type {
	RES_BMP = 0x001,
	RES_MVE = 0x002,
	RES_WAV = 0x004,
	RES_WFX = 0x005,
	RES_PLT = 0x006,
	RES_BAM = 0x3e8,
	RES_WED = 0x3e9,
	RES_TIS = 0x3eb,
	RES_MOS = 0x3ec,
	RES_ITM = 0x3ed,
	RES_SPL = 0x3ee,
	RES_COMPILED_SCRIPT = 0x3ef,
	RES_CRE = 0x3f1,
	RES_AREA = 0x3f2,
	RES_DLG = 0x3f3,
	RES_EFF = 0x3f8,
	RES_VVC = 0x3fb
};

const char *strresource(int type);

struct res_ref {
	res_ref();
	res_ref(const char *string);
	res_ref(const res_ref &);
	operator const char*() const;
	char name[8];
} __attribute__((packed));


struct ref_type {
	res_ref name;
	uint16 type;
};


struct point {
	int16 x;
	int16 y;
};


struct polygon {
	int32 vertex_index;
	int32 vertices_count;
	uint8 is_hole;
	uint8 unk;
	int16 x_min;
	int16 x_max;
	int16 y_min;
	int16 y_max;
} __attribute__((packed));


enum animation_flags {
	ANIM_SHOWN = 1 << 0,
	ANIM_SHADED = 1 << 1,
	ANIM_SHADED_NIGHT = 1 << 2,
	ANIM_HOLD = 1 << 3,
};


struct animation {
	char name[32];
	point center;
	uint32 play_time;
	res_ref bam_name;
	int16 sequence;
	int16 frame;
	int32 flags;
	int32 unk4;
	int8 bytes[16];
} __attribute__((packed));

bool operator<(const res_ref &, const res_ref &);
bool operator==(const res_ref &, const res_ref &);
bool operator<(const ref_type &, const ref_type &);

std::ostream &operator<<(std::ostream &os, res_ref ref);

#endif
