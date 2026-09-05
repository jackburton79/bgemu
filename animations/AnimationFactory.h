/*
 * AnimationFactory.h
 *
 *  Created on: 20/mag/2013
 *      Author: stefano
 */

#ifndef __ANIMATIONFACTORY_H_
#define __ANIMATIONFACTORY_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "IETypes.h"
#include "Referenceable.h"

#define ANIM_STANDING_OFFSET 9
#define ANIM_DIE_OFFSET 45
#define ANIM_DEAD_OFFSET 54

struct animation_description {
	std::string bam_name;
	int sequence_number;
	bool mirror;
	bool custom_colors;
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

	animation_description GetAnimationDescription(Actor* actor);
	animation_description _GetBGMonsterAnimationDescription(Actor* actor);
	animation_description _GetBGCharacterAnimationDescription(Actor* actor);
	animation_description _GetCharacterAnimationDescription(Actor* actor);
	animation_description _GetSimpleAnimationDescription(Actor* actor);
	animation_description _GetSplitAnimationDescription(Actor* actor);
	animation_description _GetIWDAnimationDescription(Actor* actor);
	animation_description _GetStaticAnimationDescription(Actor* actor);

	std::string BaseName() const;

	std::string _RaceCharacter(uint8 race) const;
	std::string _ClassCharacter(uint8 c) const;
	std::string _GenderCharacter(uint8 gender) const;
	std::string _ArmorCharacter(Actor* actor) const;

	bool _HasG11(const std::string& name) const;
	bool _HasG15(const std::string& name) const;
	bool _HasW(const std::string& name) const;
	bool _HasSeparateEasternOrientations(const std::string& name) const;

	static std::unordered_map<uint16, AnimationFactory*> sAnimationFactory;

private:
	std::string fBaseName;
	uint16 fID;
};

#endif /* ANIMATIONFACTORY_H_ */
