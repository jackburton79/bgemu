/*
 * AnimationFactory.h
 *
 *  Created on: 20/mag/2013
 *      Author: stefano
 */

#ifndef __ANIMATIONFACTORY_H_
#define __ANIMATIONFACTORY_H_

#include <string>

#include "IETypes.h"

#define ANIM_STANDING_OFFSET 9
#define ANIM_DIE_OFFSET 45
#define ANIM_DEAD_OFFSET 54

struct animation_description {
	std::string bam_name;
	int sequence_number = 0;
	bool mirror = false;
	bool custom_colors = false;
};

struct CREColors;
class Actor;
class Animation;
class AnimationFactory {
public:
	// Builds the factory for an actor's animation id (anisnd.ids). The
	// caller owns it - hand it back to ReleaseFactory() when done. Not
	// shared/cached: the object is trivially cheap (a name + an id), the
	// expensive part is Animation's own BAM decode.
	static AnimationFactory* GetFactory(uint16 id);
	static void ReleaseFactory(AnimationFactory*);

	Animation* AnimationFor(Actor* actor, CREColors* colors = NULL);

	// The paperdoll resref (a PLT, see resources/PLTResource.h) shown on
	// the inventory screen for this actor - the same "C" + Race + Gender
	// + Class + Armour identity prefix AnimationFor() builds for a party
	// member's own sprite, with "INV" instead of an action/orientation
	// suffix (see avatarnaming.htm's "PLT Files" paragraph). Falls back to
	// this factory's own base name + "1" for a non-party actor.
	std::string PaperdollName(const Actor* actor) const;

protected:
	AnimationFactory(const char* baseName, const uint16 id);
	~AnimationFactory();

	// Resolves fID to a per-style builder via the .cpp's dispatch table.
	animation_description GetAnimationDescription(Actor* actor);

private:
	std::string fBaseName;
	uint16 fID;
};

#endif /* ANIMATIONFACTORY_H_ */
