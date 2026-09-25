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

namespace CharGenData {
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
