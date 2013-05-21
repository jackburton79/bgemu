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
	ANIMATION_TYPE_OTHER
};


std::map<std::string, AnimationFactory*> AnimationFactory::sAnimationFactory;

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

	return factory;
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

}


Animation*
AnimationFactory::AnimationFor(int action, IE::orientation o)
{

	// TODO: Only valid for G1/G11/E files
	std::string bamName;
	int sequenceNumber = 0;

	bamName.append(fBaseName);

	switch (fAnimationType) {
		case ANIMATION_TYPE_OTHER:
		{
			if (_AreHighLowSplitted()) {
				bamName.append("H");
			}
			switch (action) {
				case ACT_WALKING:
					bamName.append("G1");
					sequenceNumber += uint32(o);
				case ACT_ATTACKING:
					break;
				default:
					break;
			}
			break;
		}
		case ANIMATION_TYPE_CHARACHTER:
		{
			// Armor
			// TODO: For real
			bamName.append("1");
			switch (action) {
				case ACT_WALKING:
					bamName.append("W2");
					sequenceNumber += uint32(o);
					break;
				default:
					break;
			}
			break;
		}
		default:
			break;
	}

	bool mirror = false;
	if (uint32(o) >= IE::ORIENTATION_NE && uint32(o) <= IE::ORIENTATION_SE) {
		if (!_HasEastBams()) {
			mirror = true;
			sequenceNumber -= 2;
		} else
			bamName.append("E");
	}

	std::cout << "Orientation: " << o << ", bam " << bamName << std::endl;
	IE::point pos;
	Animation* animation = new Animation(bamName.c_str(), sequenceNumber, pos);
	if (mirror)
		animation->SetMirrored(true);
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


void
AnimationFactory::_ClassifyAnimation()
{
	if (fList.size() > 6)
		fAnimationType = ANIMATION_TYPE_CHARACHTER;
	else
		fAnimationType = ANIMATION_TYPE_OTHER;

	// TODO: Guess the animation type
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
