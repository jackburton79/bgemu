/*
 * BGMonsterAnimationFactory.h
 *
 *  Created on: 16/nov/2014
 *      Author: Stefano Ceccherini
 */

#ifndef BGMONSTERANIMATIONFACTORY_H_
#define BGMONSTERANIMATIONFACTORY_H_

#include "AnimationFactory.h"

class BGMonsterAnimationFactory: public AnimationFactory {
public:
	BGMonsterAnimationFactory(const char* baseName, const uint16 id);
	~BGMonsterAnimationFactory();

private:
	virtual animation_description GetAnimationDescription(int action, int o);
};

#endif /* BGANIMATIONFACTORY_H_ */
