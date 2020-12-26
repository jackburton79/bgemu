/*
 * AnimationFactory.h
 *
 *  Created on: 20/mag/2013
 *      Author: stefano
 */

#ifndef __ANIMATIONFACTORY_H_
#define __ANIMATIONFACTORY_H_

#include <map>
#include <string>
#include <vector>

#include "IETypes.h"
#include "Referenceable.h"

struct animation_description {
	std::string bam_name;
	int sequence_number;
	bool mirror;
};

struct CREColors;
class Actor;
class Animation;
class AnimationFactory : public Referenceable {
public:
	static AnimationFactory* GetFactory(const uint16 id);
	static void ReleaseFactory(AnimationFactory*);

	Animation* AnimationFor(Actor* actor, CREColors* colors = NULL);
	
protected:
	AnimationFactory(const char* baseName, const uint16 id);
	virtual ~AnimationFactory();

	virtual animation_description GetAnimationDescription(int action, int orientation) = 0;
	
	const char* _GetBamName(const char* attributes) const;

	static std::map<uint16, AnimationFactory*> sAnimationFactory;

	std::string fBaseName;
	uint16 fID;
};

#endif /* ANIMATIONFACTORY_H_ */
