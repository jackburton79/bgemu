/*
 * BGAnimationFactory.cpp
 *
 *  Created on: 16/nov/2014
 *      Author: stefano
 */

#include "BGCharachterAnimationFactory.h"

#include "Actor.h"
#include "Animation.h"

BGCharachterAnimationFactory::BGCharachterAnimationFactory(const char* baseName, const uint16 id)
	:
	AnimationFactory(baseName, id)
{
}


BGCharachterAnimationFactory::~BGCharachterAnimationFactory()
{
}


/* virtual */
animation_description
BGCharachterAnimationFactory::GetAnimationDescription(Actor* actor)
{
	//std::cout << "BGAnimationFactory" << std::endl;
	int o = actor->Orientation();
	animation_description description;
	description.bam_name = fBaseName;
	description.sequence_number = o;
	description.mirror = false;
	// Armor
	// TODO: Improve
	std::string armorAnimation = _ArmorCharacter(actor);
	//std::cout << armorAnimation << std::endl;
	// TODO: Correct ? Fighters seems always to have full plate
	if (description.bam_name[3] == 'F')
		description.bam_name.append("4");
	else
		description.bam_name.append(armorAnimation);

	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			description.bam_name.append("W2");
			description.sequence_number = o;
			break;
		case ACT_STANDING:
			description.bam_name.append("G1");
			description.sequence_number = o + 8;
			break;
		case ACT_ATTACKING:
			description.bam_name.append("A1");
			description.sequence_number = o;
			break;
		default:
			std::cerr << "BGCharachterAnimationFactory::GetAnimationDescription(): UNIMPLEMENTED ";
			std::cerr << fBaseName << ", action " << actor->AnimationAction() << ", orientation " << o << std::endl;
			break;
	}
	if (o >= IE::ORIENTATION_NE
			&& o <= IE::ORIENTATION_SE) {
		description.bam_name.append("E");
	}
	return description;
}

