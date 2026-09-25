/*
 * CharGenData.h
 *
 * What character creation shows and chooses from: the races, classes,
 * alignments and portraits of BG1 with the strrefs of their names and
 * descriptions. The original keeps them in its executable (BG1 has no 2DA for
 * them), so they are GemRB's unhardcoded races/classes/aligns/pictures tables.
 */

#pragma once

#include "SupportDefs.h"

#include <cstddef>

struct CharGenRace {
	const char* name;	// RACE.IDS / ABRACERQ row
	uint32 nameRef;
	uint32 descRef;
	uint32 capRef;		// the name as written in the summary
};

struct CharGenClass {
	const char* name;	// CLASS.IDS / ABCLASRQ row, "FIGHTER_MAGE" for a multiclass
	uint32 nameRef;
	uint32 descRef;
	uint32 capRef;
	bool multi;
	// May this race be it: HUMAN ELF HALF_ELF DWARF HALFLING GNOME; 0 no, 1 yes,
	// 2 yes, as an illusionist (a gnome mage).
	int8 allowed[6];
};

struct CharGenAlignment {
	const char* name;	// ALIGNMEN.IDS
	uint32 nameRef;
	uint32 descRef;
	uint32 capRef;
	const char* code;	// "L_G"...
};

struct CharGenPortrait {
	const char* name;	// the portraits' common part: <name>G (chargen), L, S
	int gender;		// 1 male, 2 female
};

struct CharGenProficiency {
	const char* name;	// weapprof.2da's row, CLASWEAP's column
	uint32 nameRef;
	uint32 descRef;
};

struct CharGenSkill {
	const char* name;	// skills.2da's row, SKILLRAC / SKILLDEX's column
	uint32 capRef;		// how it is named in the window and the summary
	uint32 descRef;
	uint32 creOffset;	// the byte of the CRE header that holds it
};

struct CharGenHatedRace {
	const char* name;
	uint32 nameRef;
	uint8 id;		// RACE.IDS, what the CRE keeps
	uint32 helpRef;
};

namespace CharGenData {
	// The creatures a ranger may take as its racial enemy.
	const CharGenHatedRace* HatedRaces(size_t& count);
	// The thief skills in the order of the window: pick pockets, open locks, find
	// traps, stealth.
	const CharGenSkill* Skills(size_t& count);
	// The weapon proficiencies in the order of their CRE bytes (0x6e on).
	const CharGenProficiency* Proficiencies(size_t& count);
	const CharGenRace* Races(size_t& count);
	const CharGenClass* Classes(size_t& count);
	const CharGenAlignment* Alignments(size_t& count);
	const CharGenPortrait* Portraits(size_t& count);

	// The race's column of CharGenClass::allowed, -1 for a name that isn't one.
	int RaceColumn(const char* raceName);
	const CharGenRace* FindRace(const char* name);
	const CharGenClass* FindClass(const char* name);
	const CharGenAlignment* FindAlignment(const char* name);
}
