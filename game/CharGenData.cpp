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

static const CharGenAlignment kAlignments[] = {
	{ "LAWFUL_GOOD", 7186, 9603, 1102, "L_G" },
	{ "NEUTRAL_GOOD", 7183, 9606, 1105, "N_G" },
	{ "CHAOTIC_GOOD", 7189, 9609, 1108, "C_G" },
	{ "LAWFUL_NEUTRAL", 7188, 9604, 1104, "L_N" },
	{ "TRUE_NEUTRAL", 7185, 9608, 1106, "N_N" },
	{ "CHAOTIC_NEUTRAL", 7191, 9610, 1109, "C_N" },
	{ "LAWFUL_EVIL", 7187, 9605, 1103, "L_E" },
	{ "NEUTRAL_EVIL", 7184, 9607, 1107, "N_E" },
	{ "CHAOTIC_EVIL", 7190, 9611, 1110, "C_E" },
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
	{ "AJANTIS", 1 },
	{ "CORAN", 1 },
	{ "EDWIN", 1 },
	{ "ELDOTH", 1 },
	{ "GARRICK", 1 },
	{ "GENDWRF", 1 },
	{ "GENMELF", 1 },
	{ "GENMHLF", 1 },
	{ "KAGAIN", 1 },
	{ "KHALID", 1 },
	{ "KIVAN", 1 },
	{ "MAN1", 1 },
	{ "MAN2", 1 },
	{ "MINSC", 1 },
	{ "MONTAR", 1 },
	{ "QUAYLE", 1 },
	{ "TIAX", 1 },
	{ "XAN", 1 },
	{ "XZAR", 1 },
	{ "YESLICK", 1 },
	{ "ALORA", 2 },
	{ "BRANWE", 2 },
	{ "DYNAHEI", 2 },
	{ "FALDORN", 2 },
	{ "IMOEN", 2 },
	{ "JAHEIRA", 2 },
	{ "SAFANA", 2 },
	{ "SHARTEL", 2 },
	{ "SKIE", 2 },
	{ "VICONIA", 2 },
	{ "WOMAN1", 2 },
	{ "WOMAN2", 2 },
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
