/*
 * AnimationFactory.cpp
 *
 *  Created on: 20/mag/2013
 *      Author: stefano
 */

#include "AnimationFactory.h"

#include "Animation.h"
#include "BamResource.h"
#include "BGCharachterAnimationFactory.h"
#include "BG2CharachterAnimationFactory.h"
#include "BGMonsterAnimationFactory.h"
#include "Core.h"
#include "IWDAnimationFactory.h"
#include "Log.h"
#include "ResManager.h"
#include "SplitAnimationFactory.h"

#include <cxxabi.h>
#include <string>
#include <typeinfo>
#include <vector>


std::map<uint16, AnimationFactory*> AnimationFactory::sAnimationFactory;

const int kStandingOffset = 10;


/* static */
AnimationFactory*
AnimationFactory::GetFactory(uint16 animationID)
{
	uint8 highId = animationID >> 8;

	std::string baseName = IDTable::AniSndAt(animationID);
#if 0
	std::cout << "AnimationFactory::GetFactory(";
	std::cout << baseName << ", " << std::hex;
	std::cout << "0x" << animationID << ")" << std::endl;
#endif
	AnimationFactory* factory = NULL;
	std::map<uint16, AnimationFactory*>::const_iterator i
		= sAnimationFactory.find(animationID);
	if (i != sAnimationFactory.end())
		factory = i->second;
	else {
		switch (Core::Get()->Game()) {
			case GAME_BALDURSGATE:
			case GAME_BALDURSGATE2:
				switch (highId) {
					case 0x70:
						factory = new BGCharachterAnimationFactory(baseName.c_str(), animationID);
						break;
					case 0x10:
					case 0x20:
					case 0x23:
					case 0x74:
					case 0x7b: // MWLF
					case 0x7d: // MZOM (Zombie)
					case 0x7e:
					case 0x7f:
					case 0x73:
					case 0x77:	// MSHD 0x7703
					case 0x80:
					case 0x81:
					case 0x90:
					case 0xa0:
					case 0xb0:
					case 0xc1:
					case 0xc2:
					case 0xc3:
					case 0xc4:
					case 0xc5:
					case 0xd0: // AEAG (Eagle)
					case 0xd1:
					case 0xd2:
					case 0xd3:
						factory = new BGMonsterAnimationFactory(baseName.c_str(), animationID);
						break;
					// WRONG
					case 0xb4:
					case 0xb5:
					case 0xc6:
					case 0xc7:
					case 0xc8:
					case 0xc9:
					case 0xca:
						factory = new SplitAnimationFactory(baseName.c_str(), animationID);
						break;
					case 0xe0:
					case 0xe4:
					case 0xe6:
						factory = new IWDAnimationFactory(baseName.c_str(), animationID);
						break;
					case 0x50:
					case 0x51:
					case 0x52:
					case 0x60:
					case 0x61:
					case 0x62:
					case 0x63:
					case 0x64:
					case 0x65:
						factory = new BG2CharachterAnimationFactory(baseName.c_str(), animationID);
						break;
					default:
						break;
				}
				break;
			default:
				break;
		}
	}

	if (factory != NULL) {
#if 0
		int status;
		char* demangled = abi::__cxa_demangle(typeid(*factory).name(), 0, 0, &status);
		std::string name = demangled;
		free(demangled);
		std::cout << "instantiate factory " << name << std::endl;
#endif
		factory->Acquire();
	} else {
		std::cerr << Log::Red << "No animation factory " << baseName;
		std::cerr << " (0x" << std::hex << animationID << ")" << Log::Normal << std::endl;
	}
	return factory;
}


/* static */
void
AnimationFactory::ReleaseFactory(AnimationFactory* factory)
{
	if (factory->Release()) {
		sAnimationFactory.erase(factory->fID);
		delete factory;
	}
}


AnimationFactory::AnimationFactory(const char* baseName, const uint16 id)
	:
	fBaseName(baseName),
	fID(id)
{
}


AnimationFactory::~AnimationFactory()
{
}


Animation*
AnimationFactory::AnimationFor(Actor* actor, CREColors* colors)
{
	animation_description description = GetAnimationDescription(actor);
	
	Animation* animation = NULL;
	try {
		IE::point pos;
		animation = new Animation(description.bam_name.c_str(),
								description.sequence_number, description.mirror, pos, colors);
	} catch (...) {
		animation = NULL;
	}
#if 0
	std::cout << std::hex << fID << " " << description.bam_name << std::endl;
#endif
	return animation;
}


std::string
AnimationFactory::BaseName() const
{
	return fBaseName;
}


std::string
AnimationFactory::_RaceCharacter(uint8 race) const
{
	switch (race) {
		case 1: // HUMAN
		case 7: // HALFORC
			return "H";
		case 2: // ELF
			return "E";
		case 3: // HALF_ELF
			return "H";
		case 4: // DWARF
		case 6: // GNOME
			return "D";
		case 5: // HALFLING
			return "I";
		default:
			return "Z";
	}
}


std::string
AnimationFactory::_ClassCharacter(uint8 c) const
{
	switch (c) {
		case 1: // MAGE
		case 11:
			return "W";
		case 2: // FIGHTER
		case 6: // PALADIN
		case 7: // FIGHTER_MAGE
		case 8: // FIGHTER_CLERIC
		case 9: // FIGHTER_THIEF
			return "F";
		case 3: // CLERIC
			return "C";
		case 4: // THIEF
		case 5: // BARD
			return "T";
		default:
			// if class is not one of those, use the basename
			return BaseName().substr(3, 1);
			//return "B"; In BG there is no B type
	}
}


std::string
AnimationFactory::_GenderCharacter(uint8 gender) const
{
	switch (gender) {
		case 2:
			return "F";
		case 1:
		default:
			return "M";
	}
}


std::string
AnimationFactory::_ArmorCharacter(Actor* actor) const
{
	std::string armor = actor->ArmorAnimation();
	return armor.substr(0, 1);
}


bool
AnimationFactory::_HasG11(const std::string& name) const
{
	std::string walkingBam = name;
	walkingBam.append("G11");
	return gResManager->ResourceExists(walkingBam.c_str(), RES_BAM);
}


bool
AnimationFactory::_HasG15(const std::string& name) const
{
	std::string walkingBam = name;
	walkingBam.append("G15");
	return gResManager->ResourceExists(walkingBam.c_str(), RES_BAM);
}


bool
AnimationFactory::_HasW(const std::string& name) const
{
	std::string walkingBam = name;
	walkingBam.append("W2");
	return gResManager->ResourceExists(walkingBam.c_str(), RES_BAM);
}


bool
AnimationFactory::_HasSeparateEasternOrientations(const std::string& name) const
{
	std::string easternFacing = name;
	easternFacing.append("E");
	return gResManager->ResourceExists(easternFacing.c_str(), RES_BAM);
}
