/*
 * BG2AnimationFactory.h
 *
 *  Created on: 16/nov/2014
 *      Author: Stefano Ceccherini
 */

#ifndef BG2ANIMATIONFACTORY_H_
#define BG2ANIMATIONFACTORY_H_

#include "AnimationFactory.h"

class BG2CharachterAnimationFactory: public AnimationFactory {
public:
	BG2CharachterAnimationFactory(const char* baseName, const uint16 id);
	~BG2CharachterAnimationFactory();

	Animation* AnimationFor(int action, int o);
};

#endif /* BG2ANIMATIONFACTORY_H_ */
