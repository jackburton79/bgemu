/*
 * AnimationFactory.cpp
 *
 *  Created on: 20/mag/2013
 *      Author: stefano
 */

#include "Animation.h"
#include "AnimationFactory.h"
#include "BamResource.h"
#include "ResManager.h"

#include <string>
#include <vector>

enum animation_type {
	ANIMATION_TYPE_CHARACHTER,
	ANIMATION_TYPE_BG1_MONSTER,
	ANIMATION_TYPE_BOH
};

std::map<std::string, AnimationFactory*> AnimationFactory::sAnimationFactory;

const int kStandingOffset = 10;


/* static */
AnimationFactory*
AnimationFactory::GetFactory(const char* baseName)
{
	AnimationFactory* factory = NULL;
	std::map<std::string, AnimationFactory*>::const_iterator i
		= sAnimationFactory.find(baseName);
	if (i == sAnimationFactory.end())
		factory = new AnimationFactory(baseName);
	else
		factory = i->second;

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
	:
	fAnimationType(0),
	fHighLowSplitted(false),
	fEastAnimations(false)
{
	strcpy(fBaseName, baseName);

	gResManager->GetResourceList(fList, baseName, RES_BAM);

	_ClassifyAnimation();
}


AnimationFactory::~AnimationFactory()
{
	std::cout << "AnimationFactory::~AnimationFactory(): ";
	std::cout << fBaseName << std::endl;
	fAnimations.clear();
}


animation_description
AnimationFactory::CharachterAnimationFor(int action, IE::orientation o)
{
	animation_description description;
	description.bam_name = fBaseName;
	description.sequence_number = o;
	description.mirror = false;
	// Armor
	// TODO: For real
	description.bam_name.append("1");
	switch (action) {
		case ACT_WALKING:
		{
			if (_HasAnimation(description.bam_name + "W2")) {
				description.bam_name.append("W2");
				description.sequence_number = uint32(o);
			} else {
				description.bam_name.append("G11");
				description.sequence_number = uint32(o) + 8;
			}

			break;
		}
		case ACT_STANDING:
			description.bam_name.append("G1");
			description.sequence_number = uint32(o) + 8;
			break;
		default:
			break;
	}
	if (uint32(o) >= IE::ORIENTATION_NE && uint32(o) <= IE::ORIENTATION_SE) {
		if (_HasEastBams()) {
			description.bam_name.append("E");
			// TODO: Doesn't work for some animations (IE: ACOW)
			//sequenceNumber -= 1;
		} else {
			// Orientation 5 uses bitmap from orientation 3 mirrored,
			// 6 uses 2, and 7 uses 1
			description.mirror = true;
			description.sequence_number -= (uint32(o) - 4) * 2;
		}
	}
	return description;
}


animation_description
AnimationFactory::MonsterAnimationFor(int action, IE::orientation o)
{
	animation_description description;
	description.bam_name = fBaseName;
	description.sequence_number = o;
	description.mirror = false;

	// G1/G11-G15, G2/G21/26
	if (_AreHighLowSplitted()) {
		description.bam_name.append("H");
	}
	switch (action) {
		case ACT_WALKING:
			description.bam_name.append("G1");
			description.sequence_number = uint32(o);
			break;
		case ACT_ATTACKING:
			break;
		case ACT_STANDING:
			if (_HasAnimation(description.bam_name + "G1")) {
				description.bam_name.append("G1");
				description.sequence_number = uint32(o);
			} else {
				description.bam_name.append("C");
				description.sequence_number = uint32(o);
			}
			if (_HasStandingSequence())
				description.sequence_number += kStandingOffset;
			break;
		default:
			break;
	}
	if (uint32(o) >= IE::ORIENTATION_NE && uint32(o) <= IE::ORIENTATION_SE) {
		if (_HasEastBams()) {
			description.bam_name.append("E");
			// TODO: Doesn't work for some animations (IE: ACOW)
			//sequence_number -= 5;
		} else {
			// Orientation 5 uses bitmap from orientation 3 mirrored,
			// 6 uses 2, and 7 uses 1
			description.mirror = true;
			description.sequence_number -= (uint32(o) - 4) * 2;
		}
	}

	return description;
}


Animation*
AnimationFactory::AnimationFor(int action, IE::orientation o)
{
	// Check if animation was already loaded
	std::pair<int, IE::orientation> key = std::make_pair(action, o);
	std::map<std::pair<int, IE::orientation>, Animation*>::const_iterator i;
	i = fAnimations.find(key);
	if (i != fAnimations.end())
		return i->second;

	animation_description description;
	// TODO: Only valid for G1/G11/E files
	switch (fAnimationType) {
		case ANIMATION_TYPE_BG1_MONSTER:
		{
			description = MonsterAnimationFor(action, o);
			break;
		}
		case ANIMATION_TYPE_CHARACHTER:
		{
			description = CharachterAnimationFor(action, o);
			break;
		}
		default:
			return NULL;
			break;
	}

	IE::point pos;
	Animation* animation = NULL;
	try {
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


bool
AnimationFactory::_HasEastBams() const
{
	return fEastAnimations;
}


bool
AnimationFactory::_AreHighLowSplitted() const
{
	return fHighLowSplitted;
}


bool
AnimationFactory::_HasStandingSequence() const
{
	// TODO: Don't just compare with a fixed list,
	// find out a rule
	return strcasecmp(fBaseName, "ACOW");
}


void
AnimationFactory::_ClassifyAnimation()
{
	if (fList.size() > 13)
		fAnimationType = ANIMATION_TYPE_CHARACHTER;
	else
		fAnimationType = ANIMATION_TYPE_BG1_MONSTER;

	std::vector<std::string>::const_iterator i;
	for (i = fList.begin(); i != fList.end(); i++) {
		if (i->at(i->size() - 1) == 'E') {
			fEastAnimations = true;
			break;
		}
	}

	bool foundLow = false;
	bool foundHigh = false;
	try {
		for (i = fList.begin(); i != fList.end(); i++) {
			char c = (*i).at(4);
			if (c == 'L') {
				foundLow = true;
			} else if (c == 'H')
				foundHigh = true;

			if (foundHigh && foundLow)
				break;
		}
	} catch (...) {

	}
	fHighLowSplitted = foundHigh && foundLow;
}


bool
AnimationFactory::_HasAnimation(const std::string& name) const
{
    std::vector<std::string>::const_iterator i;
    for (i = fList.begin(); i != fList.end(); i++) {
		if (*i == name)
			return true;
    }
    return false;
}
