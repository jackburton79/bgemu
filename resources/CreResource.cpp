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
	Resource(name, RES_CRE),
	fItemSlotOffset(0),
	fItemsOffset(0)
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
CREResource::LongNameID() const
{
	uint32 id;
	fData->ReadAt(8, id);
	return id;
}


uint32
CREResource::ShortNameID() const
{
	uint32 id;
	fData->ReadAt(12, id);
	return id;
}


uint16
CREResource::AnimationID() const
{
	uint16 id;
	fData->ReadAt(0x28, id);
	return id;
}


uint32
CREResource::Kit() const
{
	uint32 kit;
	fData->ReadAt(0x244, kit);
	return kit;
}


const char*
CREResource::KitStr()
{
	return KitToStr(Kit());
}


uint8
CREResource::EnemyAlly() const
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
CREResource::General() const
{
	uint8 gen;
	fData->ReadAt(0x271, gen);
	return gen;
}


uint8
CREResource::Race() const
{
	uint8 rac;
	fData->ReadAt(0x272, rac);
	return rac;
}


uint8
CREResource::Class() const
{
	uint8 c;
	fData->ReadAt(0x273, c);
	return c;
}


uint8
CREResource::Specific() const
{
	uint8 spec;
	fData->ReadAt(0x0274, spec);
	return spec;
}


uint8
CREResource::Gender() const
{
	uint8 gend;
	fData->ReadAt(0x275, gend);
	return gend;
}


uint8
CREResource::Alignment() const
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
CREResource::Reputation() const
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
CREResource::Experience() const
{
	uint32 exp;
	fData->ReadAt(0x18, exp);
	return exp;
}


uint32
CREResource::PermanentStatus() const
{
	uint32 state;
	fData->ReadAt(0x20, state);
	return state;
}


uint16
CREResource::CurrentHitPoints() const
{
	uint32 hp;
	fData->ReadAt(0x24, hp);
	return hp;
}


uint32
CREResource::ExperienceValue() const
{
	uint32 exp;
	fData->ReadAt(0x14, exp);
	return exp;
}


uint32
CREResource::Gold() const
{
	uint32 gold;
	fData->ReadAt(0x1C, gold);
	return gold;
}


res_ref
CREResource::OverrideScriptName() const
{
	res_ref name;
	fData->ReadAt(0x248, name);
	return name;
}


res_ref
CREResource::ClassScriptName() const
{
	res_ref name;
	fData->ReadAt(0x250, name);
	return name;
}


res_ref
CREResource::RaceScriptName() const
{
	res_ref name;
	fData->ReadAt(0x258, name);
	return name;
}


res_ref
CREResource::GeneralScriptName() const
{
	res_ref name;
	fData->ReadAt(0x260, name);
	return name;
}


res_ref
CREResource::DefaultScriptName() const
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
CREResource::DialogFile() const
{
	res_ref dialogFile;
	fData->ReadAt(0x2cc, dialogFile.name, 8);
	return dialogFile;
}


uint16
CREResource::GlobalActorEnum() const
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
CREResource::LocalActorEnum() const
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

	//std::cout << "item at slot " << std::dec << i;
	//std::cout << " :" << std::dec << itemOffset << std::endl;

	// TODO: number 38 is a dword instead. Handle that case
	if ((int16)itemOffset == -1)
		throw std::runtime_error("Empty item");

	IE::item ieItem;
	_ReadItemNum(ieItem, itemOffset);

	return ieItem;
}


std::string
CREResource::DeathVariable() const
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
