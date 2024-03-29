#ifndef __TYPES_H
#define __TYPES_H

#include <cstring>
#include <iostream>

#include "GraphicsDefs.h"
#include "SupportDefs.h"

#define TILE_HEIGHT 64
#define TILE_WIDTH 64

#define RES_BIF_FILE_INDEX_MASK 0x00003FFF
#define RES_TILESET_INDEX_MASK 0x000FC000

#define RES_BIF_INDEX(x) ((x) >> 20)
#define RES_BIF_FILE_INDEX(x) ((x) & RES_BIF_FILE_INDEX_MASK)
#define RES_TILESET_INDEX(x) ((((x) & RES_TILESET_INDEX_MASK) >> 14) - 1)

enum resource_type {
	RES_KEY = 0x000,
	RES_BMP = 0x001,
	RES_MVE = 0x002,
	RES_WAV = 0x004,
	RES_WFX = 0x005,
	RES_PLT = 0x006,
	RES_BAM = 0x3e8,
	RES_WED = 0x3e9,
	RES_CHU = 0x3ea,
	RES_TIS = 0x3eb,
	RES_MOS = 0x3ec,
	RES_ITM = 0x3ed,
	RES_SPL = 0x3ee,
	RES_BCS = 0x3ef,
	RES_IDS = 0x3f0,
	RES_CRE = 0x3f1,
	RES_ARA = 0x3f2,
	RES_DLG = 0x3f3,
	RES_2DA = 0x3f4,
	RES_GAM = 0x3f5,
	RES_STO = 0x3f6,
	RES_WMP = 0x3f7,
	RES_EFF = 0x3f8,
	RES_VVC = 0x3fb,
	RES_PRO = 0x3fd
};

const char *strresource(int type);
const char *res_extension(int type);
int res_string_to_type(const char* string);


bool is_tileset(int16 type);

struct res_ref {
	res_ref();
	res_ref(const char *string);
	res_ref(const res_ref &);

	const char* CString() const;

	res_ref& operator=(const res_ref& other);

	char name[8];
} __attribute__((packed));


class Resource;
typedef Resource* (*resource_creation_func)(const res_ref&);
resource_creation_func get_resource_create(int type);


struct ref_type {
	res_ref name;
	uint16 type;
};


// TODO: Include the above stuff into the namespace
namespace IE {

// IE::point
struct point {
	int16 x;
	int16 y;
};

bool operator==(const IE::point&, const IE::point&);
bool operator!=(const IE::point&, const IE::point&);
int point_distance(const IE::point&, const IE::point&);
IE::point operator+(const IE::point&, const IE::point&);
IE::point operator-(const IE::point&, const IE::point&);


// IE::rect
struct rect {
	int16 x_min;
	int16 y_min;
	int16 x_max;
	int16 y_max;

	uint16 Width() const;
	uint16 Height() const;

	point LeftTop() const;
	point RightBottom() const;
};


struct wall_group {
	uint16 polygon_index;
	uint16 polygon_count;
};


enum polygon_flags {
	POLY_SHADE_WALL 		= 1 << 0,
	POLY_HOVERING 			= 1 << 1,
	POLY_COVER_ANIMATIONS 	= 1 << 2,
	POLY_COVER_ANIMATIONS2 	= 1 << 3,
	POLY_DOOR 				= 1 << 7
};


struct polygon {
	int32 vertex_index;
	int32 vertices_count;
	uint8 flags;
	uint8 height;
	int16 left;
	int16 right;
	int16 top;
	int16 bottom;
} __attribute__((packed));


enum animation_flags {
	ANIM_SHOWN 					= 1 << 0,
	ANIM_SHADED 				= 1 << 1,
	ANIM_ALLOW_TINT 			= 1 << 2,
	ANIM_STOP_AT_FRAME 			= 1 << 3,
	ANIM_SYNCHRONIZED 			= 1 << 4,
	ANIM_RANDOM_START_FRAME 	= 1 << 5,
	ANIM_IGNORE_CLIPPING 		= 1 << 6,
	ANIM_DISABLE_ON_SLOW_MACHINES = 1 << 7,
	ANIM_DO_NOT_COVER 			= 1 << 8,
	ANIM_PLAY_ALL_FRAMES 		= 1 << 9,
	ANIM_USE_PALETTE 			= 1 << 10,
	ANIM_MIRRORED 				= 1 << 11,
	ANIM_SHOW_IN_COMBAT 		= 1 << 12
};


struct animation {
	char name[32];
	point center;
	uint32 play_time;
	res_ref bam_name;
	int16 sequence;
	int16 frame;
	uint32 flags;
	int16 height;
	int16 transparency;
	int16 start_frame;
	int8 looping_chance;
	int8 skip_cycles;
	res_ref palette;
	int32 unknown;

	void Print() const;
} __attribute__((packed));


bool is_play_time(uint32 play_time);


enum orientation {
	ORIENTATION_S	= 0,
	ORIENTATION_SW	= 1,
	ORIENTATION_W 	= 2,
	ORIENTATION_NW	= 3,
	ORIENTATION_N	= 4,
	ORIENTATION_NE	= 5,
	ORIENTATION_E	= 6,
	ORIENTATION_SE	= 7
};

bool is_orientation_facing_north(int o);

enum orientation_extended {
	ORIENTATION_EXT_S	= 0,
	ORIENTATION_EXT_SSW	= 1,
	ORIENTATION_EXT_SW 	= 2,
	ORIENTATION_EXT_WSW	= 3,
	ORIENTATION_EXT_W	= 4,
	ORIENTATION_EXT_WNW	= 5,
	ORIENTATION_EXT_NW	= 6,
	ORIENTATION_EXT_NNW	= 7,
	ORIENTATION_EXT_N	= 8,
	ORIENTATION_EXT_NNE = 9,
	ORIENTATION_EXT_NE	= 10,
	ORIENTATION_EXT_ENE = 11,
	ORIENTATION_EXT_E	= 12,
	ORIENTATION_EXT_ESE	= 13,
	ORIENTATION_EXT_SE	= 14,
	ORIENTATION_EXT_SSE = 15
};

bool is_orientation_ext_facing_north(int o);
int orientation_ext_to_base(int o);

struct item {
	res_ref name;
	uint8 expiration_time;
	uint8 expiration_time2;
	uint16 quantity1;
	uint16 quantity2;
	uint16 quantity3;
	uint32 flags;

	void Print() const;
} __attribute__((packed));


enum actor_flags {
	ACTOR_CRE_EXTERNAL 		= 1 << 0,
	ACTOR_OVERRIDE_SCRIPT 	= 1 << 3
};


struct actor {
	actor();

	char name[32];
	point position;
	point destination;
	uint32 flags;
	uint16 spawned;
	uint8 cre_resref_first_letter;
	uint8 unk;
	uint32 animation;
	uint16 orientation;
	uint16 unused;
	uint32 removal_timer;
	uint16 movement_restriction_distance;
	uint16 movement_restriction_distance_object;
	uint32 time_intervals;
	uint32 num_times_talked_to;
	res_ref dialog;
	res_ref script_override;
	res_ref script_general;
	res_ref script_class;
	res_ref script_race;
	res_ref script_default;
	res_ref script_specific;
	res_ref cre;
	uint32 cre_offset;
	uint32 cre_size;
	uint8 bytes[128];

	void Print() const;
} __attribute__((packed));


enum region_type {
	REGION_TYPE_TRIGGER = 0,
	REGION_TYPE_INFO 	= 1,
	REGION_TYPE_TRAVEL	= 2
};


enum region_flags {
	REGION_KEY_REQUIRED 	= 1 << 0,
	REGION_TRAP_RESET 		= 1 << 1,
	REGION_PARTY_REQUIRED 	= 1 << 2,
	REGION_DETECTABLE 		= 1 << 3,
	REGION_NPC_ACTIVATES 	= 1 << 4,
	REGION_TUTORIAL_ONLY 	= 1 << 5,
	REGION_ANYONE_ACTIVATES = 1 << 6,
	REGION_NO_STRING 		= 1 << 7,
	REGION_DEACTIVATED 		= 1 << 8,
	REGION_NPC_CANT_PASS 	= 1 << 9,
	REGION_ALT_POINT		= 1 << 10,
	REGION_DOOR_CLOSED 		= 1 << 11
};


struct region {
	char name[32];
	uint16 type;
	IE::rect rect;
	uint16 vertex_count;
	uint32 vertex_index;
	uint32 trigger_value;
	uint32 cursor_index;
	res_ref destination;
	char entrance_name[32];
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
	point talk_location;
	uint32 speaker_name;
	res_ref dialog_file;
	void Print() const;
} __attribute__((packed));


enum door_flags {
	DOOR_OPEN = 1 << 0,
	DOOR_LOCKED	= 1 << 1,
	DOOR_TRAP_RESET = 1 << 2,
	DOOR_TRAP_DETECTABLE = 1 << 3,
	DOOR_BROKEN = 1 << 4,
	DOOR_CANTCLOSE = 1 << 5,
	DOOR_DETECTED = 1 << 6,
	DOOR_SECRET = 1 << 7
};


struct door {
	char name[32];
	res_ref short_name;
	uint32 flags;
	uint32 open_vertex_index;
	uint16 open_vertices_count;
	uint16 closed_vertices_count;
	uint32 closed_vertex_index;
	rect open_box;
	rect closed_box;
	uint32 open_cell_index;
	uint16 open_cell_count;
	uint16 closed_cell_count;
	uint32 closed_cell_index;
	uint32 unused;
	res_ref open_sound;
	res_ref closed_sound;
	uint32 cursor_index;
	uint16 trap_detection;
	uint16 trap_removal;
	uint16 trapped;
	uint16 trap_detected;
	point trap_point;
	res_ref key_item;
	res_ref script;
	uint32 detection_difficulty;
	uint32 lock_difficulty;
	rect player_box;
	uint32 lockpick_string;
	uint8 travel_trigger_name[24];
	uint32 name_ref;
	res_ref dialog;
	uint8 unk5[8];
} __attribute__((packed));


struct tiled_object {
	res_ref short_name;
	uint16 flags;
	uint16 cell_index;
	uint16 cell_count;
	uint16 open_polygon_count;
	uint16 closed_polygon_count;
	uint32 open_polygons_offset;
	uint32 closed_polygons_offset;

	void Print() const;
} __attribute__((packed));


struct variable {
	char name[32];
	int8 bytes[8];
	uint32 value;
	int8 bytes2[8];

	void Print() const;
} __attribute__((packed));


struct entrance {
	char name[32];
	uint16 x;
	uint16 y;
	uint16 orientation;
	uint8 unknown[66];
} __attribute__((packed));


struct container {
	char name[32];
	uint16 x;
	uint16 y;
	uint16 type;
	uint16 lock_difficulty;
	uint32 flags;
	uint16 trap_detection_difficulty;
	uint16 trap_removal_difficulty;
	uint16 trapped;
	uint16 trap_detected;
	uint16 trap_launch_x;
	uint16 trap_launch_y;
	uint16 x_min;
	uint16 y_min;
	uint16 x_max;
	uint16 y_max;
	uint32 item_first_index;
	uint32 item_count;
	res_ref trap_script;
	uint32 vertex_first_index;
	uint16 vertices_count;
	char unk[34];
	res_ref key_item;
	uint32 unknown;
	uint32 lockpick_string;
	char unk2[56];
} __attribute__((packed));


// GUI
struct window {
	uint16 id;
	uint16 unk;
	uint16 x;
	uint16 y;
	uint16 w;
	uint16 h;
	uint16 background;
	uint16 num_controls;
	res_ref background_mos;
	uint16 control_offset;
	uint16 unk2;

	void Print() const;
} __attribute__((packed));


enum control_type {
	CONTROL_BUTTON = 0,
	CONTROL_SLIDER = 2,
	CONTROL_TEXTEDIT = 3,
	CONTROL_TEXTAREA = 5,
	CONTROL_LABEL = 6,
	CONTROL_SCROLLBAR = 7
};


struct control {
	uint32 id;
	uint16 x;
	uint16 y;
	uint16 w;
	uint16 h;
	uint8 type;
	uint8 unk;
	void Print() const;
} __attribute__((packed));


struct button : public control {
	res_ref image;
	uint8 cycle;
	uint8 just_flags;
	uint8 frame_unpressed;
	uint8 anchor_x1;
	uint8 frame_pressed;
	uint8 anchor_x2;
	uint8 frame_selected;
	uint8 anchor_y1;
	uint8 frame_disabled;
	uint8 anchor_y2;

	void Print() const;
} __attribute__((packed));


struct slider : public control {
	res_ref background;
	res_ref knob;
	int16 cycle;
	uint8 frame_ungrabbed;
	uint8 frame_grabbed;
	uint16 knob_offset_x;
	uint16 knob_offset_y;
	uint16 knob_jump_width;
	uint16 knob_jump_count;
	uint16 unk1;
	uint16 unk2;
	uint16 unk3;
	uint16 unk4;
	void Print() const;
} __attribute__((packed));


enum label_flags {
	LABEL_USE_RGB_COLORS	= 1 << 0,
	LABEL_JUSTIFY_CENTER	= 1 << 2,
	LABEL_JUSTIFY_LEFT		= 1 << 3,
	LABEL_JUSTIFY_RIGHT		= 1 << 4,
	LABEL_JUSTIFY_TOP		= 1 << 5,
	LABEL_JUSTIFY_BOTTOM	= 1 << 7
};


struct text_area : public control {
	res_ref font_bam;
	res_ref font_initials_bam;
	uint8 color1_r;
	uint8 color1_g;
	uint8 color1_b;
	uint8 color1_a;
	uint8 color2_r;
	uint8 color2_g;
	uint8 color2_b;
	uint8 color2_a;
	uint8 color3_r;
	uint8 color3_g;
	uint8 color3_b;
	uint8 color3_a;
	uint32 parent_control_id;
	void Print() const;
} __attribute__((packed));


struct label : public control {
	uint32 text_ref;
	res_ref font_bam;
	uint8 color1_r;
	uint8 color1_g;
	uint8 color1_b;
	uint8 color1_a;
	uint8 color2_r;
	uint8 color2_g;
	uint8 color2_b;
	uint8 color2_a;
	uint16 flags;
	void Print() const;
} __attribute__((packed));


struct scrollbar : public control {
	res_ref bam;
	uint16 cycle;
	uint16 arrow_up_unpressed;
	uint16 arrow_up_pressed;
	uint16 arrow_down_unpressed;
	uint16 arrow_down_pressed;
	uint16 trough;
	uint16 slider;
	uint32 text_area_id;
	void Print() const;
} __attribute__((packed));


struct text_edit : public control {
	res_ref background1;
	res_ref background2;
	res_ref background3;
	res_ref background4;
	res_ref cursor_bam;
	uint16 carot_cycle;
	uint16 carot_frame;
	uint16 text_edit_x;
	uint16 text_edit_y;
	uint32 parent_control_id;
	res_ref font_bam;
	uint16 unk_textedit;
	char initial_text[32];
	uint16 max_length;
	uint32 case_flags;
	void Print() const;
} __attribute__((packed));


#define NUM_CURSORS 128

enum arrow_cursors {
	CURSOR_ARROW_E = 0,
	CURSOR_ARROW_NE = 1,
	CURSOR_ARROW_N = 2,
	CURSOR_ARROW_NW = 3,
	CURSOR_ARROW_W = 4,
	CURSOR_ARROW_SW = 5,
	CURSOR_ARROW_S = 6,
	CURSOR_ARROW_SE = 7,

	CURSOR_HAND = 8
};

enum cursors {
	CURSOR_HAND2 = 1,
	CURSOR_PICKUP = 2,
	CURSOR_WALKTO = 4,
	CURSOR_WALKTO2 = 5,
	CURSOR_NOWAY = 6,
	CURSOR_NOWAY2 = 7,
	CURSOR_CIRCLE_UNK = 8,
	CURSOR_CIRCLE_UNK2 = 9,
	CURSOR_CLOCK = 10,
	CURSOR_CLOCK2 = 11,
	CURSOR_ATTACK = 12,
	CURSOR_ATTACK2 = 13,
	CURSOR_SCROLLUNK = 14,
	CURSOR_SCROLLUNK2 = 15,
	CURSOR_DEFEND = 16,
	CURSOR_DEFEND2 = 17,
	CURSOR_TALK = 18,
	CURSOR_TALK2 = 19,
};

void check_objects_size();


}

// point
static inline IE::point
offset_point(const IE::point &point, sint16 x, sint16 y)
{
	IE::point newPoint = point;
	newPoint.x += x;
	newPoint.y += y;
	return newPoint;
}


static inline IE::rect
offset_rect(const IE::rect &rect, sint16 x, sint16 y)
{
	IE::rect newRect = rect;
	newRect.x_min += x;
	newRect.y_min += y;
	newRect.x_max += x;
	newRect.y_max += y;
	return newRect;
}


static inline bool
rect_contains(const IE::rect& rect, const IE::point& point)
{
	if (point.x >= rect.x_min && point.x <= rect.x_max
		&& point.y >= rect.y_min && point.y <= rect.y_max)
		return true;
	return false;
}


// conversion
static inline GFX::rect
rect_to_gfx_rect(const IE::rect& rect)
{
	GFX::rect gfxRect(
			rect.x_min,
			rect.y_min,
			rect.Width(),
			rect.Height()
	);
	return gfxRect;
}


static inline IE::rect
gfx_rect_to_rect(const GFX::rect& rect)
{
	IE::rect IERect = {
			rect.x,
			rect.y,
			(int16)(rect.x + rect.w),
			(int16)(rect.y + rect.h)
	};
	return IERect;
}


bool operator<(const res_ref&, const res_ref&);
bool operator==(const res_ref&, const res_ref&);
bool operator!=(const res_ref&, const res_ref&);
bool operator<(const ref_type&, const ref_type&);
std::ostream &operator<<(std::ostream &os, res_ref ref);


#endif
