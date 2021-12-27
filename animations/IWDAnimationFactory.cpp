/*
 * IWDAnimationFactory.cpp
 *
 *  Created on: 16/nov/2014
 *      Author: stefano
 */

#include "IWDAnimationFactory.h"

#include "Actor.h"
#include "Animation.h"
#include "Core.h"

IWDAnimationFactory::IWDAnimationFactory(const char* baseName, const uint16 id)
	:
	AnimationFactory(baseName, id)
{
}


IWDAnimationFactory::~IWDAnimationFactory()
{
}


/* virtual */
animation_description
IWDAnimationFactory::GetAnimationDescription(Actor* actor)
{
	//std::cout << "IWDAnimationFactory" << std::endl;
	int o = actor->Orientation();
	animation_description description;
	description.bam_name = BaseName();
	description.mirror = false;

	if (Core::Get()->Game() == GAME_BALDURSGATE2)
		o = IE::orientation_ext_to_base(o);

	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			description.bam_name.append("WK");
			description.sequence_number = o;
			break;
		case ACT_STANDING:
			description.bam_name.append("SD");
			description.sequence_number = o;
			break;
		case ACT_ATTACKING:
			description.bam_name.append("A1");
			description.sequence_number = o;
			break;
		default:
			std::cerr << "IWDAnimationFactory::GetAnimationDescription(): UNIMPLEMENTED ";
			std::cerr << BaseName() << ", action " << actor->AnimationAction() << ", orientation " << o << std::endl;
			break;
	}
	if (o >= IE::ORIENTATION_NE
			&& o <= IE::ORIENTATION_SE) {
		description.bam_name.append("E");
	}
	return description;
}

