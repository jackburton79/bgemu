/*
 * SplitAnimationFactory.h
 *
 *  Created on: 19/nov/2014
 *      Author: stefano
 */

#ifndef SPLITANIMATIONFACTORY_H_
#define SPLITANIMATIONFACTORY_H_

#include "AnimationFactory.h"

class SplitAnimationFactory: public AnimationFactory {
public:
	SplitAnimationFactory(const char* baseName, const uint16 id);
	~SplitAnimationFactory();

	virtual Animation* AnimationFor(int action, int o);
};

#endif /* SPLITANIMATIONFACTORY_H_ */
