/*
 * BGAnimationFactory.cpp
 *
 *  Created on: 16/nov/2014
 *      Author: stefano
 */

#include "BG2CharachterAnimationFactory.h"

#include "Actor.h"
#include "Animation.h"
#include "CreResource.h"

// TODO: Move these to a common header ?
#define ANIM_STANDING_OFFSET 9
#define ANIM_DEAD_OFFSET 48

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

	if (o >= IE::ORIENTATION_EXT_NNE && o <= IE::ORIENTATION_EXT_SSE)
		_GetMirroredAnimation(o, description);

	description.sequence_number = o;

	// TODO: In theory, fBaseName should already be correct, but in
	// practice, it's not
	if (actor->InParty()) {
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
			description.bam_name.append("G11");
			break;
		case ACT_STANDING:
			description.bam_name.append("G1");
			description.sequence_number += ANIM_STANDING_OFFSET;
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

