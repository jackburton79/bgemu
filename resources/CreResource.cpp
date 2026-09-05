#include "CreResource.h"
#include "MemoryStream.h"

#include <algorithm>
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
	fItemsOffset(0),
	fItemsCount(0),
	fKnownSpellsOffset(0),
	fKnownSpellsCount(0),
	fSpellMemoInfoOffset(0),
	fSpellMemoInfoCount(0),
	fMemorizedSpellsOffset(0),
	fMemorizedSpellsCount(0)
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
	fData->ReadAt(0x02a0, fKnownSpellsOffset);
	fData->ReadAt(0x02a4, fKnownSpellsCount);
	fData->ReadAt(0x02a8, fSpellMemoInfoOffset);
	fData->ReadAt(0x02ac, fSpellMemoInfoCount);
	fData->ReadAt(0x02b0, fMemorizedSpellsOffset);
	fData->ReadAt(0x02b4, fMemorizedSpellsCount);
	fData->ReadAt(0x02b8, fItemSlotOffset);
	fData->ReadAt(0x02bc, fItemsOffset);
	fData->ReadAt(0x02c0, fItemsCount);
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


void
CREResource::SetPermanentStatus(uint32 status)
{
	fData->WriteAt(0x20, &status, sizeof(status));
}


uint16
CREResource::CurrentHitPoints() const
{
	uint16 hp;
	fData->ReadAt(0x24, hp);
	return hp;
}


void
CREResource::SetCurrentHitPoints(uint16 hp)
{
	fData->WriteAt(0x24, &hp, sizeof(hp));
}


uint16
CREResource::MaxHitPoints() const
{
	uint16 hp;
	fData->ReadAt(0x26, hp);
	return hp;
}


uint8
CREResource::Level() const
{
	uint8 level1, level2, level3;
	fData->ReadAt(0x234, level1);
	fData->ReadAt(0x235, level2);
	fData->ReadAt(0x236, level3);
	return std::max(level1, std::max(level2, level3));
}


uint8
CREResource::Morale() const
{
	uint8 morale;
	fData->ReadAt(0x23f, morale);
	return morale;
}


// CRE v1: 0x46 Armor Class (Natural), 0x48 (Effective), 0x4a-0x50 per
// damage-type modifiers (Crushing/Missile/Piercing/Slashing).
ArmorClass
CREResource::AC() const
{
	ArmorClass ac;
	fData->ReadAt(0x46, ac.natural);
	fData->ReadAt(0x48, ac.effective);
	fData->ReadAt(0x4a, ac.crushing);
	fData->ReadAt(0x4c, ac.missile);
	fData->ReadAt(0x4e, ac.piercing);
	fData->ReadAt(0x50, ac.slashing);
	return ac;
}


uint8
CREResource::THAC0() const
{
	uint8 thac0;
	fData->ReadAt(0x52, thac0);
	return thac0;
}


void
CREResource::ModifyAC(int16 delta, uint8 typeMask)
{
	int16 effective;
	fData->ReadAt(0x48, effective);
	effective += delta;
	fData->WriteAt(0x48, &effective, sizeof(effective));

	static const struct { uint8 bit; uint32 offset; } kTypes[] = {
		{ 1, 0x4a }, // Crushing
		{ 2, 0x4c }, // Missile
		{ 4, 0x4e }, // Piercing
		{ 8, 0x50 }, // Slashing
	};
	for (const auto& type : kTypes) {
		if (typeMask != 0 && (typeMask & type.bit) == 0)
			continue;
		int16 value;
		fData->ReadAt(type.offset, value);
		value += delta;
		fData->WriteAt(type.offset, &value, sizeof(value));
	}
}


void
CREResource::ModifyTHAC0(int8 delta)
{
	int8 thac0;
	fData->ReadAt(0x52, thac0);
	thac0 += delta;
	fData->WriteAt(0x52, &thac0, sizeof(thac0));
}


// CRE v1: 0x59-0x63, one byte each, same field order as struct Resistances.
Resistances
CREResource::DamageResistances() const
{
	Resistances resist;
	fData->ReadAt(0x59, resist);
	return resist;
}


uint8
CREResource::NumberOfAttacks() const
{
	uint8 attacks;
	fData->ReadAt(0x53, attacks);
	return attacks;
}


// CRE v1: 0x54 Save vs Death, 0x55 Wands, 0x56 Polymorph, 0x57 Breath,
// 0x58 Spell.
SaveVersus
CREResource::Saves() const
{
	SaveVersus saves;
	fData->ReadAt(0x54, saves.death);
	fData->ReadAt(0x55, saves.wands);
	fData->ReadAt(0x56, saves.poly);
	fData->ReadAt(0x57, saves.breath);
	fData->ReadAt(0x58, saves.spell);
	return saves;
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


bool
CREResource::GetItemAtSlot(uint32 i, IE::item& item) const
{
	if (i >= kNumItemSlots)
		throw std::out_of_range("ItemAtSlot() out of range");

	uint16 itemOffset;
	fData->ReadAt(fItemSlotOffset + i * sizeof(itemOffset), itemOffset);

	//std::cout << "item at slot " << std::dec << i;
	//std::cout << " :" << std::dec << itemOffset << std::endl;

	// TODO: number 38 is a dword instead. Handle that case
	if ((int16)itemOffset == -1) {
		// empty
		return false;
	}

	IE::item ieItem;
	_ReadItemNum(item, itemOffset);

	return true;
}


int32
CREResource::ItemsIndexAtSlot(uint32 slot) const
{
	if (slot >= kNumItemSlots)
		throw std::out_of_range("ItemsIndexAtSlot() out of range");

	int16 itemOffset;
	fData->ReadAt(fItemSlotOffset + slot * sizeof(itemOffset), itemOffset);
	return itemOffset == -1 ? -1 : (int32)itemOffset;
}


void
CREResource::SetItemAtSlot(uint32 slot, int32 itemsIndex)
{
	if (slot >= kNumItemSlots)
		throw std::out_of_range("SetItemAtSlot() out of range");

	int16 value = (int16)itemsIndex;
	fData->WriteAt(fItemSlotOffset + slot * sizeof(value), &value, sizeof(value));
}


void
CREResource::MoveItemBetweenSlots(uint32 fromSlot, uint32 toSlot)
{
	int32 itemsIndex = ItemsIndexAtSlot(fromSlot);
	SetItemAtSlot(fromSlot, -1);
	SetItemAtSlot(toSlot, itemsIndex);
}


int32
CREResource::FindFreeSlot(uint32 firstSlot, uint32 lastSlot) const
{
	for (uint32 slot = firstSlot; slot <= lastSlot && slot < kNumItemSlots; slot++) {
		int16 itemOffset;
		fData->ReadAt(fItemSlotOffset + slot * sizeof(itemOffset), itemOffset);
		if (itemOffset == -1)
			return (int32)slot;
	}
	return -1;
}


int32
CREResource::FindItemSlot(const res_ref& itemName) const
{
	for (uint32 slot = 0; slot < kNumItemSlots; slot++) {
		IE::item item;
		if (GetItemAtSlot(slot, item) && item.name == itemName)
			return (int32)slot;
	}
	return -1;
}


int32
CREResource::FindFreeItemsEntry() const
{
	for (uint32 i = 0; i < fItemsCount; i++) {
		IE::item item;
		_ReadItemNum(item, (uint16)i);
		if (item.name.name[0] == '\0')
			return (int32)i;
	}
	return -1;
}


void
CREResource::SetItemAtItemsIndex(uint16 index, const IE::item& item)
{
	if (index >= fItemsCount)
		throw std::out_of_range("SetItemAtItemsIndex() out of range");

	const off_t offset = fItemsOffset + index * sizeof(IE::item);
	fData->WriteAt(offset, &item, sizeof(item));
}


std::vector<cre_known_spell>
CREResource::KnownSpells() const
{
	const uint32 kEntrySize = 12;

	std::vector<cre_known_spell> spells;
	for (uint32 i = 0; i < fKnownSpellsCount; i++) {
		uint32 offset = fKnownSpellsOffset + i * kEntrySize;

		cre_known_spell spell;
		fData->ReadAt(offset + 0x00, spell.spell);
		fData->ReadAt(offset + 0x08, spell.level);
		spell.level += 1; // on-disk value is (level - 1)
		fData->ReadAt(offset + 0x0a, spell.type);

		spells.push_back(spell);
	}
	return spells;
}


std::vector<cre_memorized_spell>
CREResource::MemorizedSpells() const
{
	const uint32 kEntrySize = 12;

	std::vector<cre_memorized_spell> spells;
	for (uint32 i = 0; i < fMemorizedSpellsCount; i++) {
		uint32 offset = fMemorizedSpellsOffset + i * kEntrySize;

		cre_memorized_spell spell;
		fData->ReadAt(offset + 0x00, spell.spell);
		fData->ReadAt(offset + 0x08, spell.flags);

		spells.push_back(spell);
	}
	return spells;
}


bool
CREResource::ConsumeMemorizedSpell(const res_ref& spellName)
{
	const uint32 kEntrySize = 12;

	for (uint32 i = 0; i < fMemorizedSpellsCount; i++) {
		uint32 offset = fMemorizedSpellsOffset + i * kEntrySize;

		res_ref spell;
		fData->ReadAt(offset + 0x00, spell);
		if (spell != spellName)
			continue;

		uint32 flags;
		fData->ReadAt(offset + 0x08, flags);
		if ((flags & 1) == 0)
			continue; // known, but not currently memorized/available

		flags &= ~1;
		fData->WriteAt(offset + 0x08, &flags, sizeof(flags));
		return true;
	}
	return false;
}


void
CREResource::RestoreMemorizedSpells()
{
	const uint32 kEntrySize = 12;

	for (uint32 i = 0; i < fMemorizedSpellsCount; i++) {
		uint32 offset = fMemorizedSpellsOffset + i * kEntrySize;

		uint32 flags;
		fData->ReadAt(offset + 0x08, flags);
		flags |= 1;
		fData->WriteAt(offset + 0x08, &flags, sizeof(flags));
	}
}


std::vector<cre_spell_memorization_info>
CREResource::SpellMemorizationInfo() const
{
	const uint32 kEntrySize = 16;

	std::vector<cre_spell_memorization_info> info;
	for (uint32 i = 0; i < fSpellMemoInfoCount; i++) {
		uint32 offset = fSpellMemoInfoOffset + i * kEntrySize;

		cre_spell_memorization_info entry;
		fData->ReadAt(offset + 0x00, entry.level);
		entry.level += 1; // on-disk value is (level - 1)
		fData->ReadAt(offset + 0x02, entry.numMemorizable);
		fData->ReadAt(offset + 0x04, entry.numMemorizableEffective);
		fData->ReadAt(offset + 0x06, entry.type);
		fData->ReadAt(offset + 0x08, entry.firstMemorizedIndex);
		fData->ReadAt(offset + 0x0c, entry.memorizedCount);

		info.push_back(entry);
	}
	return info;
}


std::string
CREResource::DeathVariable() const
{
	// The file field is NOT guaranteed to be NUL-terminated
	char temp[33];
	fData->ReadAt(0x280, temp, 32);
	temp[32] = '\0';
	return temp;
}


void
CREResource::_ReadItemNum(IE::item& item, uint16 itemOffset) const
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
