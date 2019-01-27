#include "Resource.h"
#include "IDSResource.h"
#include "IETypes.h"
#include "ResManager.h"
#include "Utils.h"

#define __STDC_FORMAT_MACROS

#include <assert.h>
#include <cmath>
#include <cstdio>
#include <inttypes.h>



using namespace IE;

const static int kMaxBuffers = 10;

static char*
get_buffer()
{
	static char sBuffers[kMaxBuffers][10];
	static int sBufferIndex = 0;

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
		case RES_2DA:
			return ".2DA";
		case RES_ARA:
			return ".ARE";
		case RES_BAM:
			return ".BAM";
		case RES_BCS:
			return ".BCS";
		case RES_BMP:
			return ".BMP";
		case RES_CHU:
			return ".CHU";
		case RES_CRE:
			return ".CRE";
		case RES_DLG:
			return ".DLG";
		case RES_IDS:
			return ".IDS";
		case RES_ITM:
			return ".ITM";
		case RES_MOS:
			return ".MOS";
		case RES_MVE:
			return ".MVE";
		case RES_TIS:
			return ".TIS";
		case RES_WED:
			return ".WED";
		case RES_WMP:
			return ".WMP";

		default:
			throw "Unknown resource!";
			break;
	}

	return NULL;
}


int
res_string_to_type(const char* string)
{
	const char* ext = extension(string);
	if (ext == NULL)
		return -1;
	if (!strcasecmp(ext, ".2DA"))
		return RES_2DA;
	else if (!strcasecmp(ext, ".DLG"))
		return RES_DLG;
	else if (!strcasecmp(ext, ".WED"))
		return RES_WED;
	else if (!strcasecmp(ext, ".WMP"))
		return RES_WMP;
	else if (!strcasecmp(ext, ".TIS"))
		return RES_TIS;
	else if (!strcasecmp(ext, ".BAM"))
		return RES_BAM;
	else if (!strcasecmp(ext, ".MOS"))
		return RES_MOS;
	else if (!strcasecmp(ext, ".ARE"))
		return RES_ARA;
	else if (!strcasecmp(ext, ".CRE"))
		return RES_CRE;
	else if (!strcasecmp(ext, ".BCS"))
		return RES_BCS;
	else if (!strcasecmp(ext, ".BMP"))
		return RES_BMP;
	else if (!strcasecmp(ext, ".IDS"))
		return RES_IDS;
	else if (!strcasecmp(ext, ".ITM"))
		return RES_ITM;
	return -1;
}


bool
is_tileset(int16 type)
{
	return type == RES_TIS;
}


void
IE::check_objects_size()
{
	assert_size(sizeof(polygon), 18);
	assert_size(sizeof(animation), 76);
	assert_size(sizeof(actor), 0x110);
	assert_size(sizeof(control), 14);
	assert_size(sizeof(button), 32);
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
		os << ref.name[i];
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


int
operator-(const IE::point& ptA, const IE::point& ptB)
{
	return std::abs(ptA.x - ptB.x) + std::abs(ptA.y - ptB.y);
}


// orientation
bool
IE::is_orientation_facing_north(int o)
{
	return o >= 3 && o <= 5;
}


bool
IE::is_orientation_ext_facing_north(int o)
{
	return o >= 5 && o <= 11;
}


int
IE::orientation_ext_to_base(int o)
{
	return o / 2;
}


// Animation
void
animation::Print() const
{
	printf("name: %s\n", name);
	printf("center: %d %d\n", center.x, center.y);
	printf("play_time: %d\n", play_time);
	printf("bam_name: %s\n", bam_name.CString());
	printf("sequence: %d\n", sequence);
	printf("frame: %d\n", frame);
	printf("flags: (%d)\n", flags);
	printf("\t%s %s %s %s %s %s %s %s %s\n",
			flags & ANIM_SHOWN ? "SHOWN" : "",
			flags & ANIM_SHADED ? "SHADED" : "",
			flags & ANIM_ALLOW_TINT ? "SHADED_NIGHT" : "",
			flags & ANIM_HOLD ? "HOLD" : "",
			flags & ANIM_START_ON_FIRST_FRAME ? "SYNCHRONIZED" : "",
			flags & ANIM_DISABLE_ON_SLOW_MACHINES ? "ANIM_DISABLE_ON_SLOW_MACHINES" : "",
			flags & ANIM_PLAY_ALL_FRAMES ? "PLAY_ALL_FRAMES" : "",
			flags & ANIM_USE_PALETTE ? "USE_PALETTE" : "",
			flags & ANIM_MIRRORED ? "MIRRORED" : "");
	printf("height: %d\n", height);
	printf("transparency: %d\n", transparency);
	printf("start_frame: %d\n", start_frame);
	printf("looping_chance: %d\n", looping_chance);
	printf("skip_cycles: %d\n", skip_cycles);
	printf("palette: %s\n", palette.CString());
	printf("unknown: %d\n", unknown);
}


// item
void
item::Print() const
{
	std::cout << "item " << name << std::endl;
	std::cout << "expiration: " << std::dec << (int)expiration_time;
	std::cout << " " << (int)expiration_time2 << std::endl;
	std::cout << "quantity: " << quantity1 << " " << quantity2;
	std::cout << " " << quantity3 << std::endl;
	std::cout << "flags: " << std::hex << flags << std::endl;
}


void
actor::Print() const
{
	std::cout << "Actor " << name << ": " << std::endl;
	std::cout << "\tCRE: " << cre << std::endl;
	std::cout << "\tposition: (" << position.x;
	std::cout << ", " << position.y << ")" << std::endl;
	std::cout << "\tdestination: (" << destination.x;
	std::cout << ", " << destination.y << ")" << std::endl;
	std::cout << "\tflags: " << std::hex << flags << std::endl;
	std::cout << "\tcre_resref_first_letter: " << (int)cre_resref_first_letter << std::endl;
	std::cout << "\tanimation: " << IDTable::AnimationAt(animation);
	std::cout << "(" << animation << ")" << std::endl;
	std::cout << "\tdialog: " << dialog << std::endl;
	std::cout << "\tcre attached: " << ((flags & IE::ACTOR_CRE_EXTERNAL) ? "NO" : "YES") << std::endl;
	/*printf("\tscript_override: %s\n", (const char*)script_override);
	printf("\tscript_general: %s\n", (const char*)script_general);
	printf("\tscript_class: %s\n", (const char*)script_class);
	printf("\tscript_race: %s\n", (const char*)script_race);
	printf("\tscript_default: %s\n", (const char*)script_default);
	printf("\tscript_specific: %s\n", (const char*)script_specific);
	printf("\tcre_offset: %u\n", cre_offset);
	printf("\tcre_size: %u\n", cre_size);*/
}


void
region::Print() const
{
	std::cout << "name: " << name << std::endl;
	std::cout << "type: " << type << std::endl;
	/*IE::rect rect;
	uint16 vertex_count;
	uint32 vertex_index;
	uint32 unk;
	uint32 cursor_index;
	res_ref destination; */
	std::cout << entrance_name << std::endl;
	/*
	uint32 flags;
	uint32 info_text;
	uint16 trap_detection_difficulty;
	uint16 trap_removal_difficulty;
	uint16 trapped;
	uint16 trap_detected;
	IE::point trap_launch_location;
	res_ref key_item;
	res_ref script;
	uint16 use_point_x;
	uint16 use_point_y;
	uint32 unk2;
	char unk3[32];
	res_ref sound;
	uint32 talk_location;
	uint32 str_name;
	res_ref dialog_file;*/
}


void
tiled_object::Print() const
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


void
window::Print() const
{
	std::cout << std::dec;
	std::cout << "WINDOW id: " << id << std::endl;
	std::cout << "position: " << x << ", " << y << std::endl;
	std::cout << "size: " << w << ", " << h << std::endl;
	std::cout << "has background ? " << (background ? "YES" : "NO") << std::endl;
	std::cout << "control offset: " << control_offset << std::endl;
	if (background)
		std::cout << "background: " << background_mos << std::endl;

	std::cout << "unk: " << unk << std::endl;
	std::cout << "unk2:" << unk2 << std::endl;
}


void
control::Print() const
{
	std::cout << std::dec;
	std::cout << "\tid: " << id << std::endl;
	std::cout << "\tposition: " << x << ", " << y << std::endl;
	std::cout << "\tsize: " << w << ", " << h << std::endl;
	std::cout << "\ttype: " << std::dec << (int)type << std::endl;
	std::cout << "\tunk: " << (int)unk << std::endl;
}


void
button::Print() const
{
	std::cout << "button:" << std::endl;
	control::Print();
	std::cout << "\timage: " << image << std::endl;
	std::cout << "\tcycle: " << (int)cycle << std::endl;
	std::cout << "\tflags: 0x" << std::hex << (int)just_flags << std::endl;
	std::cout << std::dec;
	std::cout << "\tunpressed: " << (int)frame_unpressed << std::endl;
	std::cout << "\tpressed: " << (int)frame_pressed << std::endl;
	std::cout << "\tselected: " << (int)frame_selected << std::endl;
	std::cout << "\tdisabled: " << (int)frame_disabled << std::endl;
	std::cout << "\tanchors: " << (int)anchor_x1 << ", " << (int)anchor_x2;
	std::cout << ", " << (int)anchor_y1 << ", " << (int)anchor_y2 << std::endl;
}


void
label::Print() const
{
	std::cout << "label:" << std::endl;
	control::Print();
	std::cout << "\tflags: " << std::endl;
	std::cout << "\tcolor 1: " << (int)color1_r << ", " << (int)color1_g;
	std::cout << ", " << (int)color1_b << ", " << (int)color1_a << std::endl;
	std::cout << "\tcolor 2: " << (int)color2_r << ", " << (int)color2_g;
	std::cout << ", " << (int)color2_b << ", " << (int)color2_a << std::endl;
	if (flags & LABEL_USE_RGB_COLORS)
		std::cout << "\tuse_rgb_colos" << std::endl;
	if (flags & LABEL_JUSTIFY_CENTER)
		std::cout << "\tcenter" << std::endl;
	if (flags & LABEL_JUSTIFY_RIGHT)
		std::cout << "\tright" << std::endl;

}
