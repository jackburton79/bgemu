/*
 * BGAnimationFactory.h
 *
 *  Created on: 16/nov/2014
 *      Author: Stefano Ceccherini
 */

#ifndef BGANIMATIONFACTORY_H_
#define BGANIMATIONFACTORY_H_

#include "AnimationFactory.h"
/*
 * BG1 character style
These animations have [E] files for eastern orientations. There are only 8 orientations of which 5 are in the first file, 3 are in the second file. Both files have the full 8 orientation slots, but only 5/3 are useful.

Convention:
[Animation ID][(optional) Single-letter weapon ID]Gn[(optional) E for +180 - +360 degrees]
Example: The sirine animation MSIRG1.BAM (unarmed/creature, "left" animation), MSIRBG1E.BAM (bow, "right" animation).
 */
class BGCharachterAnimationFactory: public AnimationFactory {
public:
	BGCharachterAnimationFactory(const char* baseName, const uint16 id);
	~BGCharachterAnimationFactory();

private:
	virtual animation_description GetAnimationDescription(Actor* actor);
};

#endif /* BGANIMATIONFACTORY_H_ */
