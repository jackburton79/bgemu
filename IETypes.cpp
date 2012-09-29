#include "Resource.h"
#include "IDSResource.h"
#include "IETypes.h"
#include "ResManager.h"

#include <assert.h>
#include <cstdio>

using namespace IE;

const static int kMaxBuffers = 10;
static char sBuffers[kMaxBuffers][16];
static int sBufferIndex = 0;

static char*
get_buffer()
{
	// TODO: Not thread safe
	char* buffer = sBuffers[sBufferIndex];

	if (++sBufferIndex >= kMaxBuffers)
		sBufferIndex = 0;

	return buffer;
}


const char*
res_extension(int type)
{
	switch (type) {
		case RES_ARA:
			return ".ARE";
		case RES_BAM:
			return ".BAM";
		case RES_MOS:
			return ".MOS";
		case RES_BCS:
			return ".BCS";
		case RES_CRE:
			return ".CRE";
		case RES_BMP:
			return ".BMP";
		case RES_IDS:
			return ".IDS";
		case RES_TIS:
			return ".TIS";
		case RES_WED:
			return ".WED";
		case RES_MVE:
			return ".MVE";
		case RES_WMP:
			return ".WMP";
		default:
			throw "Unknown resource!";
			break;
	}

	return NULL;
}

void
IE::check_objects_size()
{
	assert(sizeof(polygon) == 18);
	assert(sizeof(animation) == 76);
	assert(sizeof(actor) == 0x110);
}


res_ref::res_ref()
{
	name[0] = '\0';
}


res_ref::res_ref(const char *string)
{
	int32 len = string ? strlen(string) : 0;
	len = std::min(len, 8);
	strncpy(name, string, len);
	if (len < 8)
		name[len] = '\0';
}


res_ref::res_ref(const res_ref &ref)
{
	memcpy(name, ref.name, 8);
}


const char*
res_ref::CString() const
{
	char* str = get_buffer();
	strncpy(str, name, 8);
	str[8] = '\0';
	return (const char *)str;
}


res_ref&
res_ref::operator=(const res_ref& other)
{
	memcpy(name, other.name, 8);
	return *this;
}


/*
char&
res_ref::operator [](int index)
{
	return name[index];
};
*/

bool
operator<(const res_ref &ref1, const res_ref &ref2)
{
	return strncasecmp(ref1.name, ref2.name, 8) < 0;
}


bool
operator==(const res_ref &ref1, const res_ref &ref2)
{
	return strncasecmp(ref1.name, ref2.name, 8) == 0;
}


bool
operator!=(const res_ref &ref1, const res_ref &ref2)
{
	return strncasecmp(ref1.name, ref2.name, 8) != 0;
}


bool
operator<(const ref_type &ref1, const ref_type &ref2)
{
	if (ref1.type != ref2.type)
		return ref1.type < ref2.type;

	return ref1.name < ref2.name;
}


std::ostream &
operator<<(std::ostream &os, res_ref ref)
{
	for (int32 i = 0; i < 8; i++) {
		if (ref.name[i] == '\0')
			break;
		std::cout << ref.name[i];
	}
	return os;
};


bool
operator==(const IE::point& ptA, const IE::point& ptB)
{
	return ptA.x == ptB.x && ptA.y == ptB.y;
}


bool
operator!=(const IE::point& ptA, const IE::point& ptB)
{
	return ptA.x != ptB.x || ptA.y != ptB.y;
}


void
actor::Print() const
{
	printf("Actor %s:\n", name);
	/*printf("\tCRE: %s\n", (const char *)cre);
	printf("\tposition: (%d, %d)\n", position.x, position.y);
	printf("\tdestination: (%d, %d)\n", destination.x, destination.y);
	printf("\tflags: 0x%x\n", flags);
	printf("\tanimation: %s (0x%x)\n", AnimateIDS()->ValueFor(animation), animation);
	printf("\tdialog: %s\n", (const char*)dialog);
	printf("\tscript_override: %s\n", (const char*)script_override);
	printf("\tscript_general: %s\n", (const char*)script_general);
	printf("\tscript_class: %s\n", (const char*)script_class);
	printf("\tscript_race: %s\n", (const char*)script_race);
	printf("\tscript_default: %s\n", (const char*)script_default);
	printf("\tscript_specific: %s\n", (const char*)script_specific);
	printf("\tcre_offset: %u\n", cre_offset);
	printf("\tcre_size: %u\n", cre_size);*/
}


void
door_wed::Print() const
{
	printf("name: %s\n", name);
	printf("flags: 0x%x\n", flags);
	printf("cell_index: %d\n", cell_index);
	printf("cell_count: %d\n", cell_count);
}


void
variable::Print() const
{
	printf("name: %s, value: %d\n", name, value);
}
