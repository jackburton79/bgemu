/*
 * CharacterBuilder.h
 *
 * Headless character-creation engine (roadmap Fase 47 / A). Holds the
 * player's choices - gender, race, class, kit, alignment, the six
 * ability scores - and validates them against the real AD&D 2e rule
 * tables the original game uses (ABCLASRQ, ABRACERQ, ALIGNMNT). No GUI;
 * driven by the Char-* console commands for now. A2 turns a completed
 * builder into an actual CRE.
 */

#ifndef CHARACTER_BUILDER_H_
#define CHARACTER_BUILDER_H_

#include "SupportDefs.h"

#include <string>
#include <vector>

class CharacterBuilder {
public:
	enum {
		STR = 0, DEX, CON, INT, WIS, CHR, kNumAbilities
	};

	CharacterBuilder();

	void Reset();

	// Each returns false (leaving the field unchanged) if the value
	// isn't a known one for that field, or - for alignment - if the
	// currently chosen class forbids it.
	bool SetGender(const std::string& gender);     // GENDER.IDS name
	bool SetRace(const std::string& race);         // ABRACERQ / RACE.IDS row
	bool SetClass(const std::string& className);   // CLASS.IDS / ABCLASRQ row
	bool SetKit(const std::string& kitName);       // KIT.IDS name, "" = none
	bool SetAlignment(const std::string& align);   // ALIGNMEN.IDS name or 2-letter code

	// Identity / appearance (all optional - sensible defaults otherwise).
	void SetName(const std::string& name);
	void SetPortraits(const std::string& small, const std::string& large);
	// colour: 0..255 palette index (MPAL256), or < 0 to keep the default.
	// which: one of metal/minor/major/skin/leather/armor/hair.
	bool SetColor(const std::string& which, int index);

	const std::string& Name() const { return fName; }
	const std::string& PortraitSmall() const { return fPortraitSmall; }
	const std::string& PortraitLarge() const { return fPortraitLarge; }

	// The starting spellbook (level 1): a spell of a character spec is known and,
	// as far as the slots go, memorized; the creation window learns and memorizes
	// a mage's own choices, and a cleric's spells all. false if the resref isn't a
	// real SPL. A book only gets written for a class that has that kind of magic.
	bool AddSpell(const std::string& resref);
	bool LearnSpell(const std::string& resref, bool memorized);
	void ClearSpells();
	struct spell_choice {
		std::string resref;
		bool learnable;
	};
	std::vector<spell_choice> MageSpellChoices() const;
	int MageSpellsToLearn() const;
	int MageSpellsToMemorize() const;
	void LearnDivineSpells();
	bool IsSpellKnown(const std::string& resref) const;
	// The resrefs of the mage's (or the priest's) starting spells.
	std::vector<std::string> KnownSpells(bool divine) const;
	int MemorizedSpellCount(bool divine) const;

	// Thief skills (pick pockets, open locks, find traps, stealth: the order of
	// CharGenData::Skills()): the points given to each, as the CRE keeps them - the
	// racial (SKILLRAC) and Dexterity (SKILLDEX) adjustments are not in them, see
	// ThiefSkillBonus(). By name ("pickpockets", "openlocks", "findtraps",
	// "stealth") for a character spec.
	static const int kNumThiefSkills = 4;
	bool SetThiefSkill(const std::string& which, int value);
	bool SetThiefSkill(int skill, int points);
	int ThiefSkill(int skill) const;
	// The points the class gives at the start (skills.2da's FIRST_LEVEL for the
	// thief classes, 0 for the others).
	int ThiefSkillPoints() const;
	int ThiefSkillPointsSpent() const;
	// What the race and the Dexterity add to (or take from) the skill.
	int ThiefSkillBonus(int skill) const;
	// What a new character starts the game with (reputation, gold, quarterstaff,
	// biography) - set by the creation's last step; a character spec has its own.
	void ApplyStartingKit();
	// The gold to give the party (0 unless ApplyStartingKit() was called).
	int Gold() const { return fGold; }
	int Reputation() const { return fReputation; }
	bool HasStartingStaff() const { return fStartingStaff; }

	// A ranger's racial enemy (RACE.IDS value, 0 none): rangers alone, alone or in
	// a multiclass, have one.
	bool HasRacialEnemy() const;
	int RacialEnemy() const { return fRacialEnemy; }
	void SetRacialEnemy(int race) { fRacialEnemy = race; }

	// Whether the chosen class casts arcane / divine (memorized) spells.
	bool IsArcaneCaster() const;
	bool IsDivineCaster() const;

	// Rolls every ability (3d5 + 3 + the racial adjustment, as BG1's character
	// creation does), held within [AbilityMin, AbilityMax] - the class
	// minimums are met by raising a low roll - and the 18/xx roll of a
	// warrior. Returns the sum, or 0 without a race and a class.
	int RollAbilities();

	// Sets one ability to an explicit value; false if it's outside
	// [AbilityMin, AbilityMax] for the current race/class.
	bool SetAbility(int ability, int value);

	int Ability(int ability) const;
	int AbilityTotal() const;
	// Lowest / highest legal value for this ability given race + class; both
	// are final scores, with the racial adjustment (ABRACEAD) in.
	int AbilityMin(int ability) const;
	int AbilityMax(int ability) const;
	// What the race adds to (or takes from) the ability: an elf's Dexterity +1.
	int AbilityAdjustment(int ability) const;
	// Whether the class may have an exceptional (18/xx) strength: a fighter,
	// ranger or paladin, alone or in a multiclass.
	bool HasExceptionalStrength() const;
	// The xx of an 18/xx strength (1..100); it only counts at strength 18.
	int StrengthExtra() const { return fStrengthExtra; }
	void SetStrengthExtra(int value);

	const std::string& Gender() const { return fGender; }
	const std::string& Race() const { return fRace; }
	const std::string& Class() const { return fClass; }
	const std::string& Kit() const { return fKit; }
	const std::string& Alignment() const { return fAlignment; }
	// Weapon proficiencies (BG1: the eight of CharGenData::Proficiencies(), 0..5
	// stars each). The points to give at the start come from PROFS.2DA, the weapons
	// the class may take from CLASWEAP.2DA and the stars one weapon takes at the
	// start from PROFSMAX.2DA.
	static const int kNumProficiencies = 8;
	int ProficiencyPoints() const;
	// The stars this weapon can have now (0: the class can't use it).
	int ProficiencyLimit(int proficiency) const;
	int Proficiency(int proficiency) const;
	// false if outside [0, ProficiencyLimit()].
	bool SetProficiency(int proficiency, int stars);
	// The stars already given.
	int ProficienciesSpent() const;

	// Whether the chosen class (if any) allows this alignment (ALIGNMEN.IDS name).
	bool IsAlignmentAllowed(const std::string& alignment) const;
	// ALIGNMEN.IDS numeric value (0x11..0x33), 0 if unset.
	uint8 AlignmentValue() const { return fAlignmentValue; }

	// Fills `problems` with every reason the character isn't ready yet;
	// returns true (and leaves `problems` empty) when it is.
	bool IsComplete(std::vector<std::string>& problems) const;

	// Serializes the current choices into a minimal but valid CRE v1
	// blob (empty spell/effect/item sections, level 1). false if the
	// character isn't complete or a required IDS lookup fails.
	// HP/THAC0/saves/proficiencies/skills/spells are placeholders here -
	// later sub-phases fill them from the class tables.
	bool BuildCREData(std::vector<uint8>& out) const;

	void Print() const;

	static const char* AbilityName(int ability);

private:
	int _ClassMinimum(int ability) const;   // ABCLASRQ, 0 if none
	int _RacialMinimum(int ability) const;  // ABRACERQ
	int _RacialMaximum(int ability) const;  // ABRACERQ
	bool _AlignmentAllowedForClass(const std::string& className,
									uint8 alignmentValue) const;

	std::string fGender;
	std::string fRace;
	std::string fClass;
	std::string fKit;
	std::string fAlignment;
	uint8 fAlignmentValue;
	int fAbilities[kNumAbilities];
	int fStrengthExtra = 0;
	int fProficiencies[kNumProficiencies] = {};

	std::string fName;
	std::string fPortraitSmall;
	std::string fPortraitLarge;
	int fColors[7];    // metal, minor, major, skin, leather, armor, hair; -1 = default
	struct spell_entry {
		std::string resref;
		bool divine;
		bool memorized;
	};
	std::vector<spell_entry> fSpells;   // the level-1 starting spellbook
	int fThiefSkills[kNumThiefSkills] = {};
	int fRacialEnemy = 0;
	uint8 fReputation = 10;
	int fGold = 0;
	uint32 fBiography = 0;
	bool fStartingStaff = false;
};

#endif // CHARACTER_BUILDER_H_
