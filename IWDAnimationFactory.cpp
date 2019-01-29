/*
 * IWDAnimationFactory.cpp
 *
 *  Created on: 16/nov/2014
 *      Author: stefano
 */

#include "IWDAnimationFactory.h"

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
void
IWDAnimationFactory::GetAnimationDescription(int action, int o, animation_description& description)
{
	//std::cout << "IWDAnimationFactory" << std::endl;
	description.bam_name = fBaseName;
	description.mirror = false;

	if (Core::Get()->Game() == GAME_BALDURSGATE2)
		o = IE::orientation_ext_to_base(o);

	switch (action) {
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
			break;
	}
	if (o >= IE::ORIENTATION_NE
			&& o <= IE::ORIENTATION_SE) {
		description.bam_name.append("E");
	}
}

