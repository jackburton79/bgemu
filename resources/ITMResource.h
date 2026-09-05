	/*
 * ITMResource.h
 *
 *  Created on: 25/mag/2013
 *      Author: stefano
 */

#ifndef __ITMRESOURCE_H_
#define __ITMRESOURCE_H_

#include "Resource.h"

#include <string>

struct itm_header {
	uint32 name_unidentified;
	uint32 name_identified;
	res_ref replacement_item;
	uint32 flags;
	uint16 type;
	char usability[4];
	char animation[2];

	// TODO: Rest
};


// One 56-byte "Extended Header" (ability) entry - see IESDP itm_v1. Only
// the fields needed to resolve a melee to-hit/damage roll are read; ammo,
// projectile animation, targeting and the ability's own on-hit feature
// blocks are not (out of scope - see the combat implementation plan).
struct itm_ability {
	uint8 attackType;	// 0=None, 1=Melee, 2=Projectile, 3=Magic, 4=Launcher
	uint16 range;
	uint8 speed;		// speed factor
	int16 thac0Bonus;	// signed: cursed items can be negative; 32767 = always hits
	uint8 diceSides;
	uint8 diceThrown;
	int16 damageBonus;	// signed: cursed items can be negative
	uint16 damageType;	// 0=None,1=Piercing/Magic,2=Blunt,3=Slashing,4=Missile,5=Fists
};


class ITMResource: public Resource {
public:
	ITMResource(const res_ref& name);
	static Resource* Create(const res_ref& name);

	virtual bool Load(Archive *archive, uint32 key);

	uint16 Type() const;
	std::string Animation() const;
	uint32 DescriptionRef() const;

	// Reads the index-th ability (Extended Header). Returns false (and
	// leaves ability untouched) if the item doesn't have that many.
	bool GetAbility(uint16 index, itm_ability& ability) const;

private:
	virtual ~ITMResource();

	itm_header fHeader;
};

#endif /* ITMRESOURCE_H_ */
