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

	virtual ~CREResource();
	
	virtual bool Load(Archive *archive, uint32 key);
	bool Load(Stream* stream, uint32 position, uint32 size);

	uint32 LongNameID();
	uint32 ShortNameID();

	const uint16 AnimationID();
	const uint32 Kit();
	const char *KitStr();

	uint8 EnemyAlly();
	void SetEnemyAlly(uint8 ea);

	uint8 General();
	uint8 Race();
	uint8 Class();
	uint8 Specific();
	uint8 Gender();
	uint8 Alignment();

	CREColors Colors();

	sint8 Reputation();
	void SetReputation(sint8 rep);

	uint32 Experience();
	uint32 ExperienceValue();
	uint32 PermanentStatus();
	uint32 Gold();
	
	void GetAttributes(BaseAttributes &attributes);
	
	res_ref OverrideScriptName();
	res_ref ClassScriptName();
	res_ref RaceScriptName();
	res_ref GeneralScriptName();
	res_ref DefaultScriptName();

	uint16 GlobalActorEnum();
	void SetGlobalActorEnum(uint16 enumValue);
	uint16 LocalActorEnum();
	void SetLocalActorEnum(uint16 enumValue);

	IE::item* ItemAtSlot(uint32 i);

	const char *DialogFile();
	const char *DeathVariable();

private:
	uint32 fItemSlotOffset;
	uint32 fItemsOffset;
/*protected:
	uint32 fLongNameID;
	uint32 fShortNameID;
	
	uint16 fFlags;
	//uint8 fFlags2;
	
	int32 fExperienceValue;
	int32 fExperience;
	int32 fGold;
	uint32 fPermanentFlags;
	int16 fCurrentHP;
	int16 fMaxHP;
	int16 fAnimationID;
	
	CreatureColors fColors;
	
	char fRSCSmallPortrait[9];
	char fRSCLargePortrait[9];
	
	uint8 fReputation;
	uint8 fHideInShadows;
	
	ArmorClass fArmorClass;
	
	uint8 fTHAC0;
	
	uint8 fNumberOfAttacks;
	
	SaveVersus fSaveVersus;
	Resistances fResistances;
	
	uint8 fDetectIllusion;
	uint8 fSetTraps;
	uint8 fLore;
	uint8 fLockPicking;
	uint8 fStealth;
	uint8 fDisarmTraps;
	uint8 fPickPockets;
	uint8 fFatigue;
	uint8 fIntoxification;
	uint8 fLuck;
	
	BaseAttributes fAttributes;
	
	uint8 fMorale;
	uint8 fMoraleBreak;
	uint8 fRacialEnemy;
	uint8 fMoraleRecoveryTime;
	
	uint32 fKit;
	
	char fDialogFile[9];*/
};

const char *KitToStr(uint32 kit);

#endif
