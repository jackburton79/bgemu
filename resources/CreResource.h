#ifndef __BGCREATURE_H
#define __BGCREATURE_H

#include "Resource.h"
#include "SupportDefs.h"

#define CRE_SIGNATURE "CRE "
#define CRE_VERSION_1 "V1.0"

const uint32 kNumItemSlots = 40;

enum CreatureFlagBits {
	CRE_IS_EXPORTABLE = 0x800
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
	uint16 CurrentHitPoints() const;
	uint32 Gold() const;
	
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

	IE::item ItemAtSlot(uint32 i);

	res_ref DialogFile() const;
	std::string DeathVariable() const;

private:
	virtual ~CREResource();
	
	void _ReadItemNum(IE::item& ieItem, uint16 offset);
	
	uint32 fItemSlotOffset;
	uint32 fItemsOffset;
};

const char *KitToStr(uint32 kit);

#endif
