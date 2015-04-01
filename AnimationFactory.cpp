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
#include "BamResource.h"
#include "Core.h"
#include "ResManager.h"
#include "SimpleAnimationFactory.h"
#include "SplitAnimationFactory.h"

#include <string>
#include <vector>


std::map<uint16, AnimationFactory*> AnimationFactory::sAnimationFactory;

const int kStandingOffset = 10;


/* static */
AnimationFactory*
AnimationFactory::GetFactory(uint16 animationID)
{
	std::string baseName = IDTable::AniSndAt(animationID);

	std::cout << "AnimationFactory::GetFactory(";
	std::cout << baseName << ", " << std::hex;
	std::cout << animationID << ")" << std::endl;
	AnimationFactory* factory = NULL;
	std::map<uint16, AnimationFactory*>::const_iterator i
		= sAnimationFactory.find(animationID);
	if (i != sAnimationFactory.end())
		factory = i->second;
	else {
		switch (Core::Get()->Game()) {
			case GAME_BALDURSGATE:
				if (animationID >= 0x5000 && animationID < 0x8000)
					factory = new BGCharachterAnimationFactory(baseName.c_str(), animationID);
				else if (animationID >= 0xc000 && animationID <= 0xca00)
					factory = new SplitAnimationFactory(baseName.c_str(), animationID);
				else if (animationID >= 0xb000 && animationID <= 0xd300)
					factory = new SimpleAnimationFactory(baseName.c_str(), animationID);
				break;
			case GAME_BALDURSGATE2:
				if (animationID >= 0x5000 && animationID <= 0x9000)
					factory = new BG2CharachterAnimationFactory(baseName.c_str(), animationID);
				break;
			default:
				break;
		}

		if (factory == NULL)
			factory = new AnimationFactory(baseName.c_str(), animationID);
	}


	factory->Acquire();

	return factory;
}


/* static */
void
AnimationFactory::ReleaseFactory(AnimationFactory* factory)
{
	if (factory->Release()) {
		sAnimationFactory.erase(factory->fID);
		delete factory;
	}
}


AnimationFactory::AnimationFactory(const char* baseName, const uint16 id)
	:
	fBaseName(baseName),
	fID(id)
{
}


AnimationFactory::~AnimationFactory()
{
	std::map<std::pair<int, int>, Animation*>::const_iterator i;
	for (i = fAnimations.begin(); i != fAnimations.end(); i++)
		delete i->second;

	fAnimations.clear();
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

	std::cerr << "Missing animation for ";
	std::cerr << fBaseName << ", ID: ";
	std::cerr << std::hex << fID << std::endl;

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

