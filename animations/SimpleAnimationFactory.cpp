/*
 * SimpleAnimationFactory.cpp
 *
 *  Created on: 19/nov/2014
 *      Author: stefano
 */

#include "SimpleAnimationFactory.h"

#include "Actor.h"
#include "Animation.h"
#include "Core.h"

SimpleAnimationFactory::SimpleAnimationFactory(const char* baseName, const uint16 id)
	:
	AnimationFactory(baseName, id)
{
}


SimpleAnimationFactory::~SimpleAnimationFactory()
{
}


/* virtual */
animation_description
SimpleAnimationFactory::GetAnimationDescription(Actor* actor)
{
	//std::cout << "SimpleAnimationFactory::AnimationFor" << std::endl;
	int o = actor->Orientation();
	animation_description description;
	description.bam_name = BaseName();
	description.mirror = false;
	description.custom_colors = true;

	if (Core::Get()->Game() == game::GAME_BALDURSGATE2)
		o = IE::orientation_ext_to_base(o);

	description.sequence_number = uint32(o);

	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			description.sequence_number += 0;
			break;
		case ACT_ATTACKING:
			description.sequence_number += 0;
			break;
		case ACT_STANDING:
			description.sequence_number += 8;
			break;
		case ACT_DIE:
		default:
			std::cout << "unknown action " << actor->AnimationAction() << std::endl;
			break;
	}

	description.bam_name.append("M");
	if (o >= IE::ORIENTATION_NE
			&& uint32(o) <= IE::ORIENTATION_SE) {
		// Orientation 5 uses bitmap from orientation 3 mirrored,
		// 6 uses 2, and 7 uses 1
		//description.mirror = true;
		// TODO: not in BG2. There is a separate file with East-facing animation
//		/description.bam_name.append("E");
		//description.sequence_number -= (o - 4) * 2;
	}
	return description;
}

