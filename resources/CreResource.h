#ifndef __BGCREATURE_H
#define __BGCREATURE_H

#include "Resource.h"
#include "SupportDefs.h"

#include <vector>

#define CRE_SIGNATURE "CRE "
#define CRE_VERSION_1 "V1.0"

const uint32 kNumItemSlots = 40;

enum CreatureFlagBits {
	CRE_IS_EXPORTABLE = 0x800
};


// PermanentStatus() bits - see STATE.IDS.
enum CreatureStateBits {
	STATE_SLEEPING = 0x00000001,
	STATE_BERSERK = 0x00000002,
	STATE_PANIC = 0x00000004,
	STATE_STUNNED = 0x00000008,
	STATE_INVISIBLE = 0x00000010,
	STATE_HELPLESS = 0x00000020,	// used for Hold/Paralyze
	STATE_DEAD = 0x00000800,
	STATE_SILENCED = 0x00001000,
	STATE_POISONED = 0x00004000,
	STATE_HASTED = 0x00008000,
	STATE_SLOWED = 0x00010000,
	STATE_BLIND = 0x00040000,
	STATE_CONFUSED = 0x80000000
};


enum Kits {
	NONE				= 0x00000000,
	KIT_BARBARIAN		= 0x00004000,
	KIT_TRUECLASS		= 0x40000000,
	KIT_BERSERKER		= 0x40010000,
	KIT_WIZARDSLAYER	= 0x40020000,
	KIT_KENSAI			= 0x40030000,
	KIT_CAVALIER		= 0x40040000,
	KIT_INQUISITOR		= 0x40050000,
	KIT_UNDEADHUNTER	= 0x40060000,
	KIT_ARCHER			= 0x40070000,
	KIT_STALKER			= 0x40080000,
	KIT_BEASTMASTER		= 0x40090000,
	KIT_ASSASSIN		= 0x400A0000,
	KIT_BOUNTYHUNTER	= 0x400B0000,
	KIT_SWASHBUCKLER	= 0x400C0000,
	KIT_BLADE			= 0x400D0000,
	KIT_JESTER			= 0x400E0000,
	KIT_SKALD			= 0x400F0000,
	KIT_TOTEMIC			= 0x40100000,
	KIT_SHAPESHIFTER	= 0x40110000,
	KIT_AVENGER			= 0x40120000,
	KIT_GODTALOS		= 0x40130000,
	KIT_GODHELM			= 0x40140000,
	KIT_GODLATHANDER	= 0x40150000,
	ABJURER				= 0x00400000,
	CONJURER			= 0x00800000,
	DIVINER				= 0x01000000,
	ENCHANTER			= 0x02000000,
	ILLUSIONIST			= 0x04000000,
	INVOKER				= 0x08000000,
	NECROMANCER			= 0x10000000,
	TRANSMUTER			= 0x20000000
};


struct CREColors {
	uint8 metal;
	uint8 minor;
	uint8 major;
	uint8 skin;
	uint8 leather;
	uint8 armor;
	uint8 hair;
};


struct ArmorClass {
	int16 natural;
	int16 effective;
	int16 crushing;
	int16 missile;
	int16 piercing;
	int16 slashing;
};


struct SaveVersus {
	uint8 death;
	uint8 wands;
	uint8 poly;
	uint8 breath;
	uint8 spell;
};


struct Resistances {
	uint8 fire;
	uint8 cold;
	uint8 electricity;
	uint8 acid;
	uint8 magic;
	uint8 magic_fire;
	uint8 magic_cold;
	uint8 slashing;
	uint8 crushing;
	uint8 piercing;
	uint8 missile;
};


// CRE V1.0 Known Spells entry - see IESDP cre_v1.
struct cre_known_spell {
	res_ref spell;
	uint16 level;	// actual spell level (already +1'd from the on-disk value)
	uint16 type;	// 0=Priest, 1=Wizard, 2=Innate
};


// CRE V1.0 Memorized Spells Table entry - see IESDP cre_v1.
struct cre_memorized_spell {
	res_ref spell;
	uint32 flags;	// bit0: memorized: bit1: disabled
};


// CRE V1.0 Spell Memorization Info entry - see IESDP cre_v1. How many
// spells of a given level/type the creature can memorize, and where its
// currently memorized spells for that level/type live in the memorized
// spells table.
struct cre_spell_memorization_info {
	uint16 level;	// actual spell level (already +1'd from the on-disk value)
	uint16 numMemorizable;
	uint16 numMemorizableEffective;
	uint16 type;	// 0=Priest, 1=Wizard, 2=Innate
	uint32 firstMemorizedIndex;
	uint32 memorizedCount;
};


struct BaseAttributes {
	int8 strength;
	int8 strength_bonus;
	int8 intelligence;
	int8 wisdom;
	int8 dexterity;
	int8 constitution;
	int8 charisma;
};


class Archive;
class CREResource : public Resource {
public:
	CREResource(const res_ref& name);

	static Resource* Create(const res_ref& name);

	virtual bool Load(Archive *archive, uint32 key);
	bool Load(Stream* stream, uint32 position, uint32 size);
	
	void Init();
	
	uint32 LongNameID() const;
	uint32 ShortNameID() const;

	uint16 AnimationID() const;
	uint32 Kit() const;
	const char *KitStr();

	uint8 EnemyAlly() const;
	void SetEnemyAlly(uint8 ea);

	uint8 General() const;
	uint8 Race() const;
	uint8 Class() const;
	uint8 Specific() const;
	uint8 Gender() const;
	uint8 Alignment() const;

	CREColors Colors();

	sint8 Reputation() const;
	void SetReputation(sint8 rep);

	uint32 Experience() const;
	uint32 ExperienceValue() const;
	uint32 PermanentStatus() const;
	void SetPermanentStatus(uint32 status);
	uint16 CurrentHitPoints() const;
	void SetCurrentHitPoints(uint16 hp);
	uint16 MaxHitPoints() const;
	uint32 Gold() const;

	ArmorClass AC() const;
	uint8 THAC0() const;
	uint8 NumberOfAttacks() const;
	SaveVersus Saves() const;
	Resistances DamageResistances() const;

	// Adds delta to the "effective" AC and to any per-type field selected
	// by typeMask (bit0=Crushing,bit1=Missile,bit2=Piercing,bit3=Slashing
	// per IESDP opcode #0; 0 = all four types). Symmetric: applying
	// -delta with the same mask exactly undoes it - used by SpellEffect's
	// AC-bonus opcode to apply/remove a buff without needing to remember
	// the pre-buff value separately.
	void ModifyAC(int16 delta, uint8 typeMask);
	// Adds delta to THAC0 (lower is better) - same symmetric apply/undo
	// pattern as ModifyAC().
	void ModifyTHAC0(int8 delta);

	// Highest attained level across this creature's (up to 3, for
	// dual/multi-class) classes.
	uint8 Level() const;
	uint8 Morale() const;
	
	void GetAttributes(BaseAttributes &attributes);
	
	res_ref OverrideScriptName() const;
	res_ref ClassScriptName() const;
	res_ref RaceScriptName() const;
	res_ref GeneralScriptName() const;
	res_ref DefaultScriptName() const;

	uint16 GlobalActorEnum() const;
	void SetGlobalActorEnum(uint16 enumValue);
	uint16 LocalActorEnum() const;
	void SetLocalActorEnum(uint16 enumValue);

	bool GetItemAtSlot(uint32 i, IE::item& item) const;
	// -1 if the slot is empty, else the raw index into the Items table.
	int32 ItemsIndexAtSlot(uint32 slot) const;
	// Writes an index into the Items table (or -1 to clear) at the given
	// slot - see SLOTS.IDS for what each slot number means (e.g. 35 is
	// the currently-wielded weapon, 15-34 are general inventory).
	void SetItemAtSlot(uint32 slot, int32 itemsIndex);
	// Moves whatever Items-table index `fromSlot` holds into `toSlot`
	// (clearing `fromSlot`), overwriting anything `toSlot` already held.
	void MoveItemBetweenSlots(uint32 fromSlot, uint32 toSlot);
	// -1 if no slot in [firstSlot, lastSlot] is empty.
	int32 FindFreeSlot(uint32 firstSlot, uint32 lastSlot) const;
	// -1 if the creature isn't carrying an item with this resref.
	int32 FindItemSlot(const res_ref& itemName) const;

	// -1 if the Items table has no free (empty-resref) entry to reuse -
	// this engine doesn't grow a CRE's on-disk Items table, so giving an
	// item to a creature only works while it has spare capacity (which
	// most placed creatures do, since the original data usually carries
	// a few extra blank entries).
	int32 FindFreeItemsEntry() const;
	void SetItemAtItemsIndex(uint16 index, const IE::item& item);

	res_ref DialogFile() const;
	std::string DeathVariable() const;

	std::vector<cre_known_spell> KnownSpells() const;
	std::vector<cre_memorized_spell> MemorizedSpells() const;
	std::vector<cre_spell_memorization_info> SpellMemorizationInfo() const;

private:
	virtual ~CREResource();

	void _ReadItemNum(IE::item& ieItem, uint16 offset) const;

	uint32 fItemSlotOffset;
	uint32 fItemsOffset;
	uint32 fItemsCount;

	uint32 fKnownSpellsOffset;
	uint32 fKnownSpellsCount;
	uint32 fSpellMemoInfoOffset;
	uint32 fSpellMemoInfoCount;
	uint32 fMemorizedSpellsOffset;
	uint32 fMemorizedSpellsCount;
};

const char *KitToStr(uint32 kit);

#endif
