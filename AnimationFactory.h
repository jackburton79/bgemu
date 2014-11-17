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

	virtual Animation* AnimationFor(int action, IE::orientation o);

protected:
	AnimationFactory(const char* baseName);
	AnimationFactory(const uint16 id);
	virtual ~AnimationFactory();

	Animation* InstantiateAnimation(
							const animation_description description,
							const std::pair<int, IE::orientation> key);

	animation_description CharachterAnimationFor(int action,
													IE::orientation o);
	animation_description MonsterAnimationFor(int action,
												IE::orientation o);
	animation_description BG2AnimationFor(int action,
												IE::orientation o);
	bool _HasEastBams() const;
	bool _AreHighLowSplitted() const;
	bool _HasStandingSequence() const;
	void _ClassifyAnimation();

	const char* _GetBamName(const char* attributes) const;
	
	bool _HasAnimation(const std::string& name) const;
	
	std::vector<std::string> fList;
	std::string fBaseName;
	uint16 fID;
	int fAnimationType;
	bool fHighLowSplitted;
	bool fEastAnimations;

	// Animations are kept cached here
	std::map<std::pair<int, IE::orientation>, Animation*> fAnimations;

	static std::map<std::string, AnimationFactory*> sAnimationFactory;
};

#endif /* ANIMATIONFACTORY_H_ */
