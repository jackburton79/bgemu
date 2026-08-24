/*
 * SimpleAnimationFactory.h
 *
 *  Created on: 19/nov/2014
 *      Author: stefano
 */

#pragma once

#include "AnimationFactory.h"

class SimpleAnimationFactory: public AnimationFactory {
public:
	SimpleAnimationFactory(const char* baseName, const uint16 id);
	~SimpleAnimationFactory();

private:
	virtual animation_description GetAnimationDescription(Actor* actor);
};
