/*
 * AnimationFactory.cpp
 *
 *  Created on: 20/mag/2013
 *      Author: stefano
 */

#include "AnimationFactory.h"

#include "Animation.h"
#include "CreResource.h"
#include "Core.h"
#include "Log.h"
#include "ResManager.h"

#include <algorithm>
#include <cxxabi.h>
#include <sstream>
#include <string>


std::unordered_map<uint16, AnimationFactory*> AnimationFactory::sAnimationFactory;

const int kStandingOffset = 10;

enum class FactoryType {
	CharacterBG,
	CharacterBG2,
	Monster,
	SplitAnimation,
	SimpleAnimation,
	StaticAnimation,
	IWD
};


struct AnimationDescriptor {
	uint16 animation_id;
	std::string base_name;
	FactoryType animation_type;
};


const static AnimationDescriptor kAnimationEntries[] = {
	{ 0x1000, "", FactoryType::Monster },
	{ 0x2000, "", FactoryType::Monster },
	{ 0x2200, "MOGM", FactoryType::Monster },
	{ 0x2300, "", FactoryType::Monster },

	{ 0x4000, "SNOM", FactoryType::SimpleAnimation },
	{ 0x4010, "SNOW", FactoryType::SimpleAnimation },
	{ 0x4100, "SSIM", FactoryType::SimpleAnimation },
	{ 0x4101, "SSIM", FactoryType::SimpleAnimation },
	{ 0x4710, "LEFF", FactoryType::StaticAnimation },

	{ 0x5000, "CHMB", FactoryType::CharacterBG2 },
	{ 0x5002, "CDMB", FactoryType::CharacterBG2 },
	{ 0x5003, "", FactoryType::CharacterBG2 },
	{ 0x5100, "", FactoryType::CharacterBG2 },
	{ 0x5102, "CDMB", FactoryType::CharacterBG2 },
	{ 0x5110, "CHFB", FactoryType::CharacterBG2 },
	{ 0x5113, "CIFB", FactoryType::CharacterBG2 },
	{ 0x5200, "", FactoryType::CharacterBG2 },
	{ 0x5202, "CDMW", FactoryType::CharacterBG2 },
	{ 0x5210, "CHFW", FactoryType::CharacterBG2 },
	{ 0x5303, "CIMB", FactoryType::CharacterBG2 },
	{ 0x6000, "CHMB", FactoryType::CharacterBG2 },
	{ 0x6002, "CDMB", FactoryType::CharacterBG2 },
	{ 0x6003, "CIMB", FactoryType::CharacterBG2 }, // CIMB
	{ 0x6004, "CDMB", FactoryType::CharacterBG2 }, // CDMB
	{ 0x6010, "CHFB", FactoryType::CharacterBG2 }, // CHFB
	{ 0x6011, "CEFB", FactoryType::CharacterBG2 }, // CEFB
	{ 0x6013, "CIFB", FactoryType::CharacterBG2 }, // CIFB
	{ 0x6100, "CHMB", FactoryType::CharacterBG2 },
	{ 0x6101, "CEMB", FactoryType::CharacterBG2 }, // CEMB
	{ 0x6102, "CDMB", FactoryType::CharacterBG2 }, // CDMB
	{ 0x6103, "CIMB", FactoryType::CharacterBG2 }, // CIMB
	{ 0x6104, "CDMB", FactoryType::CharacterBG2 }, // CDMB
	{ 0x6110, "CHFB", FactoryType::CharacterBG2 },
	{ 0x6111, "CEFB", FactoryType::CharacterBG2 },
	{ 0x6113, "CIFB", FactoryType::CharacterBG2 },
	{ 0x6200, "", FactoryType::CharacterBG2 },
	{ 0x6201, "CEMW", FactoryType::CharacterBG2 },
	{ 0x6210, "CHFW", FactoryType::CharacterBG2 },
	{ 0x6211, "CEFW", FactoryType::CharacterBG2 },
	{ 0x6300, "", FactoryType::CharacterBG2 },
	{ 0x6301, "CEMB", FactoryType::CharacterBG2 },
	{ 0x6302, "CDMB", FactoryType::CharacterBG2 },
	{ 0x6303, "CIMB", FactoryType::CharacterBG2 },
	{ 0x6310, "CHFB", FactoryType::CharacterBG2 },
	{ 0x6311, "CEFB", FactoryType::CharacterBG2 },
	{ 0x6314, "CEFB", FactoryType::CharacterBG2 },
	{ 0x6315, "CEFB", FactoryType::CharacterBG2 },
	{ 0x6400, "", FactoryType::CharacterBG2 },
	{ 0x6402, "USAR", FactoryType::CharacterBG2 },
	{ 0x6403, "MSKL", FactoryType::CharacterBG2 },
	{ 0x6405, "MDGU", FactoryType::CharacterBG2 },
	{ 0x6500, "", FactoryType::CharacterBG2 },

	{ 0x7000, "", FactoryType::CharacterBG },
	{ 0x7001, "MOGR", FactoryType::Monster }, // WRONG
	{ 0x7300, "", FactoryType::Monster },
	{ 0x7400, "", FactoryType::Monster },
	{ 0x7703, "MSHD", FactoryType::Monster },
	{ 0x7c01, "MTAS", FactoryType::Monster },
	{ 0x7b00, "MWLF", FactoryType::Monster },
	{ 0x7b01, "MWLF", FactoryType::Monster },
	{ 0x7b02, "MWLF", FactoryType::Monster },
	{ 0x7d00, "MZOM", FactoryType::Monster }, // (Zombie)
	{ 0x7e00, "", FactoryType::Monster },
	{ 0x7f03, "MIMP", FactoryType::Monster },
	{ 0x7f05, "MDJI", FactoryType::Monster },
	{ 0x7f06, "MDJL", FactoryType::Monster },
	{ 0x7f07, "MGLC", FactoryType::Monster },
	{ 0x7f08, "MOTY", FactoryType::Monster },
	{ 0x7f0b, "MGCL", FactoryType::Monster },
	{ 0x7f0d, "MLIC", FactoryType::Monster },
	{ 0x7f10, "MRAK", FactoryType::Monster },
	{ 0x7f13, "MSNK", FactoryType::Monster },
	{ 0x7f16, "AMOO", FactoryType::Monster },
	{ 0x7f17, "ARAB", FactoryType::Monster },
	{ 0x7f18, "ADER", FactoryType::Monster },
	{ 0x7f20, "AGRO", FactoryType::Monster },
	{ 0x7f21, "APHE", FactoryType::Monster },
	{ 0x7f22, "MVAF", FactoryType::Monster },
	{ 0x7f24, "NPIR", FactoryType::Monster },
	{ 0x7f2a, "NSAI", FactoryType::Monster },
	{ 0x7f2c, "NSOL", FactoryType::Monster },
	{ 0x7f36, "NSHD", FactoryType::Monster },
	{ 0x7f37, "NIRE", FactoryType::Monster },
	{ 0x8000, "", FactoryType::Monster },
	{ 0x8100, "", FactoryType::Monster },
	{ 0x9000, "", FactoryType::Monster },
	{ 0xa000, "", FactoryType::Monster },
	{ 0xb000, "", FactoryType::Monster },
	{ 0xb100, "AHRS", FactoryType::Monster }, // AHRS
	{ 0xb200, "NBEG", FactoryType::SplitAnimation }, // NBEG 0xb200
	{ 0xb400, "", FactoryType::SplitAnimation },
	{ 0xb410, "NFAW", FactoryType::SplitAnimation }, // NFAW
	{ 0xb500, "", FactoryType::SplitAnimation },
	{ 0xb510, "NSIW", FactoryType::SplitAnimation }, // NSIW
	{ 0xc000, "ABAT", FactoryType::Monster },
	{ 0xc100, "", FactoryType::Monster },
	{ 0xc200, "", FactoryType::Monster },
	{ 0xc300, "", FactoryType::Monster },
	{ 0xc400, "", FactoryType::Monster },
	{ 0xc500, "", FactoryType::Monster },
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
	{ 0xd000, "AEAG", FactoryType::Monster }, // AEAG (Eagle)
	{ 0xd100, "", FactoryType::Monster },
	{ 0xd200, "", FactoryType::Monster },
	{ 0xd300, "", FactoryType::Monster },
	{ 0xe000, "", FactoryType::IWD },
	{ 0xe400, "", FactoryType::IWD },
	{ 0xe430, "MG04", FactoryType::IWD },
	{ 0xe600, "", FactoryType::IWD },
	{ 0xe710, "MNO2", FactoryType::IWD },
	{ 0xed00, "MYU1", FactoryType::IWD },
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
	/*auto i = sAnimationFactory.find(animationID);
	if (i != sAnimationFactory.end()) {
		factory = i->second;
		factory->Acquire();
	} else {*/
		factory = new AnimationFactory(baseName.c_str(), animationID);
		/*sAnimationFactory[animationID] = factory;*/
	//}
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


animation_description
AnimationFactory::GetAnimationDescription(Actor* actor)
{
	uint16 animationID = fID;
	std::string baseName = IDTable::AniSndAt(animationID);
#if 0
	uint8 highID = animationID >> 8;
	uint8 lowID = animationID & 0xF;
	std::cout << "AnimationFactory::GetAnimationDescription(";
	std::cout << baseName << ", " << std::hex;
	std::cout << "0x" << animationID << ")";
	std::cout << " (0x" << (int)highID << ", 0x" << (int)lowID << ")" << std::endl;
#endif

	auto it = std::find_if(std::begin(kAnimationEntries), std::end(kAnimationEntries),
						[&] (const AnimationDescriptor entry) {
							return entry.animation_id == animationID;
						});
	if (it == std::end(kAnimationEntries)) {
		std::string error("Animation description not found");
		std::ostringstream s;
		s << std::hex << animationID;
		error.append(" for ").append(baseName).append(" (0x").append(s.str()).append(")");
		throw std::runtime_error(error);
	}
		// Seems some animation aren't in the AniSnd file
	if (baseName == "")
		fBaseName = it->base_name;
	switch (it->animation_type) {
		case FactoryType::CharacterBG:
			return _GetBGCharacterAnimationDescription(actor);
		case FactoryType::CharacterBG2:
			return _GetBG2CharacterAnimationDescription(actor);
		case FactoryType::Monster:
			return _GetBGMonsterAnimationDescription(actor);
		case FactoryType::SimpleAnimation:
			return _GetSimpleAnimationDescription(actor);
		case FactoryType::SplitAnimation:
			return _GetSplitAnimationDescription(actor);
		case FactoryType::IWD:
			return _GetIWDAnimationDescription(actor);
		case FactoryType::StaticAnimation:
			return _GetStaticAnimationDescription(actor);
		default:
			break;
	}

	return animation_description();
}


std::string
AnimationFactory::BaseName() const
{
	return fBaseName;
}


animation_description
AnimationFactory::_GetBGMonsterAnimationDescription(Actor* actor)
{
	//std::cout << "BGAnimationFactory" << std::endl;
	int o = actor->Orientation();
	animation_description description;
	description.bam_name = BaseName();
	description.mirror = false;
	description.custom_colors = false;

	// TODO: Improve this
	if (Core::Get()->Game() == game::GAME_BALDURSGATE2) {
		if (o >= IE::ORIENTATION_EXT_NNE
				&& uint32(o) <= IE::ORIENTATION_EXT_SSE) {
			// Orientation 5 uses bitmap from orientation 3 mirrored,
			// 6 uses 2, and 7 uses 1
			description.mirror = true;
			o = 16 - o;
		}
	} else {
		if (o >= IE::ORIENTATION_NE && uint32(o) <= IE::ORIENTATION_SE) {
			// Orientation 5 uses bitmap from orientation 3 mirrored,
			// 6 uses 2, and 7 uses 1
			description.mirror = true;
			o = 8 - o;
		}
	}
	description.sequence_number = o;
	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			if (_HasG11(description.bam_name))
				description.bam_name.append("G11");
			else
				description.bam_name.append("G1");
			break;
		case ACT_STANDING:
			description.bam_name.append("G1");
			description.sequence_number += ANIM_STANDING_OFFSET;
			break;
		case ACT_ATTACKING:
			description.bam_name.append("G2");
			//description.sequence_number = o;
			break;
		case ACT_CAST_SPELL_PREPARE:
			description.bam_name.append("G25");
			description.sequence_number += 45;
			break;
		case ACT_CAST_SPELL_RELEASE:
			description.bam_name.append("G26");
			description.sequence_number += 54;
			break;
		default:
			std::cerr << "BGMonsterAnimationFactory::GetAnimationDescription(): UNIMPLEMENTED ";
			std::cerr << BaseName() << ", action " << actor->AnimationAction() << ", orientation " << o << std::endl;
			break;
	}
	return description;
}


animation_description
AnimationFactory::_GetBGCharacterAnimationDescription(Actor* actor)
{
	//std::cout << "BGAnimationFactory" << std::endl;
	int o = actor->Orientation();
	animation_description description;
	description.bam_name = BaseName();
	description.sequence_number = o;
	description.mirror = false;
	description.custom_colors = true;

	// Optional weapon id
	// TODO: improve
	if (!actor->WeaponAnimation().empty())
		description.bam_name.append(actor->WeaponAnimation().substr(0, 1));

	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			if (_HasW(description.bam_name))
				description.bam_name.append("W2");
			else
				description.bam_name.append("G1");
			description.sequence_number = o;
			break;
		case ACT_STANDING:
			description.bam_name.append("G1");
			description.sequence_number += ANIM_STANDING_OFFSET;
			break;
		case ACT_ATTACKING:
			description.bam_name.append("A1");
			description.sequence_number = o;
			break;
		default:
			std::cerr << "BGCharachterAnimationFactory::GetAnimationDescription(): UNIMPLEMENTED ";
			std::cerr << BaseName() << ", action " << actor->AnimationAction() << ", orientation " << o << std::endl;
			break;
	}
	if (o >= IE::ORIENTATION_NE
			&& o <= IE::ORIENTATION_SE) {
		description.bam_name.append("E");
	}
	return description;
}


static
void
_GetBG2MirroredAnimation(int& orientation, animation_description& description)
{
	description.mirror = true;
	orientation = 16 - orientation;
}


/* virtual */
animation_description
AnimationFactory::_GetBG2CharacterAnimationDescription(Actor* actor)
{
	//std::cout << "BG2AnimationFactory::AnimationFor" << std::endl;
	int o = actor->Orientation();
	animation_description description;
	description.mirror = false;
	description.sequence_number = 0;
	description.custom_colors = true;

	if (actor->InParty()) {
		// Charachter animations are specific
		description.bam_name = "";
		description.bam_name.append("C");

		// Race
		description.bam_name.append(_RaceCharacter(actor->CRE()->Race()));
		// Gender
		description.bam_name.append(_GenderCharacter(actor->CRE()->Gender()));
		// Class
		description.bam_name.append(_ClassCharacter(actor->CRE()->Class()));

		// Armor
		// TODO: Improve
		std::string armorAnimation = _ArmorCharacter(actor);
		//std::cout << armorAnimation << std::endl;
		// TODO: Correct ? Fighters seems always to have full plate
		if (description.bam_name[3] == 'F')
			description.bam_name.append("4");
		else
			description.bam_name.append(armorAnimation);
	} else {
		description.bam_name = BaseName();
		description.bam_name.append("1");
	}

	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			if (_HasW(description.bam_name))
				description.bam_name.append("W2");
			else
				description.bam_name.append("G11");
			break;
		case ACT_STANDING:
			description.bam_name.append("G1");
			if (Core::Get()->HasExtendedOrientations())
				description.sequence_number += ANIM_STANDING_OFFSET;
			else
				description.sequence_number += 8;
			break;
		case ACT_ATTACKING:
			description.bam_name.append("A1");
			break;
		case ACT_DIE:
			if (_HasG15(description.bam_name)) {
				description.bam_name.append("G15");
				description.sequence_number += ANIM_DIE_OFFSET;
			} else
				description.bam_name.append("G1");
			break;
		case ACT_DEAD:
			// Not a typo, if it has G15 it has also G16
			if (_HasG15(description.bam_name)) {
				description.bam_name.append("G16");
				description.sequence_number += ANIM_DEAD_OFFSET;
			} else
				description.bam_name.append("G1");
			break;
		case ACT_CAST_SPELL_PREPARE:
			std::cout << "CAST SPELL (PREPARE): " << BaseName() << std::endl;
			description.bam_name.append("C1");
			//description.sequence_number += 9;
			break;
		default:
			std::cerr << "BG2CharachterAnimationFactory::GetAnimationDescription(): UNIMPLEMENTED ";
			std::cerr << BaseName() << ", action " << actor->AnimationAction() << ", orientation " << o << std::endl;
			break;
	}
	if (Core::Get()->HasExtendedOrientations()) {
		if (o >= IE::ORIENTATION_EXT_NNE && o <= IE::ORIENTATION_EXT_SSE)
			_GetBG2MirroredAnimation(o, description);
	} else {
		if (o >= IE::ORIENTATION_NE && o <= IE::ORIENTATION_SE) {
			if (_HasSeparateEasternOrientations(description.bam_name))
				description.bam_name.append("E");
		}
	}
	description.sequence_number += o;

#if 0
	std::cout << description.bam_name << std::endl;
#endif
	return description;
}



animation_description
AnimationFactory::_GetSimpleAnimationDescription(Actor* actor)
{
	//std::cout << "SimpleAnimationFactory::AnimationFor" << std::endl;
	int o = actor->Orientation();
	animation_description description;
	description.bam_name = BaseName();
	description.mirror = false;
	description.custom_colors = true;

	if (Core::Get()->Game() == game::GAME_BALDURSGATE2)
		o = IE::orientation_ext_to_base(o);

	description.sequence_number = uint32(o);

	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			description.sequence_number += 0;
			break;
		case ACT_ATTACKING:
			description.sequence_number += 0;
			break;
		case ACT_STANDING:
			description.sequence_number += 8;
			break;
		case ACT_DIE:
		default:
			std::cout << "unknown action " << actor->AnimationAction() << std::endl;
			break;
	}

	description.bam_name.append("M");
	if (o >= IE::ORIENTATION_NE
			&& uint32(o) <= IE::ORIENTATION_SE) {
		// Orientation 5 uses bitmap from orientation 3 mirrored,
		// 6 uses 2, and 7 uses 1
		//description.mirror = true;
		// TODO: not in BG2. There is a separate file with East-facing animation
//		/description.bam_name.append("E");
		//description.sequence_number -= (o - 4) * 2;
	}
	return description;
}


/* virtual */
animation_description
AnimationFactory::_GetSplitAnimationDescription(Actor* actor)
{
	//std::cout << "SplitAnimationFactory::AnimationFor" << std::endl;
	int o = actor->Orientation();
	animation_description description;
	description.bam_name = BaseName();
	description.mirror = false;
	description.custom_colors = true;

	if (Core::Get()->Game() == game::GAME_BALDURSGATE2)
		o = IE::orientation_ext_to_base(o);

	description.sequence_number = uint32(o);

	// G1
	//if (IE::is_orientation_facing_north(o))
		description.bam_name.append("H");
	//else
		//description.bam_name.append("L");

	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			description.bam_name.append("G1");
			description.sequence_number += 0;
			break;
		case ACT_ATTACKING:
			description.bam_name.append("G1");
			description.sequence_number += 0;
			break;
		case ACT_STANDING:
			description.bam_name.append("G1");
			description.sequence_number += 8;
			break;
		case ACT_DIE:
		default:
			std::cout << "unknown action " << actor->AnimationAction() << std::endl;
			description.bam_name.append("G1");
			break;
	}

	if (o >= IE::ORIENTATION_NE
			&& uint32(o) <= IE::ORIENTATION_SE) {
		// Orientation 5 uses bitmap from orientation 3 mirrored,
		// 6 uses 2, and 7 uses 1
		//description.mirror = true;
		// TODO: not in BG2. There is a separate file with East-facing animation
		description.bam_name.append("E");
		//description.sequence_number -= (o - 4) * 2;
	}
	return description;
}


/* virtual */
animation_description
AnimationFactory::_GetIWDAnimationDescription(Actor* actor)
{
	//std::cout << "IWDAnimationFactory" << std::endl;
	int o = actor->Orientation();
	animation_description description;
	description.bam_name = BaseName();
	description.mirror = false;

	if (Core::Get()->Game() == game::GAME_BALDURSGATE2)
		o = IE::orientation_ext_to_base(o);

	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			description.bam_name.append("WK");
			description.sequence_number = o;
			break;
		case ACT_STANDING:
			description.bam_name.append("SD");
			description.sequence_number = o;
			break;
		case ACT_ATTACKING:
			description.bam_name.append("A1");
			description.sequence_number = o;
			break;
		default:
			std::cerr << "IWDAnimationFactory::GetAnimationDescription(): UNIMPLEMENTED ";
			std::cerr << BaseName() << ", action " << actor->AnimationAction() << ", orientation " << o << std::endl;
			break;
	}
	if (o >= IE::ORIENTATION_NE
			&& o <= IE::ORIENTATION_SE) {
		description.bam_name.append("E");
	}
	return description;
}


animation_description
AnimationFactory::_GetStaticAnimationDescription(Actor* actor)
{
	int o = actor->Orientation();
	animation_description description;
	description.bam_name = BaseName();
	description.mirror = false;
	description.custom_colors = true;

	if (Core::Get()->Game() == game::GAME_BALDURSGATE2)
		o = IE::orientation_ext_to_base(o);

	description.sequence_number = uint32(o);

	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			description.sequence_number += 0;
			break;
		case ACT_ATTACKING:
			description.sequence_number += 0;
			break;
		case ACT_STANDING:
			description.sequence_number += 8;
			break;
		case ACT_DIE:
		default:
			std::cout << "unknown action " << actor->AnimationAction() << std::endl;
			break;
	}

	if (o >= IE::ORIENTATION_NE
			&& uint32(o) <= IE::ORIENTATION_SE) {
		// Orientation 5 uses bitmap from orientation 3 mirrored,
		// 6 uses 2, and 7 uses 1
		//description.mirror = true;
		// TODO: not in BG2. There is a separate file with East-facing animation
//		/description.bam_name.append("E");
		//description.sequence_number -= (o - 4) * 2;
	}
	return description;
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
