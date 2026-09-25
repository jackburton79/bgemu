/*
 * CharacterBuilder.cpp
 */

#include "CharacterBuilder.h"

#include "CharGenData.h"

#include "2DAResource.h"
#include "Core.h"
#include "CreResource.h"
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


// True if the class name (a multiclass is its classes joined by "_") has `word`.
static bool
_ClassHas(const std::string& className, const char* word)
{
	size_t start = 0;
	while (start <= className.size()) {
		size_t end = className.find('_', start);
		if (end == std::string::npos)
			end = className.size();
		if (className.compare(start, end - start, word) == 0)
			return true;
		start = end + 1;
	}
	return false;
}


static int32
_IDValue(const char* idsName, const std::string& symbol, bool& ok)
{
	ok = false;
	IDSResource* ids = gResManager->GetIDS(idsName);
	if (ids == NULL)
		return 0;
	int32 value = ids->IDForString(symbol);
	if (value != -1)
		ok = true;
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
	fStrengthExtra = 0;
	std::fill(fProficiencies, fProficiencies + kNumProficiencies, 0);
	fName.clear();
	fPortraitSmall.clear();
	fPortraitLarge.clear();
	for (int i = 0; i < 7; i++)
		fColors[i] = -1;
	fSpells.clear();
	std::fill(fThiefSkills, fThiefSkills + kNumThiefSkills, 0);
	fRacialEnemy = 0;
	fReputation = 10;
	fGold = 0;
	fBiography = 0;
	fStartingStaff = false;
}


bool
CharacterBuilder::SetThiefSkill(const std::string& which, int value)
{
	static const char* kNames[kNumThiefSkills] = { "pickpockets", "openlocks", "findtraps", "stealth" };
	for (int i = 0; i < kNumThiefSkills; i++) {
		if (strcasecmp(which.c_str(), kNames[i]) == 0)
			return SetThiefSkill(i, value);
	}
	return false;
}


// A thief skill is a byte; the caller (the creation window, or a character spec
// file) is trusted with the points to give.
bool
CharacterBuilder::SetThiefSkill(int skill, int points)
{
	if (skill < 0 || skill >= kNumThiefSkills)
		return false;
	fThiefSkills[skill] = std::min(std::max(points, 0), 255);
	return true;
}


int
CharacterBuilder::ThiefSkill(int skill) const
{
	if (skill < 0 || skill >= kNumThiefSkills)
		return 0;
	return fThiefSkills[skill];
}


// skills.2da gives 40 points at the first level to the classes that are thieves
// (alone or in a multiclass).
int
CharacterBuilder::ThiefSkillPoints() const
{
	return _ClassHas(fClass, "THIEF") ? 40 : 0;
}


bool
CharacterBuilder::HasRacialEnemy() const
{
	return _ClassHas(fClass, "RANGER");
}


// The player animation of the character (GemRB's BGCommon.RefreshPDoll): 0x6000
// plus a part for the race (avprefr.2da), one for the class (avprefc.2da) and one
// for the gender (avprefg.2da); the tables are GemRB's.
uint16
CharacterBuilder::AnimationID() const
{
	uint16 id = 0x6000;
	static const struct { const char* race; uint16 part; } kRaces[] = {
		{ "HUMAN", 0 }, { "ELF", 1 }, { "HALF_ELF", 1 }, { "DWARF", 2 },
		{ "HALFLING", 3 }, { "GNOME", 4 }
	};
	for (const auto& race : kRaces) {
		if (fRace == race.race)
			id += race.part;
	}
	static const struct { const char* name; uint16 part; } kClasses[] = {
		{ "MAGE", 0x200 }, { "FIGHTER", 0x100 }, { "CLERIC", 0 }, { "THIEF", 0x300 },
		{ "BARD", 0x300 }, { "PALADIN", 0x100 }, { "FIGHTER_MAGE", 0x100 },
		{ "FIGHTER_CLERIC", 0x100 }, { "FIGHTER_THIEF", 0x100 },
		{ "FIGHTER_MAGE_THIEF", 0x100 }, { "DRUID", 0 }, { "RANGER", 0x100 },
		{ "MAGE_THIEF", 0x300 }, { "CLERIC_MAGE", 0 }, { "CLERIC_THIEF", 0 },
		{ "FIGHTER_DRUID", 0x100 }, { "FIGHTER_MAGE_CLERIC", 0x100 }, { "CLERIC_RANGER", 0 }
	};
	uint16 classPart = 0;
	for (const auto& entry : kClasses) {
		if (fClass == entry.name)
			classPart = entry.part;
	}
	id += classPart;
	if (fGender == "FEMALE")
		id += 0x10;
	return id;
}


void
CharacterBuilder::ClearAbilities()
{
	std::fill(fAbilities, fAbilities + kNumAbilities, 0);
	fStrengthExtra = 0;
}


void
CharacterBuilder::ClearSkills()
{
	std::fill(fProficiencies, fProficiencies + kNumProficiencies, 0);
	std::fill(fThiefSkills, fThiefSkills + kNumThiefSkills, 0);
	fRacialEnemy = 0;
	fSpells.clear();
}


void
CharacterBuilder::ClearColors()
{
	std::fill(fColors, fColors + 7, -1);
}


// What a new character starts the game with, as GemRB's character creation
// finishes: the reputation of its alignment (REPSTART.2DA), the gold of its class
// (STRTGOLD.2DA, rolled), a quarterstaff and the standard biography.
void
CharacterBuilder::ApplyStartingKit()
{
	const CharGenAlignment* alignment = CharGenData::FindAlignment(fAlignment.c_str());
	if (alignment != NULL)
		fReputation = (uint8)_TableInt("REPSTART", alignment->code, "VALUE", 10);
	fGold = 0;
	if (!fClass.empty()) {
		fGold = Core::RollDice(_TableInt("STRTGOLD", fClass, "ROLLS", 0),
			_TableInt("STRTGOLD", fClass, "SIDES", 0), _TableInt("STRTGOLD", fClass, "MODIFIER", 0))
			* _TableInt("STRTGOLD", fClass, "MULTIPLIER", 1);
	}
	fStartingStaff = true;
	fBiography = 11863;
}


int
CharacterBuilder::ThiefSkillPointsSpent() const
{
	int spent = 0;
	for (int i = 0; i < kNumThiefSkills; i++)
		spent += fThiefSkills[i];
	return spent;
}


int
CharacterBuilder::ThiefSkillBonus(int skill) const
{
	size_t count = 0;
	const CharGenSkill* skills = CharGenData::Skills(count);
	if (skill < 0 || (size_t)skill >= count)
		return 0;
	int bonus = fRace.empty() ? 0 : _TableInt("SKILLRAC", fRace, skills[skill].name, 0);
	// SKILLDEX's rows run from Dexterity 9 up; a lower one has the first row's.
	if (fAbilities[1] != 0)
		bonus += _TableInt("SKILLDEX", std::to_string(std::max(fAbilities[1], 9)),
			skills[skill].name, 0);
	return bonus;
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


// A spell of a character spec: known, and memorized as far as the slots go.
bool
CharacterBuilder::AddSpell(const std::string& resref)
{
	return LearnSpell(resref, true);
}


// Puts a spell in the starting spellbook of its kind (SPL type 2 is a priest's,
// the others are wizard's).
bool
CharacterBuilder::LearnSpell(const std::string& resref, bool memorized)
{
	SPLResource* spl = gResManager->GetSPL(resref.c_str());
	if (spl == NULL)
		return false;
	const bool divine = spl->SpellType() == 2;
	gResManager->ReleaseResource(spl);
	for (spell_entry& entry : fSpells) {
		if (entry.resref == resref.substr(0, 8)) {
			entry.memorized = memorized;
			return true;
		}
	}
	fSpells.push_back({ resref.substr(0, 8), divine, memorized });
	return true;
}


bool
CharacterBuilder::IsSpellKnown(const std::string& resref) const
{
	for (const spell_entry& entry : fSpells) {
		if (entry.resref == resref.substr(0, 8))
			return true;
	}
	return false;
}


std::vector<std::string>
CharacterBuilder::KnownSpells(bool divine) const
{
	std::vector<std::string> spells;
	for (const spell_entry& entry : fSpells) {
		if (entry.divine == divine)
			spells.push_back(entry.resref);
	}
	return spells;
}


int
CharacterBuilder::MemorizedSpellCount(bool divine) const
{
	int count = 0;
	for (const spell_entry& entry : fSpells)
		count += entry.divine == divine && entry.memorized ? 1 : 0;
	return count;
}


void
CharacterBuilder::ClearSpells()
{
	fSpells.clear();
}


// The level-1 wizard spells (SPWI1xx) the class's mage can take, each with
// whether it may learn it: not the ones that exclude its alignment (or, for a
// generalist, wild magic).
std::vector<CharacterBuilder::spell_choice>
CharacterBuilder::MageSpellChoices() const
{
	std::vector<spell_choice> choices;
	if (!_ClassHas(fClass, "MAGE"))
		return choices;
	const CharGenAlignment* alignment = CharGenData::FindAlignment(fAlignment.c_str());
	const uint32 unusable = 0x4000 | (alignment != NULL ? alignment->usability : 0);
	for (int i = 1; i < 100; i++) {
		char name[16];
		snprintf(name, sizeof(name), "SPWI1%02d", i);
		if (!gResManager->ResourceExists(name, RES_SPL))
			continue;
		SPLResource* spl = gResManager->GetSPL(name);
		if (spl == NULL)
			continue;
		choices.push_back({ name, (spl->ExclusionFlags() & unusable) == 0 });
		gResManager->ReleaseResource(spl);
	}
	return choices;
}


// The spells a new mage learns and memorizes (SPLWIZKN.2DA's first level, and
// MXSPLWIZ.2DA's).
int
CharacterBuilder::MageSpellsToLearn() const
{
	return _ClassHas(fClass, "MAGE") ? 2 : 0;
}


int
CharacterBuilder::MageSpellsToMemorize() const
{
	return _ClassHas(fClass, "MAGE") ? std::max(_TableInt("MXSPLWIZ", "1", "1", 0), 0) : 0;
}


// A cleric (or druid) knows every level-1 priest spell its alignment allows.
void
CharacterBuilder::LearnDivineSpells()
{
	const bool cleric = _ClassHas(fClass, "CLERIC");
	const bool druid = _ClassHas(fClass, "DRUID");
	if (!cleric && !druid)
		return;
	const CharGenAlignment* alignment = CharGenData::FindAlignment(fAlignment.c_str());
	const uint32 unusable = alignment != NULL ? alignment->usability : 0;
	for (int i = 1; i < 100; i++) {
		char name[16];
		snprintf(name, sizeof(name), "SPPR1%02d", i);
		if (!gResManager->ResourceExists(name, RES_SPL))
			continue;
		SPLResource* spl = gResManager->GetSPL(name);
		if (spl == NULL)
			continue;
		// Bit 30 keeps the spell from clerics and paladins, bit 31 from druids and
		// rangers.
		const uint32 flags = spl->ExclusionFlags();
		const bool usable = (flags & unusable) == 0
			&& !(cleric && !druid && (flags & 0x40000000))
			&& !(druid && !cleric && (flags & 0x80000000));
		gResManager->ReleaseResource(spl);
		if (usable)
			LearnSpell(name, false);
	}
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


int
CharacterBuilder::Color(const std::string& which) const
{
	static const char* kNames[7] = { "metal", "minor", "major", "skin", "leather", "armor", "hair" };
	for (int i = 0; i < 7; i++) {
		if (strcasecmp(which.c_str(), kNames[i]) == 0)
			return fColors[i];
	}
	return -1;
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
			fAbilities[i] = std::min(std::max(fAbilities[i],
				std::max(_RacialMinimum(i) + AbilityAdjustment(i), 1)), AbilityMax(i));
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
	// What the proficiencies allow depends on the class.
	std::fill(fProficiencies, fProficiencies + kNumProficiencies, 0);
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
CharacterBuilder::IsAlignmentAllowed(const std::string& alignment) const
{
	if (fClass.empty())
		return true;
	bool ok = false;
	const uint8 value = (uint8)_IDValue("ALIGNMEN", alignment, ok);
	return ok && _AlignmentAllowedForClass(fClass, value);
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
CharacterBuilder::AbilityAdjustment(int ability) const
{
	if (fRace.empty() || ability < 0 || ability >= kNumAbilities)
		return 0;
	return _TableInt("ABRACEAD", fRace, std::string("MOD_") + kAbilitySuffix[ability], 0);
}


// The scores are final ones, racial adjustment included, as the character
// creation shows them (a dwarf's Constitution runs 12..19).
int
CharacterBuilder::AbilityMin(int ability) const
{
	if (ability < 0 || ability >= kNumAbilities)
		return 3;
	return std::max(std::max(_RacialMinimum(ability), _ClassMinimum(ability))
		+ AbilityAdjustment(ability), 1);
}


int
CharacterBuilder::AbilityMax(int ability) const
{
	if (ability < 0 || ability >= kNumAbilities)
		return 18;
	return std::min(std::min(18, _RacialMaximum(ability)) + AbilityAdjustment(ability), 25);
}


int
CharacterBuilder::ProficiencyPoints() const
{
	if (fClass.empty())
		return 0;
	int allowed = 0;
	for (int i = 0; i < kNumProficiencies; i++)
		allowed += ProficiencyLimit(i) > 0 ? 1 : 0;
	return std::min(_TableInt("PROFS", fClass, "FIRST_LEVEL", 0), allowed);
}


int
CharacterBuilder::ProficiencyLimit(int proficiency) const
{
	if (fClass.empty() || proficiency < 0 || proficiency >= kNumProficiencies)
		return 0;
	size_t count = 0;
	const CharGenProficiency* profs = CharGenData::Proficiencies(count);
	if (_TableInt("CLASWEAP", fClass, profs[proficiency].name, 0) <= 0)
		return 0;
	return std::min(_TableInt("PROFSMAX", fClass, "FIRST_LEVEL", 1), 5);
}


int
CharacterBuilder::Proficiency(int proficiency) const
{
	if (proficiency < 0 || proficiency >= kNumProficiencies)
		return 0;
	return fProficiencies[proficiency];
}


bool
CharacterBuilder::SetProficiency(int proficiency, int stars)
{
	if (stars < 0 || stars > ProficiencyLimit(proficiency))
		return false;
	fProficiencies[proficiency] = stars;
	return true;
}


int
CharacterBuilder::ProficienciesSpent() const
{
	int spent = 0;
	for (int i = 0; i < kNumProficiencies; i++)
		spent += fProficiencies[i];
	return spent;
}


// The classes that fight at full warrior level may roll an 18/xx strength.
bool
CharacterBuilder::HasExceptionalStrength() const
{
	return _ClassHas(fClass, "FIGHTER") || _ClassHas(fClass, "RANGER")
		|| _ClassHas(fClass, "PALADIN");
}


void
CharacterBuilder::SetStrengthExtra(int value)
{
	fStrengthExtra = std::min(std::max(value, 0), 100);
}


// BG1's roll, as GemRB has it: 3d5 + 3 plus the racial adjustment, held within
// the limits of the race and class (so a class minimum is met by raising the
// score, not by rolling again).
int
CharacterBuilder::RollAbilities()
{
	if (fRace.empty() || fClass.empty())
		return 0;

	for (int i = 0; i < kNumAbilities; i++) {
		const int rolled = Core::RollDice(3, 5, 3 + AbilityAdjustment(i));
		fAbilities[i] = std::min(std::max(rolled, AbilityMin(i)), AbilityMax(i));
	}
	fStrengthExtra = HasExceptionalStrength() ? Core::RollDice(1, 100, 0) : 0;
	return AbilityTotal();
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

	// Starting spellbook (level 1), a book for each kind of magic the class has:
	// known = the spells the builder was given for it; memorized = those marked
	// so, as many as the level-1 slot table grants.
	struct spell_book {
		bool divine;
		bool present;
		size_t slots = 0;
		std::vector<const spell_entry*> known;
		std::vector<const spell_entry*> memorized;
	} books[2] = { { false, IsArcaneCaster() }, { true, IsDivineCaster() } };
	size_t knownCount = 0, memorizedCount = 0, memoInfoCount = 0;
	for (spell_book& book : books) {
		if (!book.present)
			continue;
		memoInfoCount++;
		book.slots = (size_t)std::max(_TableInt(book.divine ? "MXSPLPRS"
			: (_ClassHas(fClass, "MAGE") ? "MXSPLWIZ" : "MXSPLBRD"), "1", "1", 0), 0);
		for (const spell_entry& entry : fSpells) {
			if (entry.divine != book.divine)
				continue;
			book.known.push_back(&entry);
			if (entry.memorized && book.memorized.size() < book.slots)
				book.memorized.push_back(&entry);
		}
		knownCount += book.known.size();
		memorizedCount += book.memorized.size();
	}

	const size_t knownOffset = kHeaderSize;
	const size_t memoInfoOffset = knownOffset + knownCount * 12;
	const size_t memorizedOffset = memoInfoOffset + memoInfoCount * 16;
	// The starting items (item struct: resref 8, expiration 2, quantities 3 x 2,
	// flags 4), then the slot table.
	const size_t kItemSize = 20;
	const size_t itemCount = fStartingStaff ? 1 : 0;
	const size_t tailOffset = memorizedOffset + memorizedCount * 12; // effects/items/slots
	const size_t slotOffset = tailOffset + itemCount * kItemSize;
	const size_t kTotal = slotOffset + kSlotTableSize;

	out.assign(kTotal, 0);

	std::memcpy(out.data(), "CRE V1.0", 8);
	_PutU32(out, 0x08, 0xffffffff);  // long name strref  (A6)
	_PutU32(out, 0x0c, 0xffffffff);  // short name strref (A6)
	_PutU32(out, 0x10, 0);           // flags
	_PutU32(out, 0x18, 0);           // experience
	_PutU32(out, 0x20, 0);           // permanent status

	// HP starts at 0 and THAC0/saves at conservative placeholders: the
	// engine fills in the real level-1 values from the class tables
	// (Actor::_Init() runs the level-up path because class level is 0).
	_PutU16(out, 0x24, 0);           // current HP
	_PutU16(out, 0x26, 0);           // maximum HP
	_PutU32(out, 0x28, AnimationID());   // player animation

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

	_PutU8(out, 0x241, (uint8)fRacialEnemy);
	_PutU32(out, 0x1c, 0);           // gold (the party's, see Gold())
	_PutU8(out, 0x44, fReputation);  // reputation
	_PutU16(out, 0x46, 10);          // AC natural
	_PutU16(out, 0x48, 10);          // AC effective
	_PutU8(out, 0x52, 20);           // THAC0 - engine recomputes from THAC0.2da
	_PutU8(out, 0x53, 1);            // # attacks
	for (int i = 0; i < 5; i++) _PutU8(out, 0x54 + i, 20); // saves - engine recomputes
	// BG1 keeps the weapon proficiencies in the header, from 0x6e (BG2 has them
	// as effects, which the character creation doesn't make yet).
	if (Core::Get()->Game() == game::GAME_BALDURSGATE) {
		for (int i = 0; i < kNumProficiencies; i++)
			_PutU8(out, 0x6e + i, (uint8)fProficiencies[i]);
	}
	{
		size_t skillCount = 0;
		const CharGenSkill* skills = CharGenData::Skills(skillCount);
		for (size_t i = 0; i < skillCount; i++)
			_PutU8(out, skills[i].creOffset, (uint8)fThiefSkills[i]);
	}

	// Class levels stay at 0 so Actor::_Init() runs the level-up path to
	// derive level-1 HP/THAC0/saves from the class tables. _CheckLevelUp
	// reads all three per-class slots; a multi-class name just needs the
	// unused trailing slots to also be 0 (which zeroed init already is).
	_PutU8(out, 0x234, 0);
	_PutU8(out, 0x235, 0);
	_PutU8(out, 0x236, 0);

	_PutU8(out, 0x237, (uint8)genderID);   // sex
	_PutU8(out, 0x238, (uint8)Ability(STR));
	_PutU8(out, 0x239, (uint8)(Ability(STR) == 18 && HasExceptionalStrength()
		? fStrengthExtra : 0));         // exceptional strength
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
	_PutU32(out, 0x2b8, (uint32)slotOffset);                                  // item slots
	_PutU32(out, 0x2bc, (uint32)tailOffset);     _PutU32(out, 0x2c0, (uint32)itemCount); // items
	_PutU32(out, 0x2c4, (uint32)tailOffset);     _PutU32(out, 0x2c8, 0);      // effects

	// Known spells: resref(8), level(2, 0-indexed on disk = level 1),
	// type(2, 0 = priest, 1 = wizard).
	// Spell memorization info: level(2), numMemorizable(2),
	// numMemorizableEffective(2), type(2), firstMemorizedIndex(4),
	// memorizedCount(4).
	// Memorized spells: resref(8), flags(4, bit0 = memorized/available).
	size_t knownAt = 0, infoAt = 0, memorizedAt = 0;
	for (const spell_book& book : books) {
		if (!book.present)
			continue;
		const uint16 spellType = book.divine ? 0 : 1;
		for (const spell_entry* entry : book.known) {
			const size_t o = knownOffset + knownAt++ * 12;
			_PutStr(out, o, entry->resref, 8);
			_PutU16(out, o + 8, 0); // level 1
			_PutU16(out, o + 10, spellType);
		}
		const size_t info = memoInfoOffset + infoAt++ * 16;
		_PutU16(out, info + 0, 0);   // level 1
		_PutU16(out, info + 2, (uint16)book.slots);
		_PutU16(out, info + 4, (uint16)book.slots);
		_PutU16(out, info + 6, spellType);
		_PutU32(out, info + 8, (uint32)memorizedAt);   // first memorized index
		_PutU32(out, info + 12, (uint32)book.memorized.size());
		for (const spell_entry* entry : book.memorized) {
			const size_t o = memorizedOffset + memorizedAt++ * 12;
			_PutStr(out, o, entry->resref, 8);
			_PutU32(out, o + 8, 1);
		}
	}

	for (size_t i = 0; i < kSlotCount; i++)
		_PutU16(out, slotOffset + i * 2, 0xffff); // empty slot
	if (fStartingStaff) {
		// A quarterstaff in the first weapon slot, selected; the last two words of
		// the table are the selected weapon and its ability.
		_PutStr(out, tailOffset, "STAF01", 8);
		_PutU16(out, tailOffset + 10, 1);	// quantity
		_PutU16(out, slotOffset + kSlotWeaponFirst * 2, 0);
		_PutU16(out, slotOffset + kNumItemSlots * 2, 0);
		_PutU16(out, slotOffset + (kNumItemSlots + 1) * 2, 0);
	}
	// The biography the record screen shows is a string of the character's sound
	// set (slot 74).
	if (fBiography != 0)
		_PutU32(out, 0xa4 + 74 * 4, fBiography);

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
		if (i == 0 && fAbilities[0] == 18 && HasExceptionalStrength())
			std::cout << "/" << fStrengthExtra;
		if (!fRace.empty() && !fClass.empty()) {
			std::cout << "  [" << AbilityMin(i) << ".." << AbilityMax(i) << "]";
		}
		std::cout << std::endl;
	}
	std::cout << "  Total: " << AbilityTotal() << std::endl;
	if (ProficienciesSpent() > 0) {
		size_t count = 0;
		const CharGenProficiency* profs = CharGenData::Proficiencies(count);
		std::cout << "  Proficiencies:";
		for (int i = 0; i < kNumProficiencies; i++) {
			if (fProficiencies[i] > 0)
				std::cout << " " << profs[i].name << "=" << fProficiencies[i];
		}
		std::cout << std::endl;
	}
	if (!fPortraitSmall.empty() || !fPortraitLarge.empty())
		std::cout << "  Portraits: " << fPortraitSmall << " / " << fPortraitLarge << std::endl;
	if (!fSpells.empty()) {
		std::cout << "  Spells:";
		for (const spell_entry& s : fSpells)
			std::cout << " " << s.resref << (s.memorized ? "*" : "");
		std::cout << std::endl;
	}
	if (ThiefSkillPointsSpent() > 0) {
		size_t count = 0;
		const CharGenSkill* skills = CharGenData::Skills(count);
		std::cout << "  Thief skills:";
		for (size_t i = 0; i < count; i++)
			std::cout << " " << skills[i].name << "=" << fThiefSkills[i];
		std::cout << std::endl;
	}

	std::vector<std::string> problems;
	if (IsComplete(problems)) {
		std::cout << "  -> ready" << std::endl;
	} else {
		for (const std::string& p : problems)
			std::cout << "  ! " << p << std::endl;
	}
}
