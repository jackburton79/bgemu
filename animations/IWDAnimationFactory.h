/*
 * IWDAnimationFactory.h
 *
 *  Created on: 16/nov/2014
 *      Author: Stefano Ceccherini
 */

#ifndef IWDANIMATIONFACTORY_H_
#define IWDANIMATIONFACTORY_H_

#include "AnimationFactory.h"

class IWDAnimationFactory: public AnimationFactory {
public:
	IWDAnimationFactory(const char* baseName, const uint16 id);
	~IWDAnimationFactory();

private:
	virtual animation_description GetAnimationDescription(int action, int o);
};

#endif /* IWDANIMATIONFACTORY_H_ */
