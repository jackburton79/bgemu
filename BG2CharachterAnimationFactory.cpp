/*
 * BGAnimationFactory.cpp
 *
 *  Created on: 16/nov/2014
 *      Author: stefano
 */

#include "BG2CharachterAnimationFactory.h"

#include "Animation.h"

BG2CharachterAnimationFactory::BG2CharachterAnimationFactory(
		const char* baseName, const uint16 id)
	:
	AnimationFactory(baseName, id)
{
}


BG2CharachterAnimationFactory::~BG2CharachterAnimationFactory()
{
}


/* virtual */
void
BG2CharachterAnimationFactory::GetAnimationDescription(int action, int o, animation_description& description)
{
	//std::cout << "BG2AnimationFactory::AnimationFor" << std::endl;
	description.bam_name = fBaseName;
	description.sequence_number = o;
	description.mirror = false;
	// Armor
	// TODO: For real
	description.bam_name.append("1");
	switch (action) {
		case ACT_WALKING:
			description.bam_name.append("G11");
			break;
		case ACT_STANDING:
			description.bam_name.append("G1");
			description.sequence_number = o + 9;
			break;
		case ACT_ATTACKING:
			description.bam_name.append("A1");
			break;
		case ACT_DEAD:
			description.bam_name.append("G15");
			description.sequence_number = 48 + o;
			break;
		default:
			break;
	}
	if (o >= IE::ORIENTATION_EXT_NNE && o <= IE::ORIENTATION_EXT_SSE) {
		description.mirror = true;
		description.sequence_number -= (o - 8) * 2;
	}
}

