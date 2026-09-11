/*
 * CharacterBuilder.cpp
 */

#include "CharacterBuilder.h"

#include "2DAResource.h"
#include "Core.h"
#include "IDSResource.h"
#include "ResManager.h"
#include "SPLResource.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <stdexcept>

// Column suffix for each ability, matching the *_STR/*_DEX/... columns
// in ABCLASRQ / ABRACERQ.
static const char* kAbilitySuffix[CharacterBuilder::kNumAbilities] = {
	"STR", "DEX", "CON", "INT", "WIS", "CHR"
};


// IntegerValueFor() throws for any (row, column) the table doesn't list
// (it does not fall back to the file's own default) - wrap every lookup.
static int
_TableInt(const char* tableName, const std::string& row, const std::string& column,
			int fallback)
{
	TWODAResource* table = gResManager->Get2DA(tableName);
	if (table == NULL)
		return fallback;
	int value = fallback;
	try {
		value = table->IntegerValueFor(row.c_str(), column.c_str());
	} catch (const std::exception&) {
		value = fallback;
	}
	gResManager->ReleaseResource(table);
	return value;
}


static uint32
_IDValue(const char* idsName, const std::string& symbol, bool& ok)
{
	ok = false;
	IDSResource* ids = gResManager->GetIDS(idsName);
	if (ids == NULL)
		return 0;
	uint32 value = 0;
	try {
		value = ids->IDForString(symbol);
		ok = true;
	} catch (const std::exception&) {
		ok = false;
	}
	gResManager->ReleaseResource(ids);
	return value;
}


CharacterBuilder::CharacterBuilder()
{
	Reset();
}


void
CharacterBuilder::Reset()
{
	fGender.clear();
	fRace.clear();
	fClass.clear();
	fKit.clear();
	fAlignment.clear();
	fAlignmentValue = 0;
	for (int i = 0; i < kNumAbilities; i++)
		fAbilities[i] = 0;
	fName.clear();
	fPortraitSmall.clear();
	fPortraitLarge.clear();
	for (int i = 0; i < 7; i++)
		fColors[i] = -1;
	fSpells.clear();
	fOpenLocksSkill = 0;
	fFindTrapsSkill = 0;
}


bool
CharacterBuilder::SetThiefSkill(const std::string& which, int value)
{
	if (value < 0)
		value = 0;
	if (value > 255)
		value = 255;
	if (strcasecmp(which.c_str(), "openlocks") == 0) {
		fOpenLocksSkill = value;
		return true;
	}
	if (strcasecmp(which.c_str(), "findtraps") == 0) {
		fFindTrapsSkill = value;
		return true;
	}
	return false;
}


void
CharacterBuilder::SetName(const std::string& name)
{
	fName = name.substr(0, 31);
}


void
CharacterBuilder::SetPortraits(const std::string& small, const std::string& large)
{
	fPortraitSmall = small.substr(0, 8);
	fPortraitLarge = large.substr(0, 8);
}


bool
CharacterBuilder::IsArcaneCaster() const
{
	if (fClass.find("MAGE") != std::string::npos)
		return true;
	static const char* kArcane[] = {
		"SORCERER", "BARD", "ABJURER", "CONJURER", "DIVINER", "ENCHANTER",
		"ILLUSIONIST", "INVOKER", "NECROMANCER", "TRANSMUTER",
		"BLADE", "JESTER", "SKALD", "WILDMAGE"
	};
	for (const char* name : kArcane) {
		if (strcasecmp(fClass.c_str(), name) == 0)
			return true;
	}
	return false;
}


bool
CharacterBuilder::IsDivineCaster() const
{
	static const char* kDivine[] = {
		"CLERIC", "DRUID", "PALADIN", "RANGER", "TALOS", "HELM", "LATHANDER",
		"TOTEMIC_DRUID", "SHAPESHIFTER", "BEAST_FRIEND"
	};
	for (const char* name : kDivine) {
		if (fClass.find(name) != std::string::npos)
			return true;
	}
	return false;
}


bool
CharacterBuilder::AddSpell(const std::string& resref)
{
	SPLResource* spl = gResManager->GetSPL(resref.c_str());
	if (spl == NULL)
		return false;
	gResManager->ReleaseResource(spl);
	fSpells.push_back(resref.substr(0, 8));
	return true;
}


bool
CharacterBuilder::SetColor(const std::string& which, int index)
{
	static const char* kNames[7] = {
		"metal", "minor", "major", "skin", "leather", "armor", "hair"
	};
	for (int i = 0; i < 7; i++) {
		if (strcasecmp(which.c_str(), kNames[i]) == 0) {
			fColors[i] = (index >= 0 && index <= 255) ? index : -1;
			return true;
		}
	}
	return false;
}


/* static */
const char*
CharacterBuilder::AbilityName(int ability)
{
	if (ability < 0 || ability >= kNumAbilities)
		return "?";
	return kAbilitySuffix[ability];
}


bool
CharacterBuilder::SetGender(const std::string& gender)
{
	bool ok = false;
	_IDValue("GENDER", gender, ok);
	if (!ok)
		return false;
	fGender = gender;
	return true;
}


bool
CharacterBuilder::SetRace(const std::string& race)
{
	// A race is "known" if ABRACERQ lists a MAX_STR for it (every real
	// race row has one; a bad name yields the throwing-path fallback).
	if (_TableInt("ABRACERQ", race, "MAX_STR", 0) <= 0)
		return false;
	fRace = race;
	// Pull any rolled ability that now sits outside the racial range
	// back into it. Class minimums are deliberately NOT auto-applied
	// here - an unmet class minimum means "reroll", not "silently bump"
	// (IsComplete()/Print() flag it).
	for (int i = 0; i < kNumAbilities; i++) {
		if (fAbilities[i] != 0) {
			fAbilities[i] = std::min(std::max(fAbilities[i], _RacialMinimum(i)),
									_RacialMaximum(i));
			fAbilities[i] = std::min(fAbilities[i], 18);
		}
	}
	return true;
}


bool
CharacterBuilder::SetClass(const std::string& className)
{
	bool ok = false;
	_IDValue("CLASS", className, ok);
	if (!ok)
		return false;
	fClass = className;
	// A class change can invalidate the current alignment.
	if (fAlignmentValue != 0
		&& !_AlignmentAllowedForClass(fClass, fAlignmentValue)) {
		fAlignment.clear();
		fAlignmentValue = 0;
	}
	return true;
}


bool
CharacterBuilder::SetKit(const std::string& kitName)
{
	if (kitName.empty()) {
		fKit.clear();
		return true;
	}
	bool ok = false;
	_IDValue("KIT", kitName, ok);
	if (!ok)
		return false;
	// Kit<->class compatibility is validated in a later sub-phase.
	fKit = kitName;
	return true;
}


bool
CharacterBuilder::SetAlignment(const std::string& align)
{
	// Accept an ALIGNMEN.IDS symbol ("CHAOTIC_GOOD") or a 2-letter code
	// ("CG"). Codes: first letter L/N/C (ethical), second G/N/E (moral).
	uint8 value = 0;
	if (align.size() == 2) {
		static const char* kEthical = "LNC";
		static const char* kMoral = "GNE";
		char ec = (char)toupper((unsigned char)align[0]);
		char mc = (char)toupper((unsigned char)align[1]);
		if (ec == 'T') ec = 'N'; // "TN" for True Neutral
		if (mc == 'T') mc = 'N';
		const char* e = strchr(kEthical, ec);
		const char* m = strchr(kMoral, mc);
		if (e == NULL || m == NULL || ec == '\0' || mc == '\0')
			return false;
		value = (uint8)(((e - kEthical + 1) << 4) | (m - kMoral + 1));
	} else {
		bool ok = false;
		value = (uint8)_IDValue("ALIGNMEN", align, ok);
		if (!ok || value < 0x11 || value > 0x33)
			return false;
	}

	if (!fClass.empty() && !_AlignmentAllowedForClass(fClass, value))
		return false;

	fAlignmentValue = value;
	IDSResource* ids = gResManager->GetIDS("ALIGNMEN");
	if (ids != NULL) {
		try {
			fAlignment = ids->StringForID(value);
		} catch (const std::exception&) {
			fAlignment = align;
		}
		gResManager->ReleaseResource(ids);
	} else {
		fAlignment = align;
	}
	return true;
}


bool
CharacterBuilder::_AlignmentAllowedForClass(const std::string& className,
											uint8 alignmentValue) const
{
	// ALIGNMNT columns, in order: L_G L_N L_E N_G N_N N_E C_G C_N C_E.
	static const char* kColumns[9] = {
		"L_G", "L_N", "L_E", "N_G", "N_N", "N_E", "C_G", "C_N", "C_E"
	};
	int ethical = (alignmentValue >> 4) - 1; // 0=lawful 1=neutral 2=chaotic
	int moral = (alignmentValue & 0x0f) - 1; // 0=good 1=neutral 2=evil
	if (ethical < 0 || ethical > 2 || moral < 0 || moral > 2)
		return false;
	const char* column = kColumns[ethical * 3 + moral];
	// Default to allowed if the table or row is missing.
	return _TableInt("ALIGNMNT", className, column, 1) != 0;
}


int
CharacterBuilder::_ClassMinimum(int ability) const
{
	if (fClass.empty())
		return 0;
	return _TableInt("ABCLASRQ", fClass,
					std::string("MIN_") + kAbilitySuffix[ability], 0);
}


int
CharacterBuilder::_RacialMinimum(int ability) const
{
	if (fRace.empty())
		return 3;
	return _TableInt("ABRACERQ", fRace,
					std::string("MIN_") + kAbilitySuffix[ability], 3);
}


int
CharacterBuilder::_RacialMaximum(int ability) const
{
	if (fRace.empty())
		return 18;
	return _TableInt("ABRACERQ", fRace,
					std::string("MAX_") + kAbilitySuffix[ability], 18);
}


int
CharacterBuilder::AbilityMin(int ability) const
{
	if (ability < 0 || ability >= kNumAbilities)
		return 3;
	return std::max(_RacialMinimum(ability), _ClassMinimum(ability));
}


int
CharacterBuilder::AbilityMax(int ability) const
{
	if (ability < 0 || ability >= kNumAbilities)
		return 18;
	return std::min(18, _RacialMaximum(ability));
}


int
CharacterBuilder::RollAbilities()
{
	if (fRace.empty() || fClass.empty())
		return 0;

	const int kMaxAttempts = 100000;
	for (int attempt = 0; attempt < kMaxAttempts; attempt++) {
		int rolled[kNumAbilities];
		bool allMet = true;
		for (int i = 0; i < kNumAbilities; i++) {
			int v = Core::RollDice(3, 6, 0);
			v = std::min(std::max(v, _RacialMinimum(i)), _RacialMaximum(i));
			v = std::min(v, 18);
			rolled[i] = v;
			if (v < _ClassMinimum(i))
				allMet = false;
		}
		if (allMet) {
			for (int i = 0; i < kNumAbilities; i++)
				fAbilities[i] = rolled[i];
			return AbilityTotal();
		}
	}
	return 0; // impossible race/class combination
}


bool
CharacterBuilder::SetAbility(int ability, int value)
{
	if (ability < 0 || ability >= kNumAbilities)
		return false;
	if (value < AbilityMin(ability) || value > AbilityMax(ability))
		return false;
	fAbilities[ability] = value;
	return true;
}


int
CharacterBuilder::Ability(int ability) const
{
	if (ability < 0 || ability >= kNumAbilities)
		return 0;
	return fAbilities[ability];
}


int
CharacterBuilder::AbilityTotal() const
{
	int total = 0;
	for (int i = 0; i < kNumAbilities; i++)
		total += fAbilities[i];
	return total;
}


bool
CharacterBuilder::IsComplete(std::vector<std::string>& problems) const
{
	problems.clear();
	if (fGender.empty())
		problems.push_back("gender not set");
	if (fRace.empty())
		problems.push_back("race not set");
	if (fClass.empty())
		problems.push_back("class not set");
	if (fAlignmentValue == 0)
		problems.push_back("alignment not set");
	else if (!fClass.empty()
			&& !_AlignmentAllowedForClass(fClass, fAlignmentValue))
		problems.push_back("alignment not allowed for this class");

	if (!fRace.empty() && !fClass.empty()) {
		for (int i = 0; i < kNumAbilities; i++) {
			if (fAbilities[i] < AbilityMin(i)) {
				problems.push_back(std::string(kAbilitySuffix[i])
					+ " below the minimum ("
					+ std::to_string(AbilityMin(i)) + ")");
			} else if (fAbilities[i] > AbilityMax(i)) {
				problems.push_back(std::string(kAbilitySuffix[i])
					+ " above the maximum ("
					+ std::to_string(AbilityMax(i)) + ")");
			}
		}
	}
	return problems.empty();
}


namespace {

void _PutU8(std::vector<uint8>& b, size_t at, uint8 v) { b[at] = v; }
void _PutU16(std::vector<uint8>& b, size_t at, uint16 v)
{
	b[at] = (uint8)(v & 0xff);
	b[at + 1] = (uint8)(v >> 8);
}
void _PutU32(std::vector<uint8>& b, size_t at, uint32 v)
{
	for (int i = 0; i < 4; i++) b[at + i] = (uint8)((v >> (8 * i)) & 0xff);
}
void _PutStr(std::vector<uint8>& b, size_t at, const std::string& s, size_t max)
{
	for (size_t i = 0; i < max; i++)
		b[at + i] = (i < s.size()) ? (uint8)s[i] : 0;
}

} // namespace


bool
CharacterBuilder::BuildCREData(std::vector<uint8>& out) const
{
	std::vector<std::string> problems;
	if (!IsComplete(problems))
		return false;

	bool ok = false;
	uint32 raceID = _IDValue("RACE", fRace, ok);
	if (!ok) return false;
	uint32 classID = _IDValue("CLASS", fClass, ok);
	if (!ok) return false;
	uint32 genderID = _IDValue("GENDER", fGender, ok);
	if (!ok) return false;

	const size_t kHeaderSize = 0x2d4;
	const size_t kSlotCount = 40;               // BG2 CRE v1
	const size_t kSlotTableSize = kSlotCount * 2;

	// Starting spellbook (level 1). Known = every spell the builder was
	// given; memorized = as many as the level-1 slot table grants.
	const bool divine = IsDivineCaster() && !IsArcaneCaster();
	const bool caster = (IsArcaneCaster() || divine) && !fSpells.empty();
	const uint16 spellType = divine ? 0 : 1; // 0 priest, 1 wizard
	const size_t knownCount = caster ? fSpells.size() : 0;
	const size_t level1Slots = caster
		? (size_t)std::max(1, _TableInt(divine ? "MXSPLPRS" : "MXSPLWIZ", "1", "1", 1)) : 0;
	const size_t memorizedCount = std::min(knownCount, level1Slots);
	const size_t memoInfoCount = caster ? 1 : 0;

	const size_t knownOffset = kHeaderSize;
	const size_t memoInfoOffset = knownOffset + knownCount * 12;
	const size_t memorizedOffset = memoInfoOffset + memoInfoCount * 16;
	const size_t tailOffset = memorizedOffset + memorizedCount * 12; // effects/items/slots
	const size_t kTotal = tailOffset + kSlotTableSize;

	out.assign(kTotal, 0);

	std::memcpy(out.data(), "CRE V1.0", 8);
	_PutU32(out, 0x08, 0xffffffff);  // long name strref  (A6)
	_PutU32(out, 0x0c, 0xffffffff);  // short name strref (A6)
	_PutU32(out, 0x10, 0);           // flags
	_PutU32(out, 0x18, 0);           // experience
	_PutU32(out, 0x1c, 0);           // gold (A6)
	_PutU32(out, 0x20, 0);           // permanent status

	// HP starts at 0 and THAC0/saves at conservative placeholders: the
	// engine fills in the real level-1 values from the class tables
	// (Actor::_Init() runs the level-up path because class level is 0).
	_PutU16(out, 0x24, 0);           // current HP
	_PutU16(out, 0x26, 0);           // maximum HP
	_PutU32(out, 0x28, genderID == 2 ? 0x6010 : 0x6000); // player animation

	// Paperdoll/avatar colours. Order: metal, minor, major, skin,
	// leather, armor, hair - a generic default per slot unless the
	// builder was given an explicit palette index.
	static const uint8 kDefaultColors[7] = { 0x3a, 0x2d, 0x3b, 0x54, 0x5d, 0x60, 0x02 };
	for (int i = 0; i < 7; i++)
		_PutU8(out, 0x2c + i, fColors[i] >= 0 ? (uint8)fColors[i] : kDefaultColors[i]);
	_PutU8(out, 0x33, 1);            // EFF structure version (v2, BG2)

	if (!fPortraitSmall.empty())
		_PutStr(out, 0x34, fPortraitSmall, 8);
	if (!fPortraitLarge.empty())
		_PutStr(out, 0x3c, fPortraitLarge, 8);

	_PutU8(out, 0x44, 10);           // reputation
	_PutU16(out, 0x46, 10);          // AC natural
	_PutU16(out, 0x48, 10);          // AC effective
	_PutU8(out, 0x52, 20);           // THAC0 - engine recomputes from THAC0.2da
	_PutU8(out, 0x53, 1);            // # attacks
	for (int i = 0; i < 5; i++) _PutU8(out, 0x54 + i, 20); // saves - engine recomputes
	_PutU8(out, 0x67, (uint8)fOpenLocksSkill); // Fase 8: Door lock checks
	_PutU8(out, 0x69, (uint8)fFindTrapsSkill); // Fase 8: trap find/disarm

	// Class levels stay at 0 so Actor::_Init() runs the level-up path to
	// derive level-1 HP/THAC0/saves from the class tables. _CheckLevelUp
	// reads all three per-class slots; a multi-class name just needs the
	// unused trailing slots to also be 0 (which zeroed init already is).
	_PutU8(out, 0x234, 0);
	_PutU8(out, 0x235, 0);
	_PutU8(out, 0x236, 0);

	_PutU8(out, 0x237, (uint8)genderID);   // sex
	_PutU8(out, 0x238, (uint8)Ability(STR));
	_PutU8(out, 0x239, 0);                 // exceptional strength (later)
	_PutU8(out, 0x23a, (uint8)Ability(INT));
	_PutU8(out, 0x23b, (uint8)Ability(WIS));
	_PutU8(out, 0x23c, (uint8)Ability(DEX));
	_PutU8(out, 0x23d, (uint8)Ability(CON));
	_PutU8(out, 0x23e, (uint8)Ability(CHR));
	_PutU8(out, 0x23f, 10);                // morale
	_PutU32(out, 0x244, 0x40000000);       // kit = TRUECLASS (kits: later)

	_PutU8(out, 0x270, 2);                 // EA = PC
	_PutU8(out, 0x271, 1);                 // general = HUMANOID
	_PutU8(out, 0x272, (uint8)raceID);
	_PutU8(out, 0x273, (uint8)classID);
	_PutU8(out, 0x274, 0);                 // specific = NORMAL
	_PutU8(out, 0x275, (uint8)genderID);   // gender
	_PutU8(out, 0x27b, fAlignmentValue);
	_PutU16(out, 0x27c, 0xffff);           // global actor enum (unset)
	_PutU16(out, 0x27e, 0xffff);           // local actor enum (unset)
	_PutStr(out, 0x280, fName.empty() ? "Player1" : fName, 32); // death variable

	// Section offsets. Layout: header | known spells | spell memo info |
	// memorized spells | (empty effects/items) | item slots.
	_PutU32(out, 0x2a0, (uint32)knownOffset);    _PutU32(out, 0x2a4, (uint32)knownCount);
	_PutU32(out, 0x2a8, (uint32)memoInfoOffset); _PutU32(out, 0x2ac, (uint32)memoInfoCount);
	_PutU32(out, 0x2b0, (uint32)memorizedOffset);_PutU32(out, 0x2b4, (uint32)memorizedCount);
	_PutU32(out, 0x2b8, (uint32)tailOffset);                                  // item slots
	_PutU32(out, 0x2bc, (uint32)tailOffset);     _PutU32(out, 0x2c0, 0);      // items
	_PutU32(out, 0x2c4, (uint32)tailOffset);     _PutU32(out, 0x2c8, 0);      // effects

	// Known spells: resref(8), level(2, 0-indexed on disk = level 1),
	// type(2, 1 = wizard).
	for (size_t i = 0; i < knownCount; i++) {
		size_t o = knownOffset + i * 12;
		_PutStr(out, o, fSpells[i], 8);
		_PutU16(out, o + 8, 0); // level 1
		_PutU16(out, o + 10, spellType);
	}
	// Spell memorization info: level(2), numMemorizable(2),
	// numMemorizableEffective(2), type(2), firstMemorizedIndex(4),
	// memorizedCount(4).
	if (memoInfoCount == 1) {
		_PutU16(out, memoInfoOffset + 0, 0);   // level 1
		_PutU16(out, memoInfoOffset + 2, (uint16)level1Slots);
		_PutU16(out, memoInfoOffset + 4, (uint16)level1Slots);
		_PutU16(out, memoInfoOffset + 6, spellType);
		_PutU32(out, memoInfoOffset + 8, 0);   // first memorized index
		_PutU32(out, memoInfoOffset + 12, (uint32)memorizedCount);
	}
	// Memorized spells: resref(8), flags(4, bit0 = memorized/available).
	for (size_t i = 0; i < memorizedCount; i++) {
		size_t o = memorizedOffset + i * 12;
		_PutStr(out, o, fSpells[i], 8);
		_PutU32(out, o + 8, 1);
	}

	for (size_t i = 0; i < kSlotCount; i++)
		_PutU16(out, tailOffset + i * 2, 0xffff); // empty slot

	return true;
}


void
CharacterBuilder::Print() const
{
	std::cout << "Character:" << std::endl;
	std::cout << "  Name:      " << (fName.empty() ? "-" : fName) << std::endl;
	std::cout << "  Gender:    " << (fGender.empty() ? "-" : fGender) << std::endl;
	std::cout << "  Race:      " << (fRace.empty() ? "-" : fRace) << std::endl;
	std::cout << "  Class:     " << (fClass.empty() ? "-" : fClass) << std::endl;
	std::cout << "  Kit:       " << (fKit.empty() ? "-" : fKit) << std::endl;
	std::cout << "  Alignment: " << (fAlignment.empty() ? "-" : fAlignment);
	if (fAlignmentValue != 0)
		std::cout << " (0x" << std::hex << (int)fAlignmentValue << std::dec << ")";
	std::cout << std::endl;

	for (int i = 0; i < kNumAbilities; i++) {
		std::cout << "  " << kAbilitySuffix[i] << ": " << fAbilities[i];
		if (!fRace.empty() && !fClass.empty()) {
			std::cout << "  [" << AbilityMin(i) << ".." << AbilityMax(i) << "]";
		}
		std::cout << std::endl;
	}
	std::cout << "  Total: " << AbilityTotal() << std::endl;
	if (!fPortraitSmall.empty() || !fPortraitLarge.empty())
		std::cout << "  Portraits: " << fPortraitSmall << " / " << fPortraitLarge << std::endl;
	if (!fSpells.empty()) {
		std::cout << "  Spells:";
		for (const std::string& s : fSpells) std::cout << " " << s;
		std::cout << std::endl;
	}
	if (fOpenLocksSkill > 0 || fFindTrapsSkill > 0) {
		std::cout << "  Open Locks: " << fOpenLocksSkill
			<< "  Find Traps: " << fFindTrapsSkill << std::endl;
	}

	std::vector<std::string> problems;
	if (IsComplete(problems)) {
		std::cout << "  -> ready" << std::endl;
	} else {
		for (const std::string& p : problems)
			std::cout << "  ! " << p << std::endl;
	}
}
