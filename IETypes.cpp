#include "IETypes.h"

#include "2DAResource.h"
#include "AreaResource.h"
#include "BCSResource.h"
#include "BamResource.h"
#include "BmpResource.h"
#include "CHUIResource.h"
#include "CreResource.h"
#include "DLGResource.h"
#include "IDSResource.h"
#include "ITMResource.h"
#include "KEYResource.h"
#include "MOSResource.h"
#include "MveResource.h"
#include "MemoryStream.h"
#include "ResManager.h"
#include "SPLResource.h"
#include "TisResource.h"
#include "TLKResource.h"
#include "Utils.h"
#include "VVCResource.h"
#include "WAVResource.h"
#include "WedResource.h"
#include "WMAPResource.h"

#define __STDC_FORMAT_MACROS

#include <assert.h>
#include <cmath>
#include <cstdio>
#include <inttypes.h>
#include <string>


using namespace IE;

const static int kMaxBuffers = 10;

struct resource_types {
	int type;
	std::string extension;
	std::string description;
	Resource* (*create_function)(const res_ref&);
};

const static resource_types kResourceTypes[] = {
	{ RES_2DA, "2DA", "2DA format" , &TWODAResource::Create },
	{ RES_ARA, "ARE", "AREA format", &ARAResource::Create },
	{ RES_BAM, "BAM", "BAM format", &BAMResource::Create },
	{ RES_BCS, "BCS", "Compiled script (BCS format)", &BCSResource::Create },
	{ RES_BMP, "BMP", "Bitmap (BMP) format", &BMPResource::Create },
	{ RES_CHU, "CHU", "CHU format", &CHUIResource::Create  },
	{ RES_CRE, "CRE", "Creature", &CREResource::Create },
	{ RES_DLG, "DLG", "Dialog", &DLGResource::Create  },
	{ RES_EFF, "EFF", "EFF Effect", NULL  },
	{ RES_GAM, "GAM", "GAM format", NULL  },
	{ RES_IDS, "IDS", "IDS format", &IDSResource::Create },
	{ RES_ITM, "ITM", "Item", NULL },
	{ RES_MOS, "MOS", "MOS format", &MOSResource::Create },
	{ RES_MVE, "MVE", "Movie (MVE) format", &MVEResource::Create },
	{ RES_PLT, "PLT", "Paper Dolls (PLT) format", NULL },
	{ RES_PRO, "PRO", "PRO format (projectile)", NULL },
	{ RES_SPL, "SPL", "SPL format", &SPLResource::Create },
	{ RES_STO, "STO", "STORE format", NULL },
	{ RES_TIS, "TIS", "TIS format", &TISResource::Create },
	{ RES_VVC, "VVC", "VVC Effect", &VVCResource::Create },
	{ RES_WAV, "WAV", "WAV format", &WAVResource::Create },
	{ RES_WED, "WED", "WED format", &WEDResource::Create },
	{ RES_WFX, "WFX", "WFX format", NULL },
	{ RES_WMP, "WMP", "World map format", &WMAPResource::Create },
};


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
	for (size_t i = 0; i < sizeof(kResourceTypes) / sizeof(kResourceTypes[0]); i++) {
		if (kResourceTypes[i].type == type)
			return kResourceTypes[i].extension.c_str();
	}

	throw std::runtime_error("Unknown resource!");
	return NULL;
}


int
res_string_to_type(const char* string)
{
	const char* ext = extension(string);
	if (ext == NULL)
		return -1;
	for (size_t i = 0; i < sizeof(kResourceTypes) / sizeof(kResourceTypes[0]); i++) {
		if (strcasecmp(kResourceTypes[i].extension.c_str(), ext) == 0) {
			return kResourceTypes[i].type;
		}
	}
	throw std::runtime_error("Unknown Extension");
	return -1;	
}


const char*
strresource(int type)
{
	for (size_t i = 0; i < sizeof(kResourceTypes) / sizeof(kResourceTypes[0]); i++) {
		if (kResourceTypes[i].type == type)
			return kResourceTypes[i].description.c_str();
	}
	return NULL;
}


resource_creation_func
get_resource_create(int type)
{
	for (size_t i = 0; i < sizeof(kResourceTypes) / sizeof(kResourceTypes[0]); i++) {
		if (kResourceTypes[i].type == type)
			return kResourceTypes[i].create_function;
	}
	return NULL;
}


// ******
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
	printf("flags: (0x%x)\n", flags);
	printf("\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n",
			(flags & ANIM_SHOWN) ? "SHOWN" : "",
			(flags & ANIM_SHADED) ? "SHADED" : "",
			(flags & ANIM_ALLOW_TINT) ? "SHADED_NIGHT" : "",
			(flags & ANIM_STOP_AT_FRAME) ? "STOP_AT_FRAME" : "",
			(flags & ANIM_SYNCHRONIZED) ? "ANIM_SYNCHRONIZED" : "",
			(flags & ANIM_RANDOM_START_FRAME) ? "RANDOM_START_FRAME" : "",
			(flags & ANIM_IGNORE_CLIPPING) ? "ANIM_IGNORE_CLIPPING" : "",
			(flags & ANIM_DISABLE_ON_SLOW_MACHINES) ? "ANIM_DISABLE_ON_SLOW_MACHINES" : "",
			(flags & ANIM_DO_NOT_COVER) ? "ANIM_DO_NOT_COVER" : "",
			(flags & ANIM_PLAY_ALL_FRAMES) ? "PLAY_ALL_FRAMES" : "",
			(flags & ANIM_USE_PALETTE) ? "USE_PALETTE" : "",
			(flags & ANIM_MIRRORED) ? "MIRRORED" : "",
			(flags & ANIM_SHOW_IN_COMBAT) ? "ANIM_SHOW_IN_COMBAT" : "");
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
	std::cout << std::dec;
	std::cout << "name: " << name << std::endl;
	std::cout << "type: " << type << std::endl;
	std::cout << "trigger_value: " << trigger_value << std::endl;
	//uint32 cursor_index;
	//res_ref destination;
	std::cout << "entrance name: " << entrance_name << std::endl;

	std::cout << "flags:" << std::hex << "0x" << flags << std::endl;
	std::cout << std::dec;
	if (flags & REGION_KEY_REQUIRED)
		std::cout << "\t" << "key required" << std::endl;
	if (flags & REGION_TRAP_RESET)
		std::cout << "\t" << "trap reset" << std::endl;
	if (flags & REGION_PARTY_REQUIRED)
		std::cout << "\t" << "party required" << std::endl;
	if (flags & REGION_DETECTABLE)
		std::cout << "\t" << "detectable" << std::endl;
	if (flags & REGION_NPC_ACTIVATES)
		std::cout << "\t" << "NPC activates" << std::endl;
	if (flags & REGION_TUTORIAL_ONLY)
		std::cout << "\t" << "tutorial only" << std::endl;
	if (flags & REGION_ANYONE_ACTIVATES)
		std::cout << "\t" << "anyone activates" << std::endl;
	if (flags & REGION_NO_STRING)
		std::cout << "\t" << "no string" << std::endl;
	if (flags & REGION_DEACTIVATED)
		std::cout << "\t" << "deactivated" << std::endl;
	if (flags & REGION_NPC_CANT_PASS)
		std::cout << "\t" << "NPC can't pass" << std::endl;
	if (flags & REGION_ALT_POINT)
		std::cout << "\t" << "alternative point" << std::endl;
	if (flags & REGION_DOOR_CLOSED)
		std::cout << "\t" << "door closed" << std::endl;		
	
	std::cout << "info_text: " << IDTable::GetDialog(info_text) << std::endl;
	//uint16 trap_detection_difficulty;
	//uint16 trap_removal_difficulty;
	std::cout << "trapped: " << (trapped ? "YES" : "NO") << std::endl;
	
	//uint16 trap_detected;
	std::cout << "trap_launch_location:" << trap_launch_location.x << ", " << trap_launch_location.y << std::endl;
	std::cout << "key_item: " << key_item << std::endl;
	std::cout << "script: "	<< script.CString() << std::endl;
	//uint16 use_point_x;
	//uint16 use_point_y;
	std::cout << "unk2: " << unk2 << std::endl;
	std::cout << "unk3: " << unk3 << std::endl;
	//std::cout << "sound: " << sound << std::endl;
	//std::cout << "talk_location: " << talk_location << std::endl;
	//std::cout << "str_name:" << str_name << std::endl;
	//std::cout << "dialog_file: " << dialog_file.CString() << std::endl;
}


void
tiled_object::Print() const
{
	printf("short_name: %s\n", short_name.CString());
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
	std::cout << "num_controls: " << num_controls << std::endl;
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
slider::Print() const
{
	std::cout << "slider" << std::endl;
	control::Print();
}


void
text_area::Print() const
{
	std::cout << "text_area" << std::endl;
	control::Print();
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


void
scrollbar::Print() const
{
	std::cout << "scrollbar" << std::endl;
	control::Print();
}


void
text_edit::Print() const
{
	std::cout << "text_edit" << std::endl;
	control::Print();
}
