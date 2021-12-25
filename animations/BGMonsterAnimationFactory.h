/*
 * BGMonsterAnimationFactory.h
 *
 *  Created on: 16/nov/2014
 *      Author: Stefano Ceccherini
 */

/*
 * BG1 monster style
These have no secondary files for mirrors. There are 9 orientations for each action, the 7 eastern orientations are generated. All animations are 'G<number>'
Actions:
G1 = Stand combat? (sequences 10-18, min 16 frames)
G11 = Walk (sequences 1-9, min 8 frames)
G12 = Stand Peaceful?
G13 = Hit?
G14 = Hit?
G15 = Twitch (dead)
G2 = Attack
G21 = Attack
G22 = Attack
G23 = Ranged Attack?
G24 = Same as G23
G25 = Casting? Build-up of Spell?
G26 = Release Spell?

Convention:
[AnimationID]Gn[(optional) Digit)][(optional) Two-letter weapon ID]
Example: The rakshasa animation MRAKG2.BAM (unarmed/creature), MRAKG2CB.BAM (crossbow).

There are also minor sub-types for birds/cow/carrion crawler/dragons etc.
 */
#ifndef BGMONSTERANIMATIONFACTORY_H_
#define BGMONSTERANIMATIONFACTORY_H_

#include "AnimationFactory.h"

class BGMonsterAnimationFactory: public AnimationFactory {
public:
	BGMonsterAnimationFactory(const char* baseName, const uint16 id);
	~BGMonsterAnimationFactory();

private:
	virtual animation_description GetAnimationDescription(Actor* actor);
};

#endif /* BGANIMATIONFACTORY_H_ */
