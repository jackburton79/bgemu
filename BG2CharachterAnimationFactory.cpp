/*
 * BGAnimationFactory.cpp
 *
 *  Created on: 16/nov/2014
 *      Author: stefano
 */

#include "BG2CharachterAnimationFactory.h"

#include "Animation.h"

BG2CharachterAnimationFactory::BG2CharachterAnimationFactory(const char* baseName)
	:
	AnimationFactory(baseName)
{
}


BG2CharachterAnimationFactory::~BG2CharachterAnimationFactory()
{
}


Animation*
BG2CharachterAnimationFactory::AnimationFor(int action, int o)
{
	// Check if animation was already loaded
	// TODO: code duplication, put this into AnimationFactory
	std::pair<int, int> key = std::make_pair(action, o);
	std::map<std::pair<int, int>, Animation*>::const_iterator i;
	i = fAnimations.find(key);
	if (i != fAnimations.end())
		return i->second;

	std::cout << "BG2AnimationFactory::AnimationFor" << std::endl;

	animation_description description;
	description.bam_name = fBaseName;
	description.sequence_number = o;
	description.mirror = false;
	// Armor
	// TODO: For real
	description.bam_name.append("1");
	switch (action) {
		case ACT_WALKING:
		{
			description.bam_name.append("G11");
			break;
		}
		case ACT_STANDING:
			description.bam_name.append("G1");
			description.sequence_number = o + 9;
			break;
		case ACT_ATTACKING:
			description.bam_name.append("A1");
			break;
		default:
			break;
	}
	if (o >= IE::ORIENTATION_EXT_NNE && o <= IE::ORIENTATION_EXT_SSE) {
		description.mirror = true;
		description.sequence_number -= (o - 8) * 2;
	}

	return InstantiateAnimation(description, key);
}

