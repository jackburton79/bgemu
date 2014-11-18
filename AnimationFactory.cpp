/*
 * AnimationFactory.cpp
 *
 *  Created on: 20/mag/2013
 *      Author: stefano
 */

#include "Animation.h"
#include "AnimationFactory.h"
#include "BGCharachterAnimationFactory.h"
#include "BG2CharachterAnimationFactory.h"
#include "Core.h"
#include "BamResource.h"
#include "ResManager.h"

#include <string>
#include <vector>


std::map<std::string, AnimationFactory*> AnimationFactory::sAnimationFactory;

const int kStandingOffset = 10;


/* static */
AnimationFactory*
AnimationFactory::GetFactory(uint16 animationID)
{
	std::string baseName = IDTable::AniSndAt(animationID);

	AnimationFactory* factory = NULL;
	std::map<std::string, AnimationFactory*>::const_iterator i
		= sAnimationFactory.find(baseName);
	if (i != sAnimationFactory.end())
		factory = i->second;
	else {
		switch (Core::Get()->Game()) {
			case GAME_BALDURSGATE:
				if (animationID >= 0x6000 && animationID <= 0x9000)
					factory = new BGCharachterAnimationFactory(baseName.c_str());
				break;
			case GAME_BALDURSGATE2:
				if (animationID >= 0x5000 && animationID <= 0x9000)
					factory = new BG2CharachterAnimationFactory(baseName.c_str());
				break;
			default:
				break;
		}

		if (factory == NULL)
			factory = new AnimationFactory(baseName.c_str());

		factory->fID = animationID;
	}


	factory->Acquire();

	return factory;
}


/* static */
void
AnimationFactory::ReleaseFactory(AnimationFactory* factory)
{
	if (factory->Release()) {
		sAnimationFactory.erase(factory->fBaseName);
		delete factory;
	}
}


AnimationFactory::AnimationFactory(const char* baseName)
{
	fBaseName = baseName;
}


AnimationFactory::~AnimationFactory()
{
	std::map<std::pair<int, int>, Animation*>::const_iterator i;
	for (i = fAnimations.begin(); i != fAnimations.end(); i++)
		delete i->second;

	fAnimations.clear();
}



animation_description
AnimationFactory::MonsterAnimationFor(int action, int o)
{
	std::cout << "MonsterAnimationFor" << std::endl;

	animation_description description;
	description.bam_name = fBaseName;
	description.sequence_number = o;
	description.mirror = false;

	// G1/G11-G15, G2/G21/26
	/*if (_AreHighLowSplitted()) {
		description.bam_name.append("H");
	}*/
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
			/*if (_HasAnimation(description.bam_name + "G1")) {
				description.bam_name.append("G1");
				description.sequence_number = uint32(o);
			} else {
				description.bam_name.append("C");
				description.sequence_number = uint32(o);
			}
			if (_HasStandingSequence())
				description.sequence_number += kStandingOffset;*/
			break;
		default:
			break;
	}
	if (uint32(o) >= IE::ORIENTATION_NE && uint32(o) <= IE::ORIENTATION_SE) {
		/*if (_HasEastBams()) {
			description.bam_name.append("E");
			// TODO: Doesn't work for some animations (IE: ACOW)
			//sequence_number -= 5;
		} else */
		{
			// Orientation 5 uses bitmap from orientation 3 mirrored,
			// 6 uses 2, and 7 uses 1
			description.mirror = true;
			description.sequence_number -= (uint32(o) - 4) * 2;
		}
	}

	return description;
}



Animation*
AnimationFactory::AnimationFor(int action, int o)
{
	// Check if animation was already loaded
	std::pair<int, int> key = std::make_pair(action, o);
	std::map<std::pair<int, int>, Animation*>::const_iterator i;
	i = fAnimations.find(key);
	if (i != fAnimations.end())
		return i->second;

	std::cout << "Basename: " << fBaseName << ", ID: ";
	std::cout << std::hex << fID << std::endl;

	return NULL;
}


Animation*
AnimationFactory::InstantiateAnimation(
		const animation_description description,
		const std::pair<int, int> key)
{
	Animation* animation = NULL;
	try {
		IE::point pos;
		animation = new Animation(description.bam_name.c_str(),
								description.sequence_number, pos);
	} catch (...) {
		animation = NULL;
	}

	if (animation != NULL) {
		if (description.mirror)
			animation->SetMirrored(true);
		fAnimations[key] = animation;
	}
	return animation;
}

