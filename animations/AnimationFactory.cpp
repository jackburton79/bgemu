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
#include "BGMonsterAnimationFactory.h"
#include "BamResource.h"
#include "Core.h"
#include "IWDAnimationFactory.h"
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
	// 0xc100: ACAT: Simple
	// 0xc700: ABOY: Split
	// 0x7400: MDOG: Simple
	// 0x7e00: MWER, BG2Monster
	uint8 highId = animationID >> 8;

	std::string baseName = IDTable::AniSndAt(animationID);
#if 1
	std::cout << "AnimationFactory::GetFactory(";
	std::cout << baseName << ", " << std::hex;
	std::cout << animationID << ")" << std::endl;
#endif
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
				if (highId == 0x20 || highId == 0x23
						|| highId == 0x74 || highId == 0xb0
						|| (highId >= 0xc1 && highId <= 0xc4)
						|| (highId >= 0xd1 && highId <= 0xd3))
					factory = new SimpleAnimationFactory(baseName.c_str(), animationID);
				else if (highId == 0x7e || highId == 0x7f || highId == 0x73)
					factory = new BGMonsterAnimationFactory(baseName.c_str(), animationID);
				else if (highId >= 0x50 && highId <= 0x90 )
					factory = new BG2CharachterAnimationFactory(baseName.c_str(), animationID);
				else if (highId == 0xb4 || highId == 0xb5
						|| (highId >= 0xc6 && highId <= 0xca))
					factory = new SplitAnimationFactory(baseName.c_str(), animationID);
				else if (highId == 0xe0 || highId == 0xe4 || highId == 0xe6)
					factory = new IWDAnimationFactory(baseName.c_str(), animationID);
				break;

			default:
				break;
		}
	}

	if (factory != NULL)
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
}


Animation*
AnimationFactory::AnimationFor(int action, int orientation, CREColors* colors)
{
	animation_description description;
	GetAnimationDescription(action, orientation, description);
	
	Animation* animation = NULL;
	try {
		IE::point pos;
		animation = new Animation(description.bam_name.c_str(),
								description.sequence_number, description.mirror, pos, colors);
	} catch (...) {
		animation = NULL;
	}
#if 0
	std::cout << std::hex << fID << " " << description.bam_name << std::endl;
#endif
	return animation;
}
