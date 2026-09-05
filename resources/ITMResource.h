	/*
 * ITMResource.h
 *
 *  Created on: 25/mag/2013
 *      Author: stefano
 */

#ifndef __ITMRESOURCE_H_
#define __ITMRESOURCE_H_

#include "Resource.h"
#include "SPLResource.h"	// spl_effect, ReadFeatureBlocks() - shared feature-block format

#include <string>
#include <vector>

struct itm_header {
	uint32 name_unidentified;
	uint32 name_identified;
	res_ref replacement_item;
	uint32 flags;
	uint16 type;
	char usability[4];
	char animation[2];
	uint16 min_level;
	uint16 min_strength;
	uint8 min_strength_bonus;
	uint8 kit_usability1;
	uint8 min_intelligence;
	uint8 kit_usability2;
	uint8 min_dexterity;
	uint8 kit_usability3;
	uint8 min_wisdom;
	uint8 kit_usability4;
	uint8 min_constitution;
	uint8 weapon_proficiency;
	uint16 min_charisma;
	uint32 price;
	uint16 stack_amount;
	res_ref inventory_icon;
	uint16 lore_to_id;
	res_ref ground_icon;
	uint32 weight;
	// description/description-icon refs skipped: exposed via DescriptionRef()
	uint32 enchantment;
};


// One 56-byte "Extended Header" (ability) entry - see IESDP itm_v1.
struct itm_ability {
	uint8 attackType;	// 0=None, 1=Melee, 2=Projectile, 3=Magic, 4=Launcher
	uint16 range;
	uint8 speed;		// speed factor
	int16 thac0Bonus;	// signed: cursed items can be negative; 32767 = always hits
	uint8 diceSides;
	uint8 diceThrown;
	int16 damageBonus;	// signed: cursed items can be negative
	uint16 damageType;	// 0=None,1=Piercing/Magic,2=Blunt,3=Slashing,4=Missile,5=Fists
	uint16 featureBlockCount;
	uint16 featureBlockIndex;
};


class ITMResource: public Resource {
public:
	ITMResource(const res_ref& name);
	static Resource* Create(const res_ref& name);

	virtual bool Load(Archive *archive, uint32 key);

	uint16 Type() const;
	std::string Animation() const;
	uint32 DescriptionRef() const;

	uint32 Weight() const;
	uint32 Price() const;
	uint16 StackAmount() const;

	// Reads the index-th ability (Extended Header). Returns false (and
	// leaves ability untouched) if the item doesn't have that many.
	bool GetAbility(uint16 index, itm_ability& ability) const;

	// On-hit effects carried by the given ability (its own feature
	// blocks) - e.g. a sword that poisons on hit.
	std::vector<spl_effect> OnHitEffects(uint16 abilityIndex) const;

	// Effects applied for as long as the item is equipped (not tied to
	// any single ability) - e.g. a ring that grants +1 AC.
	std::vector<spl_effect> EquippingEffects() const;

private:
	virtual ~ITMResource();

	itm_header fHeader;
	uint32 fExtHeaderOffset;
	uint16 fExtHeaderCount;
	uint32 fFeatureBlockOffset;
	uint16 fEquipFeatureBlockIndex;
	uint16 fEquipFeatureBlockCount;
};

#endif /* ITMRESOURCE_H_ */
