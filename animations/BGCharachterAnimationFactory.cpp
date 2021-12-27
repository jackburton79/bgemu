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
	description.bam_name = BaseName();
	description.sequence_number = o;
	description.mirror = false;

	// Optional weapon id
	// TODO: improve
	if (!actor->WeaponAnimation().empty())
		description.bam_name.append(actor->WeaponAnimation().substr(0, 1));

	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			if (_HasW(description.bam_name))
				description.bam_name.append("W2");
			else
				description.bam_name.append("G1");
			description.sequence_number = o;
			break;
		case ACT_STANDING:
			description.bam_name.append("G1");
			description.sequence_number += ANIM_STANDING_OFFSET;
			break;
		case ACT_ATTACKING:
			description.bam_name.append("A1");
			description.sequence_number = o;
			break;
		default:
			std::cerr << "BGCharachterAnimationFactory::GetAnimationDescription(): UNIMPLEMENTED ";
			std::cerr << BaseName() << ", action " << actor->AnimationAction() << ", orientation " << o << std::endl;
			break;
	}
	if (o >= IE::ORIENTATION_NE
			&& o <= IE::ORIENTATION_SE) {
		description.bam_name.append("E");
	}
	return description;
}

