/*
 * CharGenData.cpp - see CharGenData.h
 */

#include "CharGenData.h"

#include <strings.h>

// Generated from GemRB's unhardcoded BG1 tables (races.2da, classes.2da, aligns.2da, pictures.2da).

static const CharGenRace kRaces[] = {
	{ "HUMAN", 7193, 9550, 1096 },
	{ "ELF", 7194, 9552, 1097 },
	{ "HALF_ELF", 7197, 9555, 1098 },
	{ "GNOME", 7196, 9553, 1099 },
	{ "HALFLING", 7195, 9554, 1101 },
	{ "DWARF", 7182, 9551, 1100 },
};

// weapprof.2da of GemRB's unhardcoded BG1 tables.
static const CharGenProficiency kProficiencies[] = {
	{ "LARGE_SWORD", 8668, 9589 },
	{ "SMALL_SWORD", 8732, 9590 },
	{ "BOW", 8733, 9591 },
	{ "SPEAR", 8734, 9592 },
	{ "BLUNT", 9400, 9593 },
	{ "SPIKED", 9401, 9594 },
	{ "AXE", 9402, 9595 },
	{ "MISSILE", 9403, 9596 },
};

// skills.2da of GemRB's unhardcoded BG1 tables (the CRE bytes: IESDP's cre_v1).
static const CharGenSkill kSkills[] = {
	{ "PICK_POCKETS", 9463, 9597, 0x6a },
	{ "OPEN_LOCKS", 9460, 9598, 0x67 },
	{ "FIND_TRAPS", 9462, 9599, 0x69 },
	{ "STEALTH", 9461, 9600, 0x68 },
};

// haterace.2da of GemRB's unhardcoded BG1 tables.
static const CharGenHatedRace kHatedRaces[] = {
	{ "CRAWLER", 15940, 104, 15988 },
	{ "ETTERCAP", 15939, 107, 15990 },
	{ "GHOUL", 15946, 108, 15991 },
	{ "GIBBERLING", 15931, 109, 15994 },
	{ "GNOLL", 15932, 110, 15995 },
	{ "HOBGOBLIN", 15930, 111, 15996 },
	{ "KOBOLD", 15929, 112, 15997 },
	{ "OGRE", 15933, 113, 15998 },
	{ "SKELETON", 15937, 115, 15999 },
	{ "SPIDER", 15941, 116, 16000 },
};

// clowncol.2da of GemRB's unhardcoded BG1 tables.
static const uint8 kHairColors[] = { 0, 1, 2, 3, 4, 5, 6, 7, 79, 80, 81, 82, 110, 111 };
static const uint8 kSkinColors[] = { 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 83, 84, 85, 86, 87, 88, 89, 90, 105, 106, 107, 108, 109, 112, 113, 114 };
static const uint8 kMajorColors[] = { 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66 };
static const uint8 kMinorColors[] = { 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66 };

static const CharGenAlignment kAlignments[] = {
	{ "LAWFUL_GOOD", 7186, 9603, 1102, "L_G", 0x14 },
	{ "NEUTRAL_GOOD", 7183, 9606, 1105, "N_G", 0x24 },
	{ "CHAOTIC_GOOD", 7189, 9609, 1108, "C_G", 0x05 },
	{ "LAWFUL_NEUTRAL", 7188, 9604, 1104, "L_N", 0x18 },
	{ "TRUE_NEUTRAL", 7185, 9608, 1106, "N_N", 0x28 },
	{ "CHAOTIC_NEUTRAL", 7191, 9610, 1109, "C_N", 0x09 },
	{ "LAWFUL_EVIL", 7187, 9605, 1103, "L_E", 0x12 },
	{ "NEUTRAL_EVIL", 7184, 9607, 1107, "N_E", 0x22 },
	{ "CHAOTIC_EVIL", 7190, 9611, 1110, "C_E", 0x03 },
};

// allowed: HUMAN ELF HALF_ELF DWARF HALFLING GNOME (0 no, 1 yes, 2 yes, illusionist).
static const CharGenClass kClasses[] = {
	{ "FIGHTER", 7201, 9556, 1076, 0, { 1, 1, 1, 1, 1, 1 } },
	{ "RANGER", 7200, 9557, 1077, 0, { 1, 1, 1, 0, 0, 0 } },
	{ "PALADIN", 7217, 9558, 1078, 0, { 1, 0, 1, 0, 0, 0 } },
	{ "CLERIC", 7204, 9559, 1079, 0, { 1, 1, 1, 1, 1, 1 } },
	{ "DRUID", 7210, 9560, 1080, 0, { 1, 1, 1, 1, 0, 0 } },
	{ "MAGE", 7203, 9563, 1081, 0, { 1, 1, 1, 0, 0, 2 } },
	{ "THIEF", 7202, 9561, 1082, 0, { 1, 1, 1, 1, 1, 1 } },
	{ "BARD", 7206, 9562, 1083, 0, { 1, 1, 1, 1, 1, 1 } },
	{ "FIGHTER_THIEF", 7205, 9572, 1052, 1, { 0, 1, 1, 1, 1, 1 } },
	{ "FIGHTER_CLERIC", 7211, 9573, 1053, 1, { 0, 1, 1, 1, 1, 1 } },
	{ "FIGHTER_MAGE", 7213, 9574, 1056, 1, { 0, 1, 1, 0, 0, 2 } },
	{ "MAGE_THIEF", 7216, 9575, 1057, 1, { 0, 1, 1, 0, 0, 2 } },
	{ "CLERIC_MAGE", 7207, 9577, 1058, 1, { 0, 1, 1, 0, 0, 2 } },
	{ "CLERIC_THIEF", 7209, 9578, 1065, 1, { 0, 1, 1, 1, 1, 1 } },
	{ "FIGHTER_DRUID", 7212, 9579, 1066, 1, { 0, 1, 1, 0, 0, 0 } },
	{ "CLERIC_RANGER", 7208, 9580, 1073, 1, { 0, 1, 1, 0, 0, 0 } },
	{ "FIGHTER_MAGE_THIEF", 7215, 9576, 1074, 1, { 0, 1, 1, 0, 0, 0 } },
	{ "FIGHTER_MAGE_CLERIC", 7214, 9581, 1075, 1, { 0, 1, 1, 0, 0, 0 } },
};

static const CharGenPortrait kPortraits[] = {
	{ "AJANTIS", 1, 3, 3, 3, 3 },
	{ "CORAN", 1, 2, 12, 45, 55 },
	{ "EDWIN", 1, 91, 12, 47, 41 },
	{ "ELDOTH", 1, 0, 12, 41, 36 },
	{ "GARRICK", 1, 0, 12, 41, 50 },
	{ "GENDWRF", 1, 4, 85, 39, 49 },
	{ "GENMELF", 1, 91, 10, 21, 60 },
	{ "GENMHLF", 1, 0, 85, 40, 50 },
	{ "KAGAIN", 1, 6, 85, 39, 49 },
	{ "KHALID", 1, 4, 12, 48, 41 },
	{ "KIVAN", 1, 1, 84, 37, 39 },
	{ "MAN1", 1, 6, 84, 52, 46 },
	{ "MAN2", 1, 2, 12, 37, 45 },
	{ "MINSC", 1, 110, 84, 60, 58 },
	{ "MONTAR", 1, 91, 10, 21, 60 },
	{ "QUAYLE", 1, 110, 8, 45, 66 },
	{ "TIAX", 1, 91, 12, 47, 46 },
	{ "XAN", 1, 0, 12, 45, 64 },
	{ "XZAR", 1, 91, 84, 54, 66 },
	{ "YESLICK", 1, 92, 85, 39, 42 },
	{ "ALORA", 2, 4, 12, 60, 45 },
	{ "BRANWE", 2, 3, 12, 37, 41 },
	{ "DYNAHEI", 2, 2, 12, 60, 46 },
	{ "FALDORN", 2, 91, 84, 54, 65 },
	{ "IMOEN", 2, 4, 13, 45, 61 },
	{ "JAHEIRA", 2, 92, 12, 41, 65 },
	{ "SAFANA", 2, 4, 12, 60, 60 },
	{ "SHARTEL", 2, 4, 84, 66, 21 },
	{ "SKIE", 2, 0, 12, 66, 45 },
	{ "VICONIA", 2, 79, 83, 54, 60 },
	{ "WOMAN1", 2, 3, 84, 61, 46 },
	{ "WOMAN2", 2, 0, 84, 45, 58 },
};


namespace CharGenData {

const CharGenRace*
Races(size_t& count)
{
	count = sizeof(kRaces) / sizeof(kRaces[0]);
	return kRaces;
}


const CharGenClass*
Classes(size_t& count)
{
	count = sizeof(kClasses) / sizeof(kClasses[0]);
	return kClasses;
}


const CharGenAlignment*
Alignments(size_t& count)
{
	count = sizeof(kAlignments) / sizeof(kAlignments[0]);
	return kAlignments;
}


const uint8*
Colors(ColorKind kind, size_t& count)
{
	switch (kind) {
		case COLOR_HAIR:
			count = sizeof(kHairColors);
			return kHairColors;
		case COLOR_SKIN:
			count = sizeof(kSkinColors);
			return kSkinColors;
		case COLOR_MAJOR:
			count = sizeof(kMajorColors);
			return kMajorColors;
		case COLOR_MINOR:
			break;
	}
	count = sizeof(kMinorColors);
	return kMinorColors;
}


const CharGenHatedRace*
HatedRaces(size_t& count)
{
	count = sizeof(kHatedRaces) / sizeof(kHatedRaces[0]);
	return kHatedRaces;
}


const CharGenSkill*
Skills(size_t& count)
{
	count = sizeof(kSkills) / sizeof(kSkills[0]);
	return kSkills;
}


const CharGenProficiency*
Proficiencies(size_t& count)
{
	count = sizeof(kProficiencies) / sizeof(kProficiencies[0]);
	return kProficiencies;
}


const CharGenPortrait*
Portraits(size_t& count)
{
	count = sizeof(kPortraits) / sizeof(kPortraits[0]);
	return kPortraits;
}


int
RaceColumn(const char* raceName)
{
	static const char* kColumns[] = { "HUMAN", "ELF", "HALF_ELF", "DWARF", "HALFLING", "GNOME" };
	for (int i = 0; i < 6; i++) {
		if (strcasecmp(kColumns[i], raceName) == 0)
			return i;
	}
	return -1;
}


const CharGenRace*
FindRace(const char* name)
{
	for (const CharGenRace& race : kRaces) {
		if (strcasecmp(race.name, name) == 0)
			return &race;
	}
	return nullptr;
}


const CharGenClass*
FindClass(const char* name)
{
	for (const CharGenClass& entry : kClasses) {
		if (strcasecmp(entry.name, name) == 0)
			return &entry;
	}
	return nullptr;
}


const CharGenAlignment*
FindAlignment(const char* name)
{
	for (const CharGenAlignment& entry : kAlignments) {
		if (strcasecmp(entry.name, name) == 0)
			return &entry;
	}
	return nullptr;
}

}
