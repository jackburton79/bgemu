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

	// Adds a level-1 arcane spell to the starting spellbook (ignored for
	// a non-arcane class). false if the resref isn't a real SPL.
	bool AddSpell(const std::string& resref);

	// Starting thief skill points (0..255) - only the two the engine
	// currently reads (Door::UpdateSearchMapBlocking()/Actions.cpp Fase
	// 8): "openlocks" and "findtraps". No point-pool/DEX-bonus
	// enforcement yet (same declared simplification as the ability
	// roller's manual SetAbility()) - the caller is trusted not to type
	// in an absurd value.
	bool SetThiefSkill(const std::string& which, int value);
	// Whether the chosen class casts arcane / divine (memorized) spells.
	bool IsArcaneCaster() const;
	bool IsDivineCaster() const;

	// Rolls 3d6 for every ability, clamps each into its racial range,
	// and rerolls the whole set until every class minimum is met (the
	// original game's roller never hands you an unusable set). Returns
	// the sum, or 0 if no valid set turned up (impossible combo).
	int RollAbilities();

	// Sets one ability to an explicit value; false if it's outside
	// [AbilityMin, AbilityMax] for the current race/class.
	bool SetAbility(int ability, int value);

	int Ability(int ability) const;
	int AbilityTotal() const;
	// Lowest / highest legal value for this ability given race + class.
	int AbilityMin(int ability) const;
	int AbilityMax(int ability) const;

	const std::string& Gender() const { return fGender; }
	const std::string& Race() const { return fRace; }
	const std::string& Class() const { return fClass; }
	const std::string& Kit() const { return fKit; }
	const std::string& Alignment() const { return fAlignment; }
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

	std::string fName;
	std::string fPortraitSmall;
	std::string fPortraitLarge;
	int fColors[7];    // metal, minor, major, skin, leather, armor, hair; -1 = default
	std::vector<std::string> fSpells;   // level-1 arcane starting spellbook
	int fOpenLocksSkill = 0;
	int fFindTrapsSkill = 0;
};

#endif // CHARACTER_BUILDER_H_
