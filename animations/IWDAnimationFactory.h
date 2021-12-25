/*
 * IWDAnimationFactory.h
 *
 *  Created on: 16/nov/2014
 *      Author: Stefano Ceccherini
 */

#ifndef IWDANIMATIONFACTORY_H_
#define IWDANIMATIONFACTORY_H_

#include "AnimationFactory.h"

/*
 * IWD style
One optional weapon letter for weapon overlays
A: Axe
B: Bow
C: Club
D: Dagger
F: Flail
H: Halberd
M: Mace
Q: Quarterstaff
S: Sword
W: Warhammer
Actions:
GU - Get up (switches to the SC/SD sequence)
SD - Stand
SC - Stance (ready for attack)
GH - Get hit (switches to the SC/DE sequence)
DE - Die (switches to the TW sequence)
TW - Twitch (dead, still images most of the time). Played till the last frame, then freezes.
SP - Spell (After an arbitary number of loops, switches to CA)
CA - Cast (Ending sequence of spell casting) (switches to SC/SD)
SL - Sleep. Played till the last frame then freezes (May switch to the GU sequence).
WK - Walk
A1 - Attack
A2 - Attack
A4 - Attack

Convention:
[Animation ID][(optional) Single-letter weapon ID][Sequence ID][(optional) E for +180 - +360 degrees]
Example: The armored skeleton animation MSKAAGU.BAM (battle axe, Get Up), MSKAMA1.BAM (morning star, attack-slash).
 */
class IWDAnimationFactory: public AnimationFactory {
public:
	IWDAnimationFactory(const char* baseName, const uint16 id);
	~IWDAnimationFactory();

private:
	virtual animation_description GetAnimationDescription(Actor* actor);
};

#endif /* IWDANIMATIONFACTORY_H_ */
