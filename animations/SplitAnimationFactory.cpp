/*
 * SplitAnimationFactory.cpp
 *
 *  Created on: 19/nov/2014
 *      Author: stefano
 */

#include "SplitAnimationFactory.h"

#include "Actor.h"
#include "Animation.h"
#include "Core.h"

SplitAnimationFactory::SplitAnimationFactory(const char* baseName, const uint16 id)
	:
	AnimationFactory(baseName, id)
{
}


SplitAnimationFactory::~SplitAnimationFactory()
{
}


/* virtual */
animation_description
SplitAnimationFactory::GetAnimationDescription(Actor* actor)
{
	//std::cout << "SplitAnimationFactory::AnimationFor" << std::endl;
	int o = actor->Orientation();
	animation_description description;
	description.bam_name = fBaseName;
	description.mirror = false;
	
	if (Core::Get()->Game() == GAME_BALDURSGATE2)
		o = IE::orientation_ext_to_base(o);

	description.sequence_number = uint32(o);

	// G1
	//if (IE::is_orientation_facing_north(o))
		description.bam_name.append("H");
	//else 
		//description.bam_name.append("L");
	
	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			description.bam_name.append("G1");
			description.sequence_number += 0;
			break;
		case ACT_ATTACKING:
			description.bam_name.append("G1");
			description.sequence_number += 0;
			break;
		case ACT_STANDING:
			description.bam_name.append("G1");
			description.sequence_number += 8;
			break;
		default:
			break;
	}

	if (o >= IE::ORIENTATION_NE
			&& uint32(o) <= IE::ORIENTATION_SE) {
		// Orientation 5 uses bitmap from orientation 3 mirrored,
		// 6 uses 2, and 7 uses 1
		//description.mirror = true;
		// TODO: not in BG2. There is a separate file with East-facing animation
		description.bam_name.append("E");
		//description.sequence_number -= (o - 4) * 2;
	}
	return description;
}

