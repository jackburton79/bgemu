/*
 * SimpleAnimationFactory.h
 *
 *  Created on: 18/nov/2014
 *      Author: stefano
 */

#ifndef SIMPLEANIMATIONFACTORY_H_
#define SIMPLEANIMATIONFACTORY_H_

#include "AnimationFactory.h"

class SimpleAnimationFactory: public AnimationFactory {
public:
	SimpleAnimationFactory(const char* baseName);
	~SimpleAnimationFactory();

	Animation* AnimationFor(int action, int o);
};

#endif /* SIMPLEANIMATIONFACTORY_H_ */
