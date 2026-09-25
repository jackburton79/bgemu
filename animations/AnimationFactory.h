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

	// Empty when this action has no weapon held up for the current
	// action (see AnimationFactory::WeaponOverlayFor()) - shares
	// sequence_number/mirror with the body above, since a weapon
	// overlay is always drawn frame-synced to it.
	std::string weapon_bam_name;
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

	// Whether the CRE's color bytes recolor the sprite of this animation id
	// (false: it is drawn with the BAM's own palette).
	static bool UsesCustomColors(uint16 animationID);

	// The equipped weapon's own overlay animation, drawn on top of
	// AnimationFor()'s body sprite at the same position - NULL when the
	// current action doesn't show one (e.g. no weapon equipped, or this
	// avatar style bakes the weapon into the body itself already - see
	// GetAnimationDescription()'s per-style builders). Never tinted with
	// the actor's own CRE colors (weapon BAMs aren't paletted that way).
	Animation* WeaponOverlayFor(Actor* actor);

	// The paperdoll resref (a PLT, see resources/PLTResource.h) shown on
	// the inventory screen for this actor - the same "C" + Race + Gender
	// + Class + Armour identity prefix AnimationFor() builds for a party
	// member's own sprite, with "INV" instead of an action/orientation
	// suffix (see avatarnaming.htm's "PLT Files" paragraph). Falls back to
	// this factory's own base name + "1" for a non-party actor.
	std::string PaperdollName(const Actor* actor) const;
	// BG1's paperdoll of an animation (unarmored) and its size code, for a
	// character that isn't an Actor yet; false if there is none.
	static bool PaperdollForAnimationBG1(uint16 animationID, std::string& name,
		std::string& sizeCode);

	// The wearer's body-size letter ("H"/"S"/"M") used by every "WP" +
	// size + animation-code (+ suffix) overlay resref - both the
	// in-world weapon overlay above and the inventory paperdoll's own
	// weapon/shield compositing (Game::_CompositePaperdollOverlay()).
	static const char* SizeCodeForActor(const Actor* actor);

	// The size letter of this actor's paperdoll overlays: BG1 has it per
	// animation id (empty for a doll that takes no overlays), otherwise it is
	// SizeCodeForActor().
	std::string PaperdollSizeCode(const Actor* actor) const;

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
