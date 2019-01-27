/*
 * SplitAnimationFactory.cpp
 *
 *  Created on: 19/nov/2014
 *      Author: stefano
 */

#include "SplitAnimationFactory.h"

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
void
SplitAnimationFactory::GetAnimationDescription(int action, int o, animation_description& description)
{
	//std::cout << "SplitAnimationFactory::AnimationFor" << std::endl;
	description.bam_name = fBaseName;
	description.mirror = false;
	
	if (Core::Get()->Game() == GAME_BALDURSGATE2)
		o = IE::orientation_ext_to_base(o);
		
	// G1/G11-G15, G2/G21/26
	if (IE::is_orientation_facing_north(o))
		description.bam_name.append("H");
	else 
		description.bam_name.append("L");

	switch (action) {
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

	if (o >= IE::ORIENTATION_NE
			&& uint32(o) <= IE::ORIENTATION_SE) {
		// Orientation 5 uses bitmap from orientation 3 mirrored,
		// 6 uses 2, and 7 uses 1
		description.mirror = true;
		description.sequence_number -= (o - 4) * 2;
	}
	if (fBaseName == "NSIM") {
		std::cout << "orientation: " << o << std::endl;
		std::cout << "description.sequence: " << description.sequence_number << std::endl;
		std::cout << "bam: " << description.bam_name << std::endl;
	}
}

