/*
 * CharacterBuilder.cpp
 */

#include "CharacterBuilder.h"

#include "2DAResource.h"
#include "Core.h"
#include "IDSResource.h"
#include "ResManager.h"

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
		std::string upper = align;
		for (char& c : upper) c = (char)toupper((unsigned char)c);
		const char* e = strchr(kEthical, upper[0]);
		const char* m = strchr(kMoral, upper[1]);
		if (e == NULL || m == NULL)
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


void
CharacterBuilder::Print() const
{
	std::cout << "Character:" << std::endl;
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

	std::vector<std::string> problems;
	if (IsComplete(problems)) {
		std::cout << "  -> ready" << std::endl;
	} else {
		for (const std::string& p : problems)
			std::cout << "  ! " << p << std::endl;
	}
}
