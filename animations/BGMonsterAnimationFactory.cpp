/*
 * BGAnimationFactory.cpp
 *
 *  Created on: 16/nov/2014
 *      Author: stefano
 */

#include "BGMonsterAnimationFactory.h"

#include "Actor.h"
#include "Animation.h"
#include "Core.h"

BGMonsterAnimationFactory::BGMonsterAnimationFactory(const char* baseName, const uint16 id)
	:
	AnimationFactory(baseName, id)
{
}


BGMonsterAnimationFactory::~BGMonsterAnimationFactory()
{
}


/* virtual */
animation_description
BGMonsterAnimationFactory::GetAnimationDescription(Actor* actor)
{
	//std::cout << "BGAnimationFactory" << std::endl;
	int o = actor->Orientation();
	animation_description description;
	description.bam_name = BaseName();
	description.mirror = false;

	// TODO: Improve this
	if (Core::Get()->Game() == GAME_BALDURSGATE2) {
		if (o >= IE::ORIENTATION_EXT_NNE
				&& uint32(o) <= IE::ORIENTATION_EXT_SSE) {
			// Orientation 5 uses bitmap from orientation 3 mirrored,
			// 6 uses 2, and 7 uses 1
			description.mirror = true;
			o = 16 - o;
		}
	} else {
		if (o >= IE::ORIENTATION_NE && uint32(o) <= IE::ORIENTATION_SE) {
			// Orientation 5 uses bitmap from orientation 3 mirrored,
			// 6 uses 2, and 7 uses 1
			description.mirror = true;
			o = 8 - o;
		}
	}
	description.sequence_number = o;
	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			if (_HasG11(description.bam_name))
				description.bam_name.append("G11");
			else
				description.bam_name.append("G1");
			break;
		case ACT_STANDING:
			description.bam_name.append("G1");
			description.sequence_number += ANIM_STANDING_OFFSET;
			break;
		case ACT_ATTACKING:
			description.bam_name.append("G2");
			//description.sequence_number = o;
			break;
		case ACT_CAST_SPELL_PREPARE:
			description.bam_name.append("G25");
			description.sequence_number += 45;
			break;
		case ACT_CAST_SPELL_RELEASE:
			description.bam_name.append("G26");
			description.sequence_number += 54;
			break;
		default:
			std::cerr << "BGMonsterAnimationFactory::GetAnimationDescription(): UNIMPLEMENTED ";
			std::cerr << BaseName() << ", action " << actor->AnimationAction() << ", orientation " << o << std::endl;
			break;
	}
	return description;
}

