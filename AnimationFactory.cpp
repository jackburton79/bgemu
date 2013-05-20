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

AnimationFactory::AnimationFactory(const char* baseName)
{
	// TODO: Guess the animation type

	gResManager->GetResourceList(fList, baseName, RES_BAM);
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
	switch (action) {
		case ACT_WALKING:
			bamName.append(fList[0]);
			sequenceNumber = uint32(o);
			break;
		default:
			break;

	}
	IE::point pos;
	bool mirror = false;
	if (uint32(o) >= IE::ORIENTATION_NE && uint32(o) <= IE::ORIENTATION_SE) {
		if (!_HasEastBams()) {
			mirror = true;
			sequenceNumber -= 5;
		} else
			bamName = fList[1];
	}

	Animation* animation = new Animation(bamName.c_str(), sequenceNumber, pos);
	if (mirror)
		animation->SetMirrored(true);
	return animation;
}


bool
AnimationFactory::_HasEastBams() const
{
	std::vector<std::string>::const_iterator i;
	for (i = fList.begin(); i != fList.end(); i++) {
		if (i->at(i->size() - 1) == 'E')
			return true;
	}
	return false;
}
