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
#include <sstream>
#include <string>


// One builder per "animation style" (IESDP's avatar-naming families). The
// id -> (base name, builder) table below drives the dispatch - no enum,
// no switch (mirrors scripting/Actions.cpp's kActionsTable).
typedef animation_description (*BuildDescriptionFn)(const std::string& baseName,
													Actor* actor);

static animation_description _BuildBGMonster(const std::string&, Actor*);
static animation_description _BuildBGCharacter(const std::string&, Actor*);
static animation_description _BuildCharacter(const std::string&, Actor*);
static animation_description _BuildSimple(const std::string&, Actor*);
static animation_description _BuildSplit(const std::string&, Actor*);
static animation_description _BuildIWD(const std::string&, Actor*);
static animation_description _BuildStatic(const std::string&, Actor*);


struct AnimationEntry {
	uint16 animation_id;
	const char* base_name;
	BuildDescriptionFn build;
};


// Seems BG1 names are different for same IDS:
// TODO: Drop GeneratedIDS and use a similar array, differentiating games
static const AnimationEntry kAnimationEntries[] = {
	{ 0x1000, "",     _BuildBGMonster },
	{ 0x2000, "",     _BuildBGMonster },
	{ 0x2200, "MOGM", _BuildBGMonster },
	{ 0x2300, "",     _BuildBGMonster },

	{ 0x4000, "SNOM", _BuildSimple },
	{ 0x4010, "SNOW", _BuildSimple },
	{ 0x4100, "SSIM", _BuildSimple },
	{ 0x4101, "SSIM", _BuildSimple },
	{ 0x4400, "LHMC", _BuildStatic },
	{ 0x4600, "LDMF", _BuildStatic },
	{ 0x4700, "LEMF", _BuildStatic },
	{ 0x4710, "LEFF", _BuildStatic },

	{ 0x5000, "CHMB", _BuildCharacter },
	{ 0x5002, "CDMB", _BuildCharacter },
	{ 0x5003, "CIMB", _BuildCharacter },
	{ 0x5100, "CHMB", _BuildCharacter },
	{ 0x5102, "CDMB", _BuildCharacter },
	{ 0x5110, "CHFB", _BuildCharacter },
	{ 0x5113, "CIFB", _BuildCharacter },
	{ 0x5200, "",     _BuildCharacter },
	{ 0x5202, "CDMW", _BuildCharacter },
	{ 0x5210, "CHFW", _BuildCharacter },
	{ 0x5300, "CHMB", _BuildCharacter },
	{ 0x5303, "CIMB", _BuildCharacter },
	{ 0x6000, "CHMB", _BuildCharacter },
	{ 0x6002, "CDMB", _BuildCharacter },
	{ 0x6003, "CIMB", _BuildCharacter },
	{ 0x6004, "CDMB", _BuildCharacter },
	{ 0x6010, "CHFB", _BuildCharacter },
	{ 0x6011, "CEFB", _BuildCharacter },
	{ 0x6013, "CIFB", _BuildCharacter },
	{ 0x6100, "CHMB", _BuildCharacter },
	{ 0x6101, "CEMB", _BuildCharacter },
	{ 0x6102, "CDMB", _BuildCharacter },
	{ 0x6103, "CIMB", _BuildCharacter },
	{ 0x6104, "CDMB", _BuildCharacter },
	{ 0x6110, "CHFB", _BuildCharacter },
	{ 0x6111, "CEFB", _BuildCharacter },
	{ 0x6113, "CIFB", _BuildCharacter },
	{ 0x6200, "",     _BuildCharacter },
	{ 0x6201, "CEMW", _BuildCharacter },
	{ 0x6210, "CHFW", _BuildCharacter },
	{ 0x6211, "CEFW", _BuildCharacter },
	{ 0x6300, "CHMB", _BuildCharacter },
	{ 0x6301, "CEMB", _BuildCharacter },
	{ 0x6302, "CDMB", _BuildCharacter },
	{ 0x6303, "CIMB", _BuildCharacter },
	{ 0x6310, "CHFB", _BuildCharacter },
	{ 0x6311, "CEFB", _BuildCharacter },
	{ 0x6314, "CEFB", _BuildCharacter },
	{ 0x6315, "CEFB", _BuildCharacter },
	{ 0x6400, "",     _BuildCharacter },
	{ 0x6402, "CMNK", _BuildCharacter },
	{ 0x6403, "MSKL", _BuildCharacter },
	{ 0x6405, "MDGU", _BuildCharacter },
	{ 0x6500, "CHMM", _BuildCharacter },
	{ 0x6510, "CHFM", _BuildCharacter },

	{ 0x7000, "",     _BuildBGCharacter },
	{ 0x7001, "MOGR", _BuildBGMonster },
	{ 0x7202, "MBER", _BuildBGMonster },
	{ 0x7300, "",     _BuildBGMonster },
	{ 0x7400, "MDOG", _BuildBGMonster },
	{ 0x7703, "MSHD", _BuildBGMonster },
	{ 0x7a01, "MSPI", _BuildBGMonster },
	{ 0x7c01, "MTAS", _BuildBGMonster },
	{ 0x7b00, "MWLF", _BuildBGMonster },
	{ 0x7b01, "MWLF", _BuildBGMonster },
	{ 0x7b02, "MWLF", _BuildBGMonster },
	{ 0x7d00, "MZOM", _BuildBGMonster }, // (Zombie)
	{ 0x7e00, "",     _BuildBGMonster },
	{ 0x7f03, "MIMP", _BuildBGMonster },
	{ 0x7f05, "MDJI", _BuildBGMonster },
	{ 0x7f06, "MDJL", _BuildBGMonster },
	{ 0x7f07, "MGLC", _BuildBGMonster },
	{ 0x7f08, "MOTY", _BuildBGMonster },
	{ 0x7f0a, "MGCP", _BuildBGMonster },
	{ 0x7f0b, "MGCL", _BuildBGMonster },
	{ 0x7f0d, "MLIC", _BuildBGMonster },
	{ 0x7f10, "MRAK", _BuildBGMonster },
	{ 0x7f13, "MSNK", _BuildBGMonster },
	{ 0x7f16, "AMOO", _BuildBGMonster },
	{ 0x7f17, "ARAB", _BuildBGMonster },
	{ 0x7f18, "ADER", _BuildBGMonster },
	{ 0x7f20, "AGRO", _BuildBGMonster },
	{ 0x7f21, "APHE", _BuildBGMonster },
	{ 0x7f22, "MVAF", _BuildBGMonster },
	{ 0x7f24, "NPIR", _BuildBGMonster },
	{ 0x7f2a, "NSAI", _BuildBGMonster },
	{ 0x7f2c, "NSOL", _BuildBGMonster },
	{ 0x7f36, "NSHD", _BuildBGMonster },
	{ 0x7f37, "NIRE", _BuildBGMonster },
	{ 0x8000, "",     _BuildBGMonster },
	{ 0x8100, "",     _BuildBGMonster },
	{ 0x9000, "MOGR", _BuildBGMonster },
	{ 0xa000, "",     _BuildBGMonster },
	{ 0xb000, "ACOW", _BuildBGMonster },
	{ 0xb100, "AHRS", _BuildBGMonster },
	{ 0xb200, "NBEG", _BuildSplit },
	{ 0xb400, "NFAM", _BuildSplit },
	{ 0xb410, "NFAW", _BuildSplit },
	{ 0xb500, "NSIM", _BuildSplit },
	{ 0xb510, "NSIW", _BuildSplit },
	{ 0xc000, "ABAT", _BuildBGMonster },
	{ 0xc100, "ACAT", _BuildBGMonster },
	{ 0xc200, "ACHK", _BuildBGMonster },
	{ 0xc300, "ARAT", _BuildBGMonster },
	{ 0xc400, "",     _BuildBGMonster },
	{ 0xc500, "",     _BuildBGMonster },
	{ 0xc700, "NBOY", _BuildSplit },
	{ 0xc800, "NFAM", _BuildSplit },
	{ 0xc600, "NBEG", _BuildSplit },
	{ 0xc610, "NPRO", _BuildSplit },
	{ 0xc710, "NGRL", _BuildSplit },
	{ 0xc800, "",     _BuildSplit },
	{ 0xc810, "NFAW", _BuildSplit },
	{ 0xc900, "NSIM", _BuildSplit },
	{ 0xc910, "NSIW", _BuildSplit },
	{ 0xca00, "NNOM", _BuildSplit },
	{ 0xca10, "NNOW", _BuildSplit },
	{ 0xd000, "AEAG", _BuildBGMonster }, // AEAG (Eagle)
	{ 0xd100, "AGUL", _BuildBGMonster },
	{ 0xd200, "",     _BuildBGMonster },
	{ 0xd300, "",     _BuildBGMonster },
	{ 0xe000, "",     _BuildIWD },
	{ 0xe400, "",     _BuildIWD },
	{ 0xe430, "MG04", _BuildIWD },
	{ 0xe600, "",     _BuildIWD },
	{ 0xe710, "MNO2", _BuildIWD },
	{ 0xed00, "MYU1", _BuildIWD },
};


// --- shared helpers ---------------------------------------------------

// One BAM-suffix character per race (avatarnaming.htm).
static const char*
_RaceCharacter(uint8 race)
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


static const char*
_GenderCharacter(uint8 gender)
{
	return gender == 2 ? "F" : "M";
}


static std::string
_ClassCharacter(uint8 c, const std::string& baseName)
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
			// Not one of those: fall back to the factory's own base name.
			return baseName.substr(3, 1);
	}
}


static std::string
_ArmorCharacter(const Actor* actor)
{
	return actor->ArmorAnimation().substr(0, 1);
}


static bool
_HasBAMVariant(const std::string& name, const char* suffix)
{
	return gResManager->ResourceExists((name + suffix).c_str(), RES_BAM);
}


// "C" + race + gender + class + armour-state digit - the shared identity
// prefix for a party member's sprite and their inventory paperdoll.
// forcePlateForFighters: the in-world sprite shows every fighter-type in
// full plate (a long-standing quirk, see AnimationFor()); the paperdoll
// does not - it tracks the actually-worn armour.
static std::string
_CharacterIdentityPrefix(const Actor* actor, const std::string& baseName,
						bool forcePlateForFighters)
{
	std::string name = "C";
	name += _RaceCharacter(actor->CRE()->Race());
	name += _GenderCharacter(actor->CRE()->Gender());
	name += _ClassCharacter(actor->CRE()->Class(), baseName);
	if (forcePlateForFighters && name[3] == 'F')
		name += "4";
	else
		name += _ArmorCharacter(actor);
	return name;
}


// BG2 stores only the 8 base facings for most non-character sprites; fold
// an extended (16-way) orientation back to one of them.
static int
_BaseOrientation(int o)
{
	if (Core::Get()->Game() == game::GAME_BALDURSGATE2)
		return IE::orientation_ext_to_base(o);
	return o;
}


// An extended NE..SE facing (9..15) is drawn as its mirrored western
// counterpart (5 uses 3, 6 uses 2, 7 uses 1).
static int
_MirrorExtendedOrientation(int o, animation_description& description)
{
	description.mirror = true;
	return 16 - o;
}


// Some sprites carry a separate east-facing BAM ("...E") for base NE..SE
// orientations. onlyIfPresent: append it only when that BAM exists (the
// character/BG-character styles), otherwise always (split/IWD styles,
// where the E file is assumed).
static void
_AppendEasternSuffix(animation_description& description, int o, bool onlyIfPresent)
{
	if (o < IE::ORIENTATION_NE || o > IE::ORIENTATION_SE)
		return;
	if (!onlyIfPresent || _HasBAMVariant(description.bam_name, "E"))
		description.bam_name += "E";
}


static void
_WarnUnimplementedAction(const char* style, const std::string& baseName, Actor* actor)
{
	std::cerr << style << ": unimplemented action " << actor->AnimationAction()
		<< " for " << baseName << std::endl;
}


// --- per-style builders ---------------------------------------------------

static animation_description
_BuildBGMonster(const std::string& baseName, Actor* actor)
{
	int o = actor->Orientation();
	animation_description description;
	description.bam_name = baseName;

	if (Core::Get()->Game() == game::GAME_BALDURSGATE2) {
		if (o >= IE::ORIENTATION_EXT_NNE && o <= IE::ORIENTATION_EXT_SSE)
			o = _MirrorExtendedOrientation(o, description);
	} else if (o >= IE::ORIENTATION_NE && o <= IE::ORIENTATION_SE) {
		description.mirror = true;
		o = 8 - o;
	}
	description.sequence_number = o;

	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			description.bam_name += _HasBAMVariant(description.bam_name, "G11") ? "G11" : "G1";
			break;
		case ACT_STANDING:
			description.bam_name += "G1";
			description.sequence_number += ANIM_STANDING_OFFSET;
			break;
		case ACT_ATTACKING:
			description.bam_name += "G2";
			break;
		case ACT_CAST_SPELL_PREPARE:
			description.bam_name += "G25";
			description.sequence_number += 45;
			break;
		case ACT_CAST_SPELL_RELEASE:
			description.bam_name += "G26";
			description.sequence_number += 54;
			break;
		case ACT_DIE:
		case ACT_DEAD:
			// "BG1 monster style" (avatarnaming.htm) has no separate "dead"
			// file the way BG2 characters do (G15 vs G16) - G15 ("Twitch")
			// is both: played once for the die animation, then meant to
			// freeze on its own last frame. Mapping ACT_DEAD to the same
			// file/sequence is an approximation - Actor::UpdateAnimation()'s
			// generic auto-switch restarts G15 from frame 0 and freezes
			// there, not on its true last frame.
			description.bam_name += _HasBAMVariant(description.bam_name, "G15") ? "G15" : "G1";
			break;
		default:
			_WarnUnimplementedAction("BGMonster", baseName, actor);
			break;
	}
	return description;
}


static animation_description
_BuildBGCharacter(const std::string& baseName, Actor* actor)
{
	int o = actor->Orientation();
	animation_description description;
	description.bam_name = baseName;
	description.sequence_number = o;
	description.custom_colors = true;

	// Optional weapon id
	if (!actor->WeaponAnimation().empty())
		description.bam_name += actor->WeaponAnimation().substr(0, 1);

	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			description.bam_name += _HasBAMVariant(description.bam_name, "W2") ? "W2" : "G1";
			break;
		case ACT_STANDING:
			description.bam_name += "G1";
			break;
		case ACT_ATTACKING:
			description.bam_name += "A1";
			break;
		default:
			_WarnUnimplementedAction("BGCharacter", baseName, actor);
			break;
	}
	_AppendEasternSuffix(description, o, true);
	return description;
}


static animation_description
_BuildCharacter(const std::string& baseName, Actor* actor)
{
	int o = actor->Orientation();
	animation_description description;
	description.custom_colors = true;

	description.bam_name = actor->InParty()
		? _CharacterIdentityPrefix(actor, baseName, true)
		: baseName + "1";

	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			description.bam_name += _HasBAMVariant(description.bam_name, "W2") ? "W2" : "G11";
			break;
		case ACT_STANDING:
			description.bam_name += "G1";
			description.sequence_number += Core::Get()->HasExtendedOrientations()
				? ANIM_STANDING_OFFSET : 8;
			break;
		case ACT_ATTACKING:
			description.bam_name += "A1";
			break;
		case ACT_DIE:
			if (_HasBAMVariant(description.bam_name, "G15")) {
				description.bam_name += "G15";
				description.sequence_number += ANIM_DIE_OFFSET;
			} else
				description.bam_name += "G1";
			break;
		case ACT_DEAD:
			// If it has G15 it also has G16.
			if (_HasBAMVariant(description.bam_name, "G15")) {
				description.bam_name += "G16";
				description.sequence_number += ANIM_DEAD_OFFSET;
			} else
				description.bam_name += "G1";
			break;
		case ACT_CAST_SPELL_PREPARE:
			description.bam_name += "C1";
			break;
		default:
			_WarnUnimplementedAction("Character", baseName, actor);
			break;
	}

	if (Core::Get()->HasExtendedOrientations()) {
		if (o >= IE::ORIENTATION_EXT_NNE && o <= IE::ORIENTATION_EXT_SSE)
			o = _MirrorExtendedOrientation(o, description);
	} else {
		_AppendEasternSuffix(description, o, true);
	}
	description.sequence_number += o;
	return description;
}


static animation_description
_BuildSimple(const std::string& baseName, Actor* actor)
{
	int o = _BaseOrientation(actor->Orientation());
	animation_description description;
	description.bam_name = baseName;
	description.custom_colors = true;
	description.sequence_number = o;

	switch (actor->AnimationAction()) {
		case ACT_WALKING:
		case ACT_ATTACKING:
			break;
		case ACT_STANDING:
			description.sequence_number += 8;
			break;
		default:
			_WarnUnimplementedAction("Simple", baseName, actor);
			break;
	}

	description.bam_name += "M";
	return description;
}


static animation_description
_BuildSplit(const std::string& baseName, Actor* actor)
{
	int o = _BaseOrientation(actor->Orientation());
	animation_description description;
	description.bam_name = baseName;
	description.custom_colors = true;
	description.sequence_number = o;

	// TODO: the north-facing half should be "L" instead of "H".
	description.bam_name += "H";

	switch (actor->AnimationAction()) {
		case ACT_WALKING:
		case ACT_ATTACKING:
			description.bam_name += "G1";
			break;
		case ACT_STANDING:
			description.bam_name += "G1";
			description.sequence_number += 8;
			break;
		default:
			_WarnUnimplementedAction("Split", baseName, actor);
			description.bam_name += "G1";
			break;
	}

	_AppendEasternSuffix(description, o, false);
	return description;
}


static animation_description
_BuildIWD(const std::string& baseName, Actor* actor)
{
	int o = _BaseOrientation(actor->Orientation());
	animation_description description;
	description.bam_name = baseName;
	description.sequence_number = o;

	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			description.bam_name += "WK";
			break;
		case ACT_STANDING:
			description.bam_name += "SD";
			break;
		case ACT_ATTACKING:
			description.bam_name += "A1";
			break;
		default:
			_WarnUnimplementedAction("IWD", baseName, actor);
			break;
	}
	_AppendEasternSuffix(description, o, false);
	return description;
}


static animation_description
_BuildStatic(const std::string& baseName, Actor* actor)
{
	int o = _BaseOrientation(actor->Orientation());
	animation_description description;
	description.bam_name = baseName;
	description.custom_colors = true;
	description.sequence_number = o;

	switch (actor->AnimationAction()) {
		case ACT_WALKING:
		case ACT_ATTACKING:
			break;
		case ACT_STANDING:
			description.sequence_number += 8;
			break;
		default:
			_WarnUnimplementedAction("Static", baseName, actor);
			break;
	}
	return description;
}


// --- AnimationFactory ---------------------------------------------------

/* static */
AnimationFactory*
AnimationFactory::GetFactory(uint16 animationID)
{
	std::string baseName = IDTable::AniSndAt(animationID);
	return new AnimationFactory(baseName.c_str(), animationID);
}


/* static */
void
AnimationFactory::ReleaseFactory(AnimationFactory* factory)
{
	delete factory;
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

	try {
		IE::point pos;
		return new Animation(description.bam_name.c_str(),
							description.sequence_number, description.mirror,
							pos, description.custom_colors ? colors : nullptr);
	} catch (...) {
		return NULL;
	}
}


std::string
AnimationFactory::PaperdollName(const Actor* actor) const
{
	std::string name;
	if (actor->InParty())
		name = _CharacterIdentityPrefix(actor, fBaseName, false);
	else
		name = fBaseName + "1";
	name += "INV";
	return name;
}


animation_description
AnimationFactory::GetAnimationDescription(Actor* actor)
{
	auto it = std::find_if(std::begin(kAnimationEntries), std::end(kAnimationEntries),
						[this] (const AnimationEntry& entry) {
							return entry.animation_id == fID;
						});
	if (it == std::end(kAnimationEntries)) {
		std::ostringstream error;
		error << "Animation description not found for " << fBaseName
			<< " (0x" << std::hex << fID << ")";
		throw std::runtime_error(error.str());
	}

	// Some animations aren't in the AniSnd file - use the table's name.
	if (fBaseName.empty())
		fBaseName = it->base_name;

	return it->build(fBaseName, actor);
}
