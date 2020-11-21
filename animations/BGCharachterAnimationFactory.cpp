/*
 * BGAnimationFactory.cpp
 *
 *  Created on: 16/nov/2014
 *      Author: stefano
 */

#include "BGCharachterAnimationFactory.h"

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
void
BGCharachterAnimationFactory::GetAnimationDescription(int action, int o, animation_description& description)
{
	//std::cout << "BGAnimationFactory" << std::endl;
	description.bam_name = fBaseName;
	description.sequence_number = o;
	description.mirror = false;
	// Armor
	// TODO: For real
	description.bam_name.append("1");
	switch (action) {
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
			std::cerr << fBaseName << ", action " << action << ", orientation " << o << std::endl;
			break;
	}
	if (o >= IE::ORIENTATION_NE
			&& o <= IE::ORIENTATION_SE) {
		description.bam_name.append("E");
	}
}

