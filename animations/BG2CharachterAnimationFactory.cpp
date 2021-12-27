/*
 * BGAnimationFactory.cpp
 *
 *  Created on: 16/nov/2014
 *      Author: stefano
 */

#include "BG2CharachterAnimationFactory.h"

#include "Actor.h"
#include "Animation.h"
#include "Core.h"
#include "CreResource.h"


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
animation_description
BG2CharachterAnimationFactory::GetAnimationDescription(Actor* actor)
{
	//std::cout << "BG2AnimationFactory::AnimationFor" << std::endl;
	int o = actor->Orientation();
	animation_description description;
	description.mirror = false;
	description.sequence_number = 0;

	if (actor->InParty()) {
		// Charachter animations are specific
		description.bam_name = "";
		description.bam_name.append("C");

		// Race
		description.bam_name.append(_RaceCharacter(actor->CRE()->Race()));
		// Gender
		description.bam_name.append(_GenderCharacter(actor->CRE()->Gender()));
		// Class
		description.bam_name.append(_ClassCharacter(actor->CRE()->Class()));

		// Armor
		// TODO: Improve
		std::string armorAnimation = _ArmorCharacter(actor);
		//std::cout << armorAnimation << std::endl;
		// TODO: Correct ? Fighters seems always to have full plate
		if (description.bam_name[3] == 'F')
			description.bam_name.append("4");
		else
			description.bam_name.append(armorAnimation);
	} else {
		description.bam_name = fBaseName;
		description.bam_name.append("1");
	}

	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			if (_HasW(description.bam_name))
				description.bam_name.append("W2");
			else
				description.bam_name.append("G11");
			break;
		case ACT_STANDING:
			description.bam_name.append("G1");
			if (Core::Get()->HasExtendedOrientations())
				description.sequence_number += ANIM_STANDING_OFFSET;
			else
				description.sequence_number += 8;
			break;
		case ACT_ATTACKING:
			description.bam_name.append("A1");
			break;
		case ACT_DEAD:
			description.bam_name.append("G15");
			description.sequence_number += ANIM_DEAD_OFFSET;
			break;
		case ACT_CAST_SPELL:
			std::cout << "CAST SPELL: " << fBaseName << std::endl;
			description.bam_name.append("C1");
			//description.sequence_number += 9;
			break;
		default:
			std::cerr << "BG2CharachterAnimationFactory::GetAnimationDescription(): UNIMPLEMENTED ";
			std::cerr << fBaseName << ", action " << actor->AnimationAction() << ", orientation " << o << std::endl;
			break;
	}
	if (Core::Get()->HasExtendedOrientations()) {
		if (o >= IE::ORIENTATION_EXT_NNE && o <= IE::ORIENTATION_EXT_SSE)
			_GetMirroredAnimation(o, description);
	} else {
		if (o >= IE::ORIENTATION_NE && o <= IE::ORIENTATION_SE) {
			if (_HasSeparateEasternOrientations(description.bam_name))
				description.bam_name.append("E");
		}
	}
	description.sequence_number += o;

#if 0
	std::cout << description.bam_name << std::endl;
#endif
	return description;
}


void
BG2CharachterAnimationFactory::_GetMirroredAnimation(int& orientation, animation_description& description)
{
	description.mirror = true;
	orientation = 16 - orientation;
}

