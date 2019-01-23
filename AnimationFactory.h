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

class Animation;
class AnimationFactory : public Referenceable {
public:
	static AnimationFactory* GetFactory(const uint16 id);
	static void ReleaseFactory(AnimationFactory*);

	virtual Animation* AnimationFor(int action, int orientation) = 0;

protected:
	AnimationFactory(const char* baseName, const uint16 id);
	virtual ~AnimationFactory();

	Animation* InstantiateAnimation(
							const animation_description& description);

	const char* _GetBamName(const char* attributes) const;
	
	std::string fBaseName;
	uint16 fID;

	static std::map<uint16, AnimationFactory*> sAnimationFactory;
};

#endif /* ANIMATIONFACTORY_H_ */
