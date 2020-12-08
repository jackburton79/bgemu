#include "CreResource.h"
#include "MemoryStream.h"

#include <stdio.h>
#include <stdlib.h>


/* static */
Resource*
CREResource::Create(const res_ref& name)
{
	return new CREResource(name);
}


CREResource::CREResource(const res_ref &name)
	:
	Resource(name, RES_CRE)
{
}


CREResource::~CREResource()
{
}


/* virtual */
bool
CREResource::Load(Archive *archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;

	if (!CheckSignature(CRE_SIGNATURE))
		return false;

	if (!CheckVersion(CRE_VERSION_1))
		return false;

	Init();

	return true;
}


bool
CREResource::Load(Stream* stream, uint32 position, uint32 size)
{
	delete fData;

	fData = new MemoryStream(size);
	stream->ReadAt(position, fData->Data(), size);

	Init();

	return true;
}


void
CREResource::Init()
{
	fData->ReadAt(0x02b8, fItemSlotOffset);
	fData->ReadAt(0x02bc, fItemsOffset);
}


uint32
CREResource::LongNameID()
{
	uint32 id;
	fData->ReadAt(8, id);
	return id;
}


uint32
CREResource::ShortNameID()
{
	uint32 id;
	fData->ReadAt(12, id);
	return id;
}


uint16
CREResource::AnimationID()
{
	uint16 id;
	fData->ReadAt(0x28, id);
	return id;
}


uint32
CREResource::Kit()
{
	uint32 kit;
	fData->ReadAt(0x244, kit);
	return kit;
}


const char *
CREResource::KitStr()
{
	return KitToStr(Kit());
}


uint8
CREResource::EnemyAlly()
{
	uint8 ea;
	fData->ReadAt(0x270, ea);
	return ea;
}


void
CREResource::SetEnemyAlly(uint8 ea)
{
	fData->WriteAt(0x270, &ea, sizeof(ea));
}


uint8
CREResource::General()
{
	uint8 gen;
	fData->ReadAt(0x271, gen);
	return gen;
}


uint8
CREResource::Race()
{
	uint8 rac;
	fData->ReadAt(0x272, rac);
	return rac;
}


uint8
CREResource::Class()
{
	uint8 c;
	fData->ReadAt(0x273, c);
	return c;
}


uint8
CREResource::Specific()
{
	uint8 spec;
	fData->ReadAt(0x0274, spec);
	return spec;
}


uint8
CREResource::Gender()
{
	uint8 gend;
	fData->ReadAt(0x275, gend);
	return gend;
}


uint8
CREResource::Alignment()
{
	uint8 align;
	fData->ReadAt(0x27b, align);
	return align;
}


CREColors
CREResource::Colors()
{
	CREColors colors;
	fData->ReadAt(0x2c, colors.metal);
	fData->ReadAt(0x2d, colors.minor);
	fData->ReadAt(0x2e, colors.major);
	fData->ReadAt(0x2f, colors.skin);
	fData->ReadAt(0x30, colors.leather);
	fData->ReadAt(0x31, colors.armor);
	fData->ReadAt(0x32, colors.hair);
	return colors;
}


sint8
CREResource::Reputation()
{
	sint8 rep;
	fData->ReadAt(0x44, rep);
	return rep;
}


void
CREResource::SetReputation(sint8 rep)
{
	fData->WriteAt(0x44, &rep, sizeof(rep));
}


uint32
CREResource::Experience()
{
	uint32 exp;
	fData->ReadAt(0x18, exp);
	return exp;
}


uint32
CREResource::PermanentStatus()
{
	uint32 state;
	fData->ReadAt(0x20, state);
	return state;
}


uint32
CREResource::ExperienceValue()
{
	uint32 exp;
	fData->ReadAt(0x14, exp);
	return exp;
}


uint32
CREResource::Gold()
{
	uint32 gold;
	fData->ReadAt(0x1C, gold);
	return gold;
}


res_ref
CREResource::OverrideScriptName()
{
	res_ref name;
	fData->ReadAt(0x248, name);
	return name;
}


res_ref
CREResource::ClassScriptName()
{
	res_ref name;
	fData->ReadAt(0x250, name);
	return name;
}


res_ref
CREResource::RaceScriptName()
{
	res_ref name;
	fData->ReadAt(0x258, name);
	return name;
}


res_ref
CREResource::GeneralScriptName()
{
	res_ref name;
	fData->ReadAt(0x260, name);
	return name;
}

res_ref
CREResource::DefaultScriptName()
{
	res_ref name;
	fData->ReadAt(0x268, name);
	return name;
}


void
CREResource::GetAttributes(BaseAttributes &attributes)
{
	fData->ReadAt(0x238, attributes);
}


res_ref
CREResource::DialogFile()
{
	res_ref dialogFile;
	fData->ReadAt(0x2cc, dialogFile.name, 8);
	return dialogFile;
}


uint16
CREResource::GlobalActorEnum()
{
	uint16 value;
	fData->ReadAt(0x27c, &value, sizeof(value));
	return value;
}


void
CREResource::SetGlobalActorEnum(uint16 value)
{
	fData->WriteAt(0x27c, &value, sizeof(value));
}


uint16
CREResource::LocalActorEnum()
{
	uint16 value;
	fData->ReadAt(0x27e, &value, sizeof(value));
	return value;
}


void
CREResource::SetLocalActorEnum(uint16 value)
{
	fData->WriteAt(0x27e, &value, sizeof(value));
}


IE::item
CREResource::ItemAtSlot(uint32 i)
{
	if (i >= kNumItemSlots)
		throw std::out_of_range("ItemAtSlot() out of range");

	uint16 itemOffset;
	fData->ReadAt(fItemSlotOffset + i * sizeof(itemOffset), itemOffset);

	std::cout << "item at slot " << std::dec << i;
	std::cout << " :" << std::dec << itemOffset << std::endl;

	// TODO: number 38 is a dword instead. Handle that case
	if (itemOffset == (int16)-1)
		throw std::runtime_error("Wrong item offset");

	IE::item ieItem;
	_ReadItemNum(ieItem, itemOffset);

	return ieItem;
}


std::string
CREResource::DeathVariable()
{
	char temp[32];
	fData->ReadAt(0x280, temp, 32);
	return temp;
}


void
CREResource::_ReadItemNum(IE::item& item, uint16 itemOffset)
{
	const off_t offset = fItemsOffset + itemOffset * sizeof(IE::item);
	fData->ReadAt(offset, item);
}

/*
void
BGCreature::GetCreatureInfo(TStream &file)
{
	char array[9];
	file.Read(array, 8);
	array[8] = '\0';
	cout << array << endl;
	if (!strcmp(array, "CRE V1.0")) {	
		file >> fLongNameOffset;
		printf("Long Name Offset: %ld\n", fLongNameOffset);
		file >> fShortNameOffset;
		printf("Short Name Offset: %ld\n", fShortNameOffset);
		
		file >> fFlags;
		printf("Exportable: %s\n", fFlags & CRE_IS_EXPORTABLE ? "YES" : "NO");	
		
		file.Seek(2, SEEK_CUR);
		
		file >> fExperienceValue;
		printf("Experience value: %ld\n", fExperienceValue);
		file >> fExperience;
		printf("Experience: %ld\n", fExperience);
		file >> fGold;
		printf("Gold: %ld\n", fGold);
		file >> fPermanentFlags;
		printf("Permanent flags: %lX\n", fPermanentFlags);
		file >> fCurrentHP;
		printf("Current HP: %d\n", fCurrentHP);
		file >> fMaxHP;
		printf("Max HP: %d\n", fMaxHP);
		file >> fAnimationID;
		printf("Animation ID: %d\n", fAnimationID);
		
		file.Seek(2, SEEK_CUR); // unknown field
		
		cout << "Color indexes: " << endl;
		file >> fColors.metal;
		cout << "\t" << "metal: " << (int)fColors.metal << endl;
		file >> fColors.minor;
		cout << "\t" << "minor: " << (int)fColors.minor << endl;
		file >> fColors.major;
		cout << "\t" << "major: " << (int)fColors.major << endl;
		file >> fColors.skin;
		cout << "\t" << "skin: " << (int)fColors.skin << endl;
		file >> fColors.leather;
		cout << "\t" << "leather: " << (int)fColors.leather << endl;
		file >> fColors.armor;
		cout << "\t" << "armor: " << (int)fColors.armor << endl;
		file >> fColors.hair;
		cout << "\t" << "hair: " << (int)fColors.hair << endl;
		
		file.Seek(1, SEEK_CUR); //?
		
		file.Read(fRSCSmallPortrait, 8);
		fRSCSmallPortrait[8] = '\0';
		printf("Small portrait resource name: %s\n", fRSCSmallPortrait);
		
		file.Read(fRSCLargePortrait, 8);
		fRSCLargePortrait[8] = '\0';
		printf("Large portrait resource name: %s\n", fRSCLargePortrait);
		
		file >> fReputation;
		printf("Reputation: %d\n", fReputation);
		
		file >> fHideInShadows;
		printf("Hide in shadows: %d\n", fHideInShadows);
		
		printf("Armor class: \n");
		file >> fArmorClass.natural;
		printf("\tNatural: %d\n", fArmorClass.natural);
		file >> fArmorClass.effective;
		printf("\tEffective: %d\n", fArmorClass.effective);
		file >> fArmorClass.crushing;
		printf("\tCrushing: %d\n", fArmorClass.crushing);
		file >> fArmorClass.missile;
		printf("\tMissile: %d\n", fArmorClass.missile);
		file >> fArmorClass.piercing;
		printf("\tPiercing: %d\n", fArmorClass.piercing);
		file >> fArmorClass.slashing;
		printf("\tSlashing: %d\n", fArmorClass.slashing);
		
		file >> fTHAC0;
		printf("THAC0: %d\n", fTHAC0);
		
		file >> fNumberOfAttacks;
		printf("Number of Attacks: %d\n", fNumberOfAttacks);
		
		printf("Save Versus: \n");
		file >> fSaveVersus.death;
		file >> fSaveVersus.wands;
		file >> fSaveVersus.poly;
		file >> fSaveVersus.breath;
		file >> fSaveVersus.spell;
		 
		printf("\tdeath: %d\n", fSaveVersus.death);
		printf("\twands: %d\n", fSaveVersus.wands);
		printf("\tpoly: %d\n", fSaveVersus.poly);
		printf("\tbreath: %d\n", fSaveVersus.breath);
		printf("\tspell: %d\n", fSaveVersus.spell);
		
		printf("Resistances: \n");
		file >> fResistances.fire;
		file >> fResistances.cold;
		file >> fResistances.electricity;
		file >> fResistances.acid;
		file >> fResistances.magic;
		file >> fResistances.magic_fire;
		file >> fResistances.magic_cold;
		file >> fResistances.slashing;
		file >> fResistances.crushing;
		file >> fResistances.piercing;
		file >> fResistances.missile;
		printf("\tfire: %d\n", fResistances.fire);
		printf("\tcold: %d\n", fResistances.cold);
		printf("\telectricity: %d\n", fResistances.electricity);
		printf("\tacid: %d\n", fResistances.acid);
		printf("\tmagic: %d\n", fResistances.magic);
		printf("\tmagic_fire: %d\n", fResistances.magic_fire);
		printf("\tmagic_cold: %d\n", fResistances.magic_cold);
		printf("\tslashing: %d\n", fResistances.slashing);
		printf("\tcrushing: %d\n", fResistances.crushing);
		printf("\tpiercing: %d\n", fResistances.piercing);
		printf("\tmissile: %d\n", fResistances.missile);
		
		file >> fDetectIllusion;
		printf("Detect illusion: %d\n", fDetectIllusion);
		
		file >> fSetTraps;
		cout << "Set traps: " << (int)fSetTraps << endl;
		
		file >> fLore;
		cout << "Lore: " << (int)fLore << endl;
		
		file >> fLockPicking;
		cout << "Lock picking: " << (int)fLockPicking << endl;
		
		file >> fStealth;
		cout << "Stealth: " << (int)fStealth << endl;
		
		file >> fDisarmTraps;
		cout << "Find/Disarm traps: " << (int)fDisarmTraps << endl;
		
		file >> fPickPockets;
		cout << "Pick-pockets: " << (int)fPickPockets << endl;
		
		file >> fFatigue;
		cout << "Fatigue: " << (int)fFatigue << endl;
		
		file >> fIntoxification;
		cout << "Intoxification: " << (int)fIntoxification << endl;
		
		file >> fLuck;
		cout << "Luck: " << (int)fLuck << endl;
		
		file.Seek(0x238 - 0x6e, SEEK_CUR); // TODO: Later
		
		printf("Attributes:\n");
		file >> fAttributes.strength;
		file >> fAttributes.strength_bonus;
		file >> fAttributes.intelligence;
		file >> fAttributes.wisdom;
		file >> fAttributes.dexterity;
		file >> fAttributes.constitution;
		file >> fAttributes.charisma;
		printf("\tstrength: %d", fAttributes.strength);
		if (fAttributes.strength_bonus != 0)
			printf("/%d", fAttributes.strength_bonus);
		printf("\n");
		printf("\tintellicence: %d\n", fAttributes.intelligence);
		printf("\twisdom: %d\n", fAttributes.wisdom);
		printf("\tdexterity: %d\n", fAttributes.dexterity);
		printf("\tconstitution: %d\n", fAttributes.constitution);
		printf("\tcharisma: %d\n", fAttributes.charisma);
		
		file >> fMorale;
		cout << "Morale: " << (int)fMorale << endl;
		
		file >> fMoraleBreak;
		cout << "Morale break: " << (int)fMoraleBreak << endl;
		
		file >> fRacialEnemy;
		cout << "Racial enemy: " << (int)fRacialEnemy << endl;
		
		file >> fMoraleRecoveryTime;
		cout << "Morale recovery time: " << (int)fMoraleRecoveryTime << endl;
 		
 		file.Seek(1, SEEK_CUR);
 		
 		file >> fKit;
 		
 		cout << "Kit: " << KitToStr(fKit) << "(0x" << hex << fKit << ")" << endl;
 		
 		file.Seek(0x2cc - 0x248, SEEK_CUR);
		
		file.Read(fDialogFile, 8);
		fDialogFile[8] = '\0';
		
		printf("Dialog file: %s\n", fDialogFile);
		
	}
}
*/

const char *
KitToStr(uint32 kit)
{
	switch (kit) {
		case KIT_BARBARIAN:
			return "Barbarian";
		case KIT_TRUECLASS:
			return "TrueClass";
		case KIT_BERSERKER:
			return "Berserker";
		case KIT_WIZARDSLAYER:
			return "WizardSlayer";
		case KIT_KENSAI:
			return "Kensai";
		case KIT_CAVALIER:
			return "Cavalier";
		case KIT_INQUISITOR:
			return "Inquisitor";
		case KIT_UNDEADHUNTER:
			return "Undead Hunter";
		case KIT_ARCHER:
			return "Archer";
		case KIT_STALKER:
			return "Stalker";
		case KIT_BEASTMASTER:
			return "Beast Master";
		case KIT_ASSASSIN:
			return "Assassin";
		case KIT_BOUNTYHUNTER:
			return "Bounty Hunter";
		case KIT_SWASHBUCKLER:
			return "SwashBuckler";
		case KIT_BLADE:
			return "Blade";
		case KIT_JESTER:
			return "Jester";
		case KIT_SKALD:
			return "Skald";
		case KIT_TOTEMIC:
			return "Totemic";
		case KIT_SHAPESHIFTER:
			return "Shape Shifter";
		case KIT_AVENGER:
			return "Avenger";
		case KIT_GODTALOS:
			return "Druid of Talos";
		case KIT_GODHELM:
			return "Druid of Helm";
		case KIT_GODLATHANDER:
			return "Druid of Lathander";
		case ABJURER:
			return "Abjurer";
		case CONJURER:
			return "Conjurer";
		case DIVINER:
			return "Diviner";
		case ENCHANTER:
			return "Enchanter";
		case ILLUSIONIST:
			return "Illusionist";
		case INVOKER:
			return "Invoker";
		case NECROMANCER:
			return "Necromancer";
		case TRANSMUTER:
			return "Transmuter";
		case NONE:
		default:
			return "None";
	}
}
