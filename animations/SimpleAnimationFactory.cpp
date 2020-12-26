/*
 * SimpleAnimationFactory.cpp
 *
 *  Created on: 18/nov/2014
 *      Author: stefano
 */

#include "SimpleAnimationFactory.h"

#include "Actor.h"
#include "Animation.h"

SimpleAnimationFactory::SimpleAnimationFactory(
		const char* baseName, const uint16 id)
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
	//std::cout << "SimpleAnimationFactory::AnimationFor(";
	//std::cout << action << ", " << o << ")" << std::endl;
	int o = actor->Orientation();
	animation_description description;
	description.bam_name = fBaseName;
	description.sequence_number = o;
	description.mirror = false;

	// G1/G11-G15, G2/G21/26
	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			description.bam_name.append("G1");
			description.sequence_number = uint32(o);
			break;
		case ACT_ATTACKING:
			description.bam_name.append("G1");
			description.sequence_number = uint32(o);
			break;
		case ACT_STANDING:
			description.bam_name.append("G1");
			description.sequence_number = uint32(o);
			break;
		default:
			break;
	}
	if (o >= IE::ORIENTATION_NE && uint32(o) <= IE::ORIENTATION_SE) {
		// Orientation 5 uses bitmap from orientation 3 mirrored,
		// 6 uses 2, and 7 uses 1
		description.mirror = true;
		description.sequence_number -= (o - 4) * 2;
	}
	return description;
}
