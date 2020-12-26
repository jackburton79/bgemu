/*
 * BGAnimationFactory.h
 *
 *  Created on: 16/nov/2014
 *      Author: Stefano Ceccherini
 */

#ifndef BGANIMATIONFACTORY_H_
#define BGANIMATIONFACTORY_H_

#include "AnimationFactory.h"

class BGCharachterAnimationFactory: public AnimationFactory {
public:
	BGCharachterAnimationFactory(const char* baseName, const uint16 id);
	~BGCharachterAnimationFactory();

private:
	virtual animation_description GetAnimationDescription(int action, int o);
};

#endif /* BGANIMATIONFACTORY_H_ */
