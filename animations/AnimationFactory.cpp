/*
 * AnimationFactory.cpp
 *
 *  Created on: 20/mag/2013
 *      Author: stefano
 */

#include "AnimationFactory.h"

#include "Animation.h"
#include "BGCharachterAnimationFactory.h"
#include "BG2CharachterAnimationFactory.h"
#include "BGMonsterAnimationFactory.h"
#include "Core.h"
#include "IWDAnimationFactory.h"
#include "Log.h"
#include "ResManager.h"
#include "SimpleAnimationFactory.h"
#include "SplitAnimationFactory.h"

#include <algorithm>
#include <cxxabi.h>
#include <string>


std::unordered_map<uint16, AnimationFactory*> AnimationFactory::sAnimationFactory;

const int kStandingOffset = 10;

enum class FactoryType {
	CharacterAnimationBG,
	CharacterAnimationBG2,
	MonsterAnimation,
	SplitAnimation,
	SimpleAnimation,
	IWD
};


struct AnimationDescriptor {
	uint16 animation_id;
	std::string base_name;
	FactoryType animation_type;
};


const static AnimationDescriptor kAnimationEntries[] = {
	{ 0x1000, "", FactoryType::MonsterAnimation },
	{ 0x2000, "", FactoryType::MonsterAnimation },
	{ 0x2300, "", FactoryType::MonsterAnimation },

	{ 0x4000, "SNOM", FactoryType::SimpleAnimation },
	{ 0x4010, "SNOW", FactoryType::SimpleAnimation },
	{ 0x4101, "SSIM", FactoryType::SimpleAnimation },
	{ 0x4101, "SSIM", FactoryType::SimpleAnimation },

	{ 0x5000, "CHMB", FactoryType::CharacterAnimationBG2 },
	{ 0x5002, "CDMB", FactoryType::CharacterAnimationBG2 },
	{ 0x5003, "", FactoryType::CharacterAnimationBG2 },
	{ 0x5100, "", FactoryType::CharacterAnimationBG2 },
	{ 0x5102, "CDMB", FactoryType::CharacterAnimationBG2 },
	{ 0x5110, "CHFB", FactoryType::CharacterAnimationBG2 },
	{ 0x5113, "CIFB", FactoryType::CharacterAnimationBG2 },
	{ 0x5200, "", FactoryType::CharacterAnimationBG2 },
	{ 0x5210, "CHFW", FactoryType::CharacterAnimationBG2 },
	{ 0x5303, "CIMB", FactoryType::CharacterAnimationBG2 },
	{ 0x6000, "CHMB", FactoryType::CharacterAnimationBG2 },
	{ 0x6002, "CDMB", FactoryType::CharacterAnimationBG2 },
	{ 0x6003, "CIMB", FactoryType::CharacterAnimationBG2 }, // CIMB
	{ 0x6004, "CDMB", FactoryType::CharacterAnimationBG2 }, // CDMB
	{ 0x6010, "CHFB", FactoryType::CharacterAnimationBG2 }, // CHFB
	{ 0x6011, "CEFB", FactoryType::CharacterAnimationBG2 }, // CEFB
	{ 0x6013, "CIFB", FactoryType::CharacterAnimationBG2 }, // CIFB
	{ 0x6100, "", FactoryType::CharacterAnimationBG2 },
	{ 0x6101, "CEMB", FactoryType::CharacterAnimationBG2 }, // CEMB
	{ 0x6102, "CDMB", FactoryType::CharacterAnimationBG2 }, // CDMB
	{ 0x6103, "CIMB", FactoryType::CharacterAnimationBG2 }, // CIMB
	{ 0x6104, "CDMB", FactoryType::CharacterAnimationBG2 }, // CDMB
	{ 0x6110, "CHFB", FactoryType::CharacterAnimationBG2 },
	{ 0x6111, "CEFB", FactoryType::CharacterAnimationBG2 },
	{ 0x6113, "CIFB", FactoryType::CharacterAnimationBG2 },
	{ 0x6200, "", FactoryType::CharacterAnimationBG2 },
	{ 0x6201, "CEMW", FactoryType::CharacterAnimationBG2 },
	{ 0x6210, "CHFW", FactoryType::CharacterAnimationBG2 },
	{ 0x6300, "", FactoryType::CharacterAnimationBG2 },
	{ 0x6301, "CEMB", FactoryType::CharacterAnimationBG2 },
	{ 0x6302, "CDMB", FactoryType::CharacterAnimationBG2 },
	{ 0x6303, "CIMB", FactoryType::CharacterAnimationBG2 },
	{ 0x6310, "CHFB", FactoryType::CharacterAnimationBG2 },
	{ 0x6311, "CEFB", FactoryType::CharacterAnimationBG2 },
	{ 0x6314, "CEFB", FactoryType::CharacterAnimationBG2 },
	{ 0x6315, "CEFB", FactoryType::CharacterAnimationBG2 },
	{ 0x6400, "", FactoryType::CharacterAnimationBG2 },
	{ 0x6403, "MSKL", FactoryType::CharacterAnimationBG2 },
	{ 0x6500, "", FactoryType::CharacterAnimationBG2 },

	{ 0x7000, "", FactoryType::CharacterAnimationBG },
	{ 0x7300, "", FactoryType::MonsterAnimation },
	{ 0x7400, "", FactoryType::MonsterAnimation },
	{ 0x7703, "MSHD", FactoryType::MonsterAnimation },
	{ 0x7c01, "MTAS", FactoryType::MonsterAnimation },
	{ 0x7b00, "MWLF", FactoryType::MonsterAnimation },
	{ 0x7b02, "MWLF", FactoryType::MonsterAnimation },
	{ 0x7d00, "MZOM", FactoryType::MonsterAnimation }, // (Zombie)
	{ 0x7e00, "", FactoryType::MonsterAnimation },
	{ 0x7f05, "MDJI", FactoryType::MonsterAnimation },
	{ 0x7f08, "MOTY", FactoryType::MonsterAnimation },
	{ 0x7f0b, "MGCL", FactoryType::MonsterAnimation },
	{ 0x7f0d, "MLIC", FactoryType::MonsterAnimation },
	{ 0x7f10, "MRAK", FactoryType::MonsterAnimation },
	{ 0x7f13, "MSNK", FactoryType::MonsterAnimation },
	{ 0x7f16, "AMOO", FactoryType::MonsterAnimation },
	{ 0x7f17, "ARAB", FactoryType::MonsterAnimation },
	{ 0x7f18, "ADER", FactoryType::MonsterAnimation },
	{ 0x7f20, "AGRO", FactoryType::MonsterAnimation },
	{ 0x7f21, "APHE", FactoryType::MonsterAnimation },
	{ 0x7f24, "NPIR", FactoryType::MonsterAnimation },
	{ 0x7f2a, "NSAI", FactoryType::MonsterAnimation },
	{ 0x7f2c, "NSOL", FactoryType::MonsterAnimation },
	{ 0x7f36, "NSHD", FactoryType::MonsterAnimation },
	{ 0x8000, "", FactoryType::MonsterAnimation },
	{ 0x8100, "", FactoryType::MonsterAnimation },
	{ 0x9000, "", FactoryType::MonsterAnimation },
	{ 0xa000, "", FactoryType::MonsterAnimation },
	{ 0xb000, "", FactoryType::MonsterAnimation },
	{ 0xb100, "AHRS", FactoryType::MonsterAnimation }, // AHRS
	{ 0xb200, "NBEG", FactoryType::SplitAnimation }, // NBEG 0xb200
	{ 0xb400, "", FactoryType::SplitAnimation },
	{ 0xb410, "NFAW", FactoryType::SplitAnimation }, // NFAW
	{ 0xb500, "", FactoryType::SplitAnimation },
	{ 0xb510, "NSIW", FactoryType::SplitAnimation }, // NSIW
	{ 0xc100, "", FactoryType::MonsterAnimation },
	{ 0xc200, "", FactoryType::MonsterAnimation },
	{ 0xc300, "", FactoryType::MonsterAnimation },
	{ 0xc400, "", FactoryType::MonsterAnimation },
	{ 0xc500, "", FactoryType::MonsterAnimation },
	{ 0xc700, "NBOY", FactoryType::SplitAnimation }, // NBOY
	{ 0xc800, "NFAM", FactoryType::SplitAnimation }, // NBOY
	{ 0xc600, "NBEG", FactoryType::SplitAnimation }, // NBEG
	{ 0xc610, "NPRO", FactoryType::SplitAnimation }, // NPRO
	{ 0xc710, "NGRL", FactoryType::SplitAnimation }, // NGRL
	{ 0xc800, "", FactoryType::SplitAnimation },
	{ 0xc810, "NFAW", FactoryType::SplitAnimation }, // NFAW
	{ 0xc900, "NSIM", FactoryType::SplitAnimation }, // NSIM
	{ 0xc910, "NSIW", FactoryType::SplitAnimation }, // NSIW
	{ 0xca00, "", FactoryType::SplitAnimation },
	{ 0xca10, "NNOW", FactoryType::SplitAnimation }, // NNOW
	{ 0xd000, "AEAG", FactoryType::MonsterAnimation }, // AEAG (Eagle)
	{ 0xd100, "", FactoryType::MonsterAnimation },
	{ 0xd200, "", FactoryType::MonsterAnimation },
	{ 0xd300, "", FactoryType::MonsterAnimation },
	{ 0xe000, "", FactoryType::IWD },
	{ 0xed00, "MYU1", FactoryType::IWD },
	{ 0xe400, "", FactoryType::IWD },
	{ 0xe600, "", FactoryType::IWD }
};

struct AnimationFinder {
	bool operator()(const AnimationDescriptor *A, const AnimationDescriptor* B) {
		return A->animation_id == B->animation_id;
	}
};

/* static */
AnimationFactory*
AnimationFactory::GetFactory(uint16 animationID)
{
	uint8 highID = animationID >> 8;
	uint8 lowID = animationID & 0xF;
	std::string baseName = IDTable::AniSndAt(animationID);
#if 1
	std::cout << "AnimationFactory::GetFactory(";
	std::cout << baseName << ", " << std::hex;
	std::cout << "0x" << animationID << ")";
	std::cout << " (0x" << (int)highID << ", 0x" << (int)lowID << ")" << std::endl;
#endif
	AnimationFactory* factory = NULL;
	auto i = sAnimationFactory.find(animationID);
	if (i != sAnimationFactory.end())
		factory = i->second;
	else {
		auto it = std::find_if(std::begin(kAnimationEntries), std::end(kAnimationEntries),
							[&] (const AnimationDescriptor entry) {
								return entry.animation_id == animationID;
							});
		if (it != std::end(kAnimationEntries)) {
			// Seems some animation aren't in the AniSnd file
			if (baseName == "")
				baseName = it->base_name;
			switch (it->animation_type) {
				case FactoryType::CharacterAnimationBG:
					factory = new BGCharachterAnimationFactory(baseName.c_str(), animationID);
					break;
				case FactoryType::CharacterAnimationBG2:
					factory = new BG2CharachterAnimationFactory(baseName.c_str(), animationID);
					break;
				case FactoryType::MonsterAnimation:
					factory = new BGMonsterAnimationFactory(baseName.c_str(), animationID);
					break;
				case FactoryType::SimpleAnimation:
					factory = new SimpleAnimationFactory(baseName.c_str(), animationID);
					break;
				case FactoryType::SplitAnimation:
					factory = new SplitAnimationFactory(baseName.c_str(), animationID);
					break;
				case FactoryType::IWD:
					factory = new IWDAnimationFactory(baseName.c_str(), animationID);
					break;
				default:
					break;
			}
		}
	}

	if (factory != NULL) {
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
								description.sequence_number, description.mirror,
								pos, description.custom_colors ? colors : nullptr);
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
