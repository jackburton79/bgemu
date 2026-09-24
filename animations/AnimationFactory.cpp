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
#include "ITMResource.h"
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
static animation_description _BuildCharacter(const std::string&, Actor*);
static animation_description _BuildSplit(const std::string&, Actor*);
static animation_description _BuildIWD(const std::string&, Actor*);
static animation_description _BuildMOGR(const std::string&, Actor*);
static animation_description _BuildFourFiles(const std::string&, Actor*);
static animation_description _BuildOneFile(const std::string&, Actor*);
static animation_description _BuildTwoFiles(const std::string&, Actor*);
static animation_description _BuildTwoFiles2(const std::string&, Actor*);
static animation_description _BuildSixFiles(const std::string&, Actor*);


struct AnimationEntry {
	uint16 animation_id;
	const char* base_name;
	BuildDescriptionFn build;
	// The CRE's color bytes recolor the BAM (the generic character-style
	// sprites). False for a creature drawn with the BAM's own palette
	// (avatars.2da's PALETTE column, GemRB's bg1 table: Sarevok, skeletons,
	// most monsters) and for the styles that never take the colors.
	bool custom_colors;
};


// `base_name` here is only used as a fallback for a non-party actor
// (AnimationFactory::GetAnimationDescription()) when GeneratedIDS::
// FillAniSnd()'s BG1 ANISND.IDS substitute doesn't have that animation
// id - which it mostly does. Checked entry by entry (2026-09): about
// half of GeneratedIDS's entries duplicate the same name already here
// and were dropped from it; the rest genuinely differ (this table's
// base_name is a placeholder never used for party members - see
// _ClassCharacter()'s switch, which derives the class letter straight
// from the CRE and only falls back to base_name for an unmapped class)
// or are BG1-only animation ids (e.g. MAKH/Ankheg) this table doesn't
// carry at all - merging those without verifying each one against real
// BG1 data risks silently picking the wrong sprite for non-party
// actors, so GeneratedIDS keeps them.
static const AnimationEntry kAnimationEntries[] = {
	{ 0x1000, "MWYV", _BuildBGMonster, false },
	{ 0x2000, "MSIR", _BuildFourFiles, false }, // avatars.2da TYPE 2 (FOUR_FILES)
	{ 0x2200, "MOGM", _BuildFourFiles, false }, // avatars.2da TYPE 2 (FOUR_FILES)
	{ 0x2300, "",     _BuildFourFiles, false }, // avatars.2da TYPE 2 (FOUR_FILES)

	// avatars.2da TYPE 1 (ONE_FILE) throughout this block. bgemu's own
	// base names for the SNOM/SSIM* rows were missing their trailing
	// variant letter (the real, complete resref - confirmed against
	// avatars.2da and that resource actually existing); the L-prefixed
	// ids already had the correct complete name.
	{ 0x4000, "SNOMC", _BuildOneFile, true },
	{ 0x4010, "SNOWC", _BuildOneFile, true },
	{ 0x4100, "SSIMC", _BuildOneFile, true },
	{ 0x4101, "SSIMS", _BuildOneFile, true },
	{ 0x4102, "SSIMM", _BuildOneFile, true },
	{ 0x4110, "SSIWC", _BuildOneFile, true },
	{ 0x4400, "LHMC", _BuildOneFile, true },
	{ 0x4410, "LHFC", _BuildOneFile, true },
	{ 0x4500, "LFAM", _BuildOneFile, true },
	{ 0x4600, "LDMF", _BuildOneFile, true },
	{ 0x4700, "LEMF", _BuildOneFile, true },
	{ 0x4710, "LEFF", _BuildOneFile, true },
	{ 0x4800, "LIMC", _BuildOneFile, true }, // SLEEPING_MAN_HALFLING

	{ 0x5000, "CHMB", _BuildCharacter, true },
	{ 0x5002, "CDMB", _BuildCharacter, true },
	{ 0x5003, "CIMB", _BuildCharacter, true },
	{ 0x5100, "CHMB", _BuildCharacter, true },
	{ 0x5102, "CDMB", _BuildCharacter, true },
	{ 0x5110, "CHFB", _BuildCharacter, true },
	{ 0x5113, "CIFB", _BuildCharacter, true },
	{ 0x5200, "",     _BuildCharacter, true },
	{ 0x5202, "CDMW", _BuildCharacter, true },
	{ 0x5210, "CHFW", _BuildCharacter, true },
	{ 0x5300, "CHMB", _BuildCharacter, true },
	{ 0x5303, "CIMB", _BuildCharacter, true },
	{ 0x6000, "CHMB", _BuildCharacter, true },
	{ 0x6002, "CDMB", _BuildCharacter, true },
	{ 0x6003, "CIMB", _BuildCharacter, true },
	{ 0x6004, "CDMB", _BuildCharacter, true },
	{ 0x6010, "CHFB", _BuildCharacter, true },
	{ 0x6011, "CEFB", _BuildCharacter, true },
	{ 0x6013, "CIFB", _BuildCharacter, true },
	{ 0x6100, "CHMB", _BuildCharacter, true },
	{ 0x6101, "CEMB", _BuildCharacter, true },
	{ 0x6102, "CDMB", _BuildCharacter, true },
	{ 0x6103, "CIMB", _BuildCharacter, true },
	{ 0x6104, "CDMB", _BuildCharacter, true },
	{ 0x6110, "CHFB", _BuildCharacter, true },
	{ 0x6111, "CEFB", _BuildCharacter, true },
	{ 0x6113, "CIFB", _BuildCharacter, true },
	{ 0x6200, "",     _BuildCharacter, true },
	{ 0x6201, "CEMW", _BuildCharacter, true },
	{ 0x6210, "CHFW", _BuildCharacter, true },
	{ 0x6211, "CEFW", _BuildCharacter, true },
	{ 0x6300, "CHMB", _BuildCharacter, true },
	{ 0x6301, "CEMB", _BuildCharacter, true },
	{ 0x6302, "CDMB", _BuildCharacter, true },
	{ 0x6303, "CIMB", _BuildCharacter, true },
	{ 0x6310, "CHFB", _BuildCharacter, true },
	{ 0x6311, "CEFB", _BuildCharacter, true },
	{ 0x6314, "CEFB", _BuildCharacter, true },
	{ 0x6315, "CEFB", _BuildCharacter, true },
	{ 0x6400, "",     _BuildCharacter, true },
	{ 0x6402, "CMNK", _BuildCharacter, true },
	{ 0x6403, "MSKL", _BuildCharacter, false },
	{ 0x6404, "USAR", _BuildCharacter, false }, // Sarevok
	{ 0x6405, "MDGU", _BuildCharacter, true },
	{ 0x6500, "CHMM", _BuildCharacter, true },
	{ 0x6510, "CHFM", _BuildCharacter, true },

	{ 0x7000, "",     _BuildFourFiles, false }, // avatars.2da TYPE 14 (FOUR_FILES_2)
	{ 0x7001, "MOGN", _BuildFourFiles, false }, // Ogrillon - confirmed against avatars.2da;
	{ 0x7202, "MBER", _BuildFourFiles, false }, // avatars.2da TYPE 14 (FOUR_FILES_2)
	{ 0x7300, "",     _BuildBGMonster, false },
	{ 0x7400, "MDOG", _BuildFourFiles, false }, // avatars.2da TYPE 14 (FOUR_FILES_2)
	{ 0x7700, "MGHL", _BuildFourFiles, false }, // Ghoul - confirmed against avatars.2da; avatars.2da TYPE 14 (FOUR_FILES_2)
	{ 0x7703, "MGHL", _BuildFourFiles, false }, // Ghoul - confirmed against avatars.2da; avatars.2da TYPE 14 (FOUR_FILES_2)
	{ 0x7a01, "MSPI", _BuildFourFiles, false }, // avatars.2da TYPE 14 (FOUR_FILES_2)
	{ 0x7c01, "MTAS", _BuildFourFiles, false }, // avatars.2da TYPE 14 (FOUR_FILES_2)
	{ 0x7b00, "MWLF", _BuildFourFiles, false }, // avatars.2da TYPE 14 (FOUR_FILES_2)
	{ 0x7b01, "MWLF", _BuildFourFiles, false }, // avatars.2da TYPE 14 (FOUR_FILES_2)
	{ 0x7b02, "MWLF", _BuildFourFiles, false }, // avatars.2da TYPE 14 (FOUR_FILES_2)
	{ 0x7d00, "MZOM", _BuildFourFiles, false }, // (Zombie) avatars.2da TYPE 14 (FOUR_FILES_2)
	{ 0x7e00, "",     _BuildFourFiles, false }, // avatars.2da TYPE 14 (FOUR_FILES_2)
	{ 0x7f02, "MBEH", _BuildBGMonster, false },
	{ 0x7f03, "MIMP", _BuildBGMonster, false },
	{ 0x7f05, "MDJI", _BuildBGMonster, false },
	{ 0x7f06, "MDJL", _BuildBGMonster, false },
	{ 0x7f07, "MGLC", _BuildBGMonster, false },
	{ 0x7f08, "MOTY", _BuildBGMonster, false },
	{ 0x7f09, "MSAH", _BuildBGMonster, false },
	{ 0x7f0a, "MGCP", _BuildBGMonster, false },
	{ 0x7f0b, "MGCL", _BuildBGMonster, false },
	{ 0x7f0d, "MLIC", _BuildBGMonster, false },
	{ 0x7f10, "MRAK", _BuildBGMonster, false },
	{ 0x7f13, "MSNK", _BuildBGMonster, false },
	{ 0x7f16, "AMOO", _BuildBGMonster, false },
	{ 0x7f17, "ARAB", _BuildBGMonster, false },
	{ 0x7f18, "ADER", _BuildBGMonster, false },
	{ 0x7f20, "AGRO", _BuildBGMonster, false },
	{ 0x7f21, "APHE", _BuildBGMonster, false },
	{ 0x7f22, "MVAF", _BuildBGMonster, false },
	{ 0x7f23, "MSAT", _BuildBGMonster, false },
	{ 0x7f24, "NPIR", _BuildBGMonster, false },
	{ 0x7f2a, "NSAI", _BuildBGMonster, false },
	{ 0x7f2c, "NSOL", _BuildBGMonster, false },
	{ 0x7f36, "NSHD", _BuildBGMonster, false },
	{ 0x7f37, "NIRE", _BuildBGMonster, false },
	{ 0x8000, "",     _BuildFourFiles, false }, // avatars.2da TYPE 2 (FOUR_FILES)
	{ 0x8100, "",     _BuildFourFiles, false }, // avatars.2da TYPE 2 (FOUR_FILES)
	{ 0x9000, "MOGR", _BuildMOGR, false }, // Ogre - avatars.2da TYPE 5 (SIX_FILES_2), GemRB's own comment: "Only one animation uses it: MOGR"
	{ 0xa000, "MWYV", _BuildSixFiles, false }, // avatars.2da TYPE 8 (SIX_FILES)
	{ 0xb000, "ACOW", _BuildTwoFiles2, false }, // avatars.2da TYPE 10 (TWO_FILES_2)
	{ 0xb100, "AHRS", _BuildTwoFiles2, false }, // avatars.2da TYPE 10 (TWO_FILES_2)
	{ 0xb200, "NBEG", _BuildSplit, true },
	{ 0xb210, "NPRO", _BuildSplit, true },
	{ 0xb300, "NBOY", _BuildSplit, true },
	{ 0xb310, "NGRL", _BuildSplit, true },
	{ 0xb400, "NFAM", _BuildSplit, true },
	{ 0xb410, "NFAW", _BuildSplit, true },
	{ 0xb500, "NSIM", _BuildSplit, true },
	{ 0xb510, "NSIW", _BuildSplit, true },
	{ 0xb600, "NNOM", _BuildSplit, true },
	// avatars.2da TYPE 3 (TWO_FILES) throughout this block.
	{ 0xc000, "ABAT", _BuildTwoFiles, false },
	{ 0xc100, "ACAT", _BuildTwoFiles, false },
	{ 0xc200, "ACHK", _BuildTwoFiles, false },
	{ 0xc300, "ARAT", _BuildTwoFiles, false },
	{ 0xc400, "ASQU", _BuildTwoFiles, false },
	{ 0xc500, "ABAT", _BuildTwoFiles, false },
	{ 0xc700, "NBOY", _BuildSplit, true },
	{ 0xc800, "NFAM", _BuildSplit, true },
	{ 0xc600, "NBEG", _BuildSplit, true },
	{ 0xc610, "NPRO", _BuildSplit, true },
	{ 0xc710, "NGRL", _BuildSplit, true },
	{ 0xc800, "",     _BuildSplit, true },
	{ 0xc810, "NFAW", _BuildSplit, true },
	{ 0xc900, "NSIM", _BuildSplit, true },
	{ 0xc910, "NSIW", _BuildSplit, true },
	{ 0xca00, "NNOM", _BuildSplit, true },
	{ 0xca10, "NNOW", _BuildSplit, true },
	{ 0xd000, "AEAG", _BuildBGMonster, false }, // AEAG (Eagle)
	{ 0xd100, "AGUL", _BuildBGMonster, false }, // Seagull
	{ 0xd200, "",     _BuildBGMonster, false },
	{ 0xd300, "",     _BuildBGMonster, false },
	{ 0xe000, "",     _BuildIWD, false },
	{ 0xe010, "METN", _BuildIWD, false },
	{ 0xe400, "",     _BuildIWD, false },
	{ 0xe430, "MG04", _BuildIWD, false },
	{ 0xe600, "",     _BuildIWD, false },
	{ 0xe710, "MNO2", _BuildIWD, false },
	{ 0xeb10, "MSKA", _BuildIWD, false }, // (Skeleton Warrior)
	{ 0xeb20, "MSKB", _BuildIWD, false },
	{ 0xed00, "MYU1", _BuildIWD, false },
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


// Actor::Orientation() is always an extended (16-way) value (see its own
// declaration comment) - most non-character sprite styles only store the
// 8 base facings, so fold it back down to one of them.
static int
_BaseOrientation(int o)
{
	return IE::orientation_ext_to_base(o);
}


// "BG1 character style" (avatarnaming.htm) ships a dedicated file for
// eastern orientations; "BG2/IWD style" generates them by mirroring
// instead (see _BuildCharacter(), the only remaining caller - every
// other style/game distinction this project cared about turned out to
// be about the animation's own file layout, not the actor's actual
// facing, which is always extended - see Actor::Orientation()).
static bool
_GeneratesEasternOrientations()
{
	return Core::Get()->Game() != game::GAME_BALDURSGATE;
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

	// "BG1 monster style" (avatarnaming.htm): every G-file for this
	// style stores 9 cycles spanning the western half-circle - S, SSW,
	// SW, ..., N, at extended (16-way) granularity - and mirrors them
	// for the 7 eastern orientations (confirmed against a real BAM
	// dump: AGULG1.BAM has cycles 0-8 and 9-17). Actor::Orientation()
	// is always extended already, so this applies as-is regardless of
	// game.
	if (o >= IE::ORIENTATION_EXT_NNE && o <= IE::ORIENTATION_EXT_SSE)
		o = _MirrorExtendedOrientation(o, description);
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


// avatars.2da TYPE 5 (SIX_FILES_2) - GemRB's own comment: "Only one
// animation uses it: MOGR" (the ogre), so this is a dedicated one-off
// builder rather than a general style. Three files (G1/G2/G3), each
// bundling several actions at base-8-folded (not mirrored) cycle
// offsets, with a real dedicated East file for the eastern facings -
// confirmed against both GemRB's CharAnimations::AddLR3Suffix() and
// the real MOGRG1/G2/G3(E) BAMs' own cycle counts (24/16/32 = 3/2/4
// banks of 8).
static animation_description
_BuildMOGR(const std::string& baseName, Actor* actor)
{
	int o = IE::orientation_ext_to_base(actor->Orientation());
	animation_description description;
	description.bam_name = baseName;

	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			description.bam_name += "G1";
			description.sequence_number = 16 + o;
			break;
		case ACT_STANDING:
			description.bam_name += "G1";
			description.sequence_number = 8 + o;
			break;
		case ACT_ATTACKING:
			description.bam_name += "G2";
			description.sequence_number = o;
			break;
		case ACT_CAST_SPELL_PREPARE:
		case ACT_CAST_SPELL_RELEASE:
			description.bam_name += "G3";
			description.sequence_number = o;
			break;
		case ACT_DIE:
			description.bam_name += "G3";
			description.sequence_number = 16 + o;
			break;
		case ACT_DEAD:
			description.bam_name += "G3";
			description.sequence_number = 24 + o;
			break;
		default:
			_WarnUnimplementedAction("MOGR", baseName, actor);
			break;
	}

	_AppendEasternSuffix(description, o, false);
	return description;
}


// avatars.2da TYPE 2 (FOUR_FILES) and TYPE 14 (FOUR_FILES_2) - GemRB's
// own comment on the latter: "Like FOUR_FILES but with only 16 cycles
// per frame" (a shorter G2 file - an extra attack-variant bank this
// engine's action model doesn't need anyway). For every action bgemu
// actually models, both types share the identical G1/G2 bank layout:
// base-8-folded (Orient/2) cycles with a real East file rather than a
// runtime mirror - confirmed against GemRB's AddLRSuffix()/
// AddLRSuffix2() and real BAMs' own cycle counts (MWLFG1/G2.BAM:
// 48/24 = 6/3 banks of 8; only the first 2 G2 banks - attack, cast -
// are ever addressed here).
static animation_description
_BuildFourFiles(const std::string& baseName, Actor* actor)
{
	int halfOrient = actor->Orientation() / 2;
	animation_description description;
	description.bam_name = baseName;

	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			description.bam_name += "G1";
			description.sequence_number = halfOrient;
			break;
		case ACT_STANDING:
			description.bam_name += "G1";
			description.sequence_number = 8 + halfOrient;
			break;
		case ACT_ATTACKING:
			description.bam_name += "G2";
			description.sequence_number = halfOrient;
			break;
		case ACT_CAST_SPELL_PREPARE:
		case ACT_CAST_SPELL_RELEASE:
			description.bam_name += "G2";
			description.sequence_number = 8 + halfOrient;
			break;
		case ACT_DIE:
			description.bam_name += "G1";
			description.sequence_number = 32 + halfOrient;
			break;
		case ACT_DEAD:
			description.bam_name += "G1";
			description.sequence_number = 40 + halfOrient;
			break;
		default:
			_WarnUnimplementedAction("FourFiles", baseName, actor);
			break;
	}

	_AppendEasternSuffix(description, halfOrient, false);
	return description;
}


static animation_description
_BuildCharacter(const std::string& baseName, Actor* actor)
{
	int o = actor->Orientation();
	animation_description description;

	description.bam_name = actor->InParty()
		? _CharacterIdentityPrefix(actor, baseName, true)
		: baseName + "1";

	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			description.bam_name += _HasBAMVariant(description.bam_name, "W2") ? "W2" : "G11";
			break;
		case ACT_STANDING:
			description.bam_name += "G1";
			description.sequence_number += _GeneratesEasternOrientations()
				? ANIM_STANDING_OFFSET : 8;
			break;
		case ACT_ATTACKING: {
			// avatarnaming.htm "Character": Action='A' detail digit
			// 1 (1H) / 2 (2H) overhead swing for melee, or Action='S'
			// with an empty ("bow") / "x" ("crossbow") suffix for a
			// ranged weapon - see Actor::EquippedWeaponAnimationType().
			// Falls back to the melee digit if this character doesn't
			// actually have a Shooting BAM (not every race/class/armour
			// combination does).
			WeaponAnimationType weapon = actor->EquippedWeaponAnimationType();
			std::string suffix = weapon.isTwoHanded ? "A2" : "A1";
			if (weapon.isRanged) {
				std::string shootSuffix = weapon.isCrossbow ? "Sx" : "S";
				if (_HasBAMVariant(description.bam_name, shootSuffix.c_str()))
					suffix = shootSuffix;
			}
			description.bam_name += suffix;

			// The body swing above is weapon-agnostic (avatarnaming.htm
			// "Character": "Weapon overlays are in separate files") - the
			// weapon itself, when one is equipped, is a second BAM drawn
			// on top at the same cycle/orientation (see
			// AnimationFactory::WeaponOverlayFor()).
			ITMResource* weaponItem = actor->EquippedWeapon();
			if (weaponItem != nullptr) {
				std::string code = weaponItem->Animation();
				if (!code.empty()) {
					description.weapon_bam_name = std::string("WP")
						+ AnimationFactory::SizeCodeForActor(actor) + code + suffix;
				}
				gResManager->ReleaseResource(weaponItem);
			}
			break;
		}
		case ACT_DIE:
			// A separate G15 file isn't universal even within a single
			// game (e.g. BG2's own MSKL - a skeleton warrior avatar -
			// has none), so this is decided by whether the file actually
			// exists, not by _GeneratesEasternOrientations(). Without
			// one, every action lives in G1 itself, 8 orientation slots
			// per bank; Die is bank 6 (confirmed against both a real
			// no-G15 BAM's own cycle layout and GemRB's
			// CharAnimations::AddMHRSuffix(), the "BG1 character style"
			// handler that always works this way).
			if (_HasBAMVariant(description.bam_name, "G15")) {
				description.bam_name += "G15";
				description.sequence_number += ANIM_DIE_OFFSET;
			} else {
				description.bam_name += "G1";
				description.sequence_number += 48;
			}
			break;
		case ACT_DEAD:
			// If it has G15 it also has G16 - same reasoning as Die above.
			if (_HasBAMVariant(description.bam_name, "G15")) {
				description.bam_name += "G16";
				description.sequence_number += ANIM_DEAD_OFFSET;
			} else {
				// Same file as Die above - the frozen/twitch bank (7).
				description.bam_name += "G1";
				description.sequence_number += 56;
			}
			break;
		case ACT_CAST_SPELL_PREPARE:
			description.bam_name += "CA";
			break;
		case ACT_CAST_SPELL_RELEASE:
			description.bam_name += "CA";
			break;
		default:
			_WarnUnimplementedAction("Character", baseName, actor);
			break;
	}

	if (_GeneratesEasternOrientations()) {
		if (o >= IE::ORIENTATION_EXT_NNE && o <= IE::ORIENTATION_EXT_SSE)
			o = _MirrorExtendedOrientation(o, description);
	} else {
		// This game's Character-style content only ships 8 base facings
		// (+ a dedicated east-facing file) - actor orientation is always
		// extended (see Actor::Orientation()), so fold it back down.
		o = IE::orientation_ext_to_base(o);
		_AppendEasternSuffix(description, o, true);
	}
	description.sequence_number += o;
	return description;
}


// avatars.2da TYPE 1 (ONE_FILE): genuinely native 16-way - no base-8
// folding, no mirror, no East file, and no resref suffix at all (the
// table's own base name is already the complete resref). 5 action
// banks of 16 cycles each (Cycle = bank*16 + Orient), confirmed
// against GemRB's own one_file[] stance table and a real BAM's cycle
// count (SNOMC.BAM: 80 = 5*16).
static animation_description
_BuildOneFile(const std::string& baseName, Actor* actor)
{
	animation_description description;
	description.bam_name = baseName;

	int bank;
	switch (actor->AnimationAction()) {
		case ACT_WALKING:
		case ACT_STANDING:
			bank = 1;
			break;
		case ACT_ATTACKING:
			bank = 2;
			break;
		case ACT_DIE:
			bank = 3;
			break;
		case ACT_DEAD:
			bank = 4;
			break;
		default:
			_WarnUnimplementedAction("OneFile", baseName, actor);
			bank = 1;
			break;
	}
	description.sequence_number = bank * 16 + actor->Orientation();
	return description;
}


// avatars.2da TYPE 3 (TWO_FILES) - small animals (bat/cat/chicken/rat/
// squirrel). A single "G1" file, base-8-folded (Orient/2) cycles with
// a real East file; standing and attacking share one bank (no
// distinct attack pose for these) - confirmed against GemRB's
// AddTwoFileSuffix() and a real BAM's cycle count (ABATG1.BAM: 48 = 6
// banks of 8).
static animation_description
_BuildTwoFiles(const std::string& baseName, Actor* actor)
{
	int halfOrient = actor->Orientation() / 2;
	animation_description description;
	description.bam_name = baseName + "G1";

	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			description.sequence_number = halfOrient;
			break;
		case ACT_STANDING:
		case ACT_ATTACKING:
			description.sequence_number = 8 + halfOrient;
			break;
		case ACT_DIE:
			description.sequence_number = 32 + halfOrient;
			break;
		case ACT_DEAD:
			description.sequence_number = 40 + halfOrient;
			break;
		default:
			_WarnUnimplementedAction("TwoFiles", baseName, actor);
			description.sequence_number = 8 + halfOrient;
			break;
	}

	_AppendEasternSuffix(description, halfOrient, false);
	return description;
}


// avatars.2da TYPE 10 (TWO_FILES_2, "low res bg1 anim") - cow/horse
// only. A single "G1"(+E) file split down the middle by orientation
// (base 0-3 west, 4-7 east - not the usual 5/mirror-of-3 split other
// types use), base-8-folded (Orient/2) cycles. Walk and stand share
// one bank here, unlike most other types - confirmed against GemRB's
// AddLR2Suffix() and real ACOWG1/G1E.BAM cycle counts (40 each = 5
// banks of 8, only half of each meaningfully used per file).
static animation_description
_BuildTwoFiles2(const std::string& baseName, Actor* actor)
{
	int halfOrient = actor->Orientation() / 2;
	animation_description description;
	description.bam_name = baseName + "G1";

	int bank;
	switch (actor->AnimationAction()) {
		case ACT_WALKING:
		case ACT_STANDING:
		case ACT_CAST_SPELL_PREPARE:
		case ACT_CAST_SPELL_RELEASE:
			bank = 0;
			break;
		case ACT_ATTACKING:
			bank = 8;
			break;
		case ACT_DIE:
			bank = 24;
			break;
		case ACT_DEAD:
			bank = 32;
			break;
		default:
			_WarnUnimplementedAction("TwoFiles2", baseName, actor);
			bank = 0;
			break;
	}
	description.sequence_number = bank + halfOrient;
	if (halfOrient >= 4)
		description.bam_name += "E";
	return description;
}


// avatars.2da TYPE 8 (SIX_FILES) - the wyvern (0xA000 specifically;
// the *other* wyvern id, 0x1000, is TYPE 11/FOUR_FRAMES, a multi-part
// body-compositing scheme this codebase has no machinery for yet - not
// covered here). Genuinely native 16-way (no folding, no mirror) split
// across three files: G1 (walk only), G2 (idle/damage/die/twitch banks
// of 16), G3 (attack variants, banks of 16) - confirmed against
// GemRB's AddSixSuffix() and real MWYVG1/G2/G3(E) BAMs' cycle counts
// (16/80/48 = exactly 1/5/3 banks of 16).
static animation_description
_BuildSixFiles(const std::string& baseName, Actor* actor)
{
	int o = actor->Orientation();
	animation_description description;

	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			description.bam_name = baseName + "G1";
			description.sequence_number = o;
			break;
		case ACT_STANDING:
			description.bam_name = baseName + "G2";
			description.sequence_number = 16 + o;
			break;
		case ACT_CAST_SPELL_PREPARE:
		case ACT_CAST_SPELL_RELEASE:
			description.bam_name = baseName + "G2";
			description.sequence_number = o;
			break;
		case ACT_DIE:
			description.bam_name = baseName + "G2";
			description.sequence_number = 48 + o;
			break;
		case ACT_DEAD:
			description.bam_name = baseName + "G2";
			description.sequence_number = 64 + o;
			break;
		case ACT_ATTACKING:
			description.bam_name = baseName + "G3";
			description.sequence_number = o;
			break;
		default:
			_WarnUnimplementedAction("SixFiles", baseName, actor);
			description.bam_name = baseName + "G2";
			description.sequence_number = 16 + o;
			break;
	}

	if (o > 9)
		description.bam_name += "E";
	return description;
}


// avatars.2da TYPE 18 (FOUR_FILES_3): even orientations live in an "H"
// file, odd in "L" ("[NAME][H|L]G1[/E]") - confirmed against a real
// pair's own cycle counts (NBEGHG1.BAM/NBEGLG1.BAM: 48/40 = 6/5 banks
// of 8, matching GemRB's AddHLSuffix() exactly). Walk always lives in H
// regardless of orientation parity - it's the first (and only 8-cycle)
// bank there, which is also why every other H-file stance sits 8 cycles
// further in than its L-file counterpart.
static animation_description
_BuildSplit(const std::string& baseName, Actor* actor)
{
	int o = actor->Orientation();
	animation_description description;
	description.bam_name = baseName;

	bool useH = (o % 2) == 0;
	int halfOrient = o / 2;

	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			useH = true;
			description.sequence_number = halfOrient;
			break;
		case ACT_STANDING:
		case ACT_ATTACKING:
		case ACT_CAST_SPELL_PREPARE:
		case ACT_CAST_SPELL_RELEASE:
			description.sequence_number = (useH ? 16 : 8) + halfOrient;
			break;
		case ACT_DIE:
			description.sequence_number = (useH ? 32 : 24) + halfOrient;
			break;
		case ACT_DEAD:
			description.sequence_number = (useH ? 40 : 32) + halfOrient;
			break;
		default:
			_WarnUnimplementedAction("Split", baseName, actor);
			description.sequence_number = (useH ? 16 : 8) + halfOrient;
			break;
	}

	description.bam_name += useH ? "H" : "L";
	description.bam_name += "G1";
	_AppendEasternSuffix(description, halfOrient, false);
	return description;
}


// avatars.2da TYPE 9 (TWO_FILES_3, "IWD style") - a dedicated file per
// action ("[NAME][ACTIONCODE][/E]"), each just base-8-folded (Orient/2)
// cycles with a real East file - this builder's existing WK/SD/A1
// cases already matched GemRB's AddMMRSuffix() exactly (confirmed
// against real MGO1* BAMs); added the missing Die/Dead/Cast codes the
// same function also defines ("de"/"tw"/"sp"/"ca").
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
		case ACT_CAST_SPELL_PREPARE:
			description.bam_name += "SP";
			break;
		case ACT_CAST_SPELL_RELEASE:
			description.bam_name += "CA";
			break;
		case ACT_DIE:
			description.bam_name += "DE";
			break;
		case ACT_DEAD:
			description.bam_name += "TW";
			break;
		default:
			_WarnUnimplementedAction("IWD", baseName, actor);
			break;
	}
	_AppendEasternSuffix(description, o, false);
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
	try {
		animation_description description = GetAnimationDescription(actor);
		IE::point pos;
		CREColors* colorsToApply = UsesCustomColors(fID) ? colors : nullptr;
		return new Animation(description.bam_name.c_str(),
							description.sequence_number, description.mirror,
							pos, colorsToApply);
	} catch (std::exception& exception) {
		std::cerr << exception.what() << std::endl;
		return NULL;
	} catch (...) {
		return NULL;
	}
}


Animation*
AnimationFactory::WeaponOverlayFor(Actor* actor)
{
	try {
		animation_description description = GetAnimationDescription(actor);
		if (description.weapon_bam_name.empty())
			return NULL;
		IE::point pos;
		return new Animation(description.weapon_bam_name.c_str(),
							description.sequence_number, description.mirror,
							pos, nullptr);
	} catch (std::exception& exception) {
		std::cerr << exception.what() << std::endl;
		return NULL;
	} catch (...) {
		return NULL;
	}
}


/* static */
const char*
AnimationFactory::SizeCodeForActor(const Actor* actor)
{
	// Real BG2 keys this off the avatar animation id via a table
	// hardcoded in the executable (no data file for it) - this is a
	// simplification keyed off race instead, covering every playable PC
	// race. Halflings get their own "H" variant (helmets are the one
	// exception - they fall back to the gnome/"S" files - but this
	// codebase doesn't composite helmets).
	switch (actor->CRE()->Race()) {
		case 5: // HALFLING
			return "H";
		case 6: // GNOME
			return "S";
		default:
			return "M";
	}
}


// Paperdoll resource per animation id and armor level 1-4, from GemRB's
// unhardcoded BG2 pdolls.2da (the original game keeps this table in its
// executable). Level 3/4 of several rows reuse another class' doll: not every
// class x armor combination has a PLT of its own.
struct PaperdollEntry {
	uint16 animation_id;
	const char* names[4];
};

static const PaperdollEntry kPaperdolls[] = {
	{ 0x5000, { "CHMC1INV", "CHMC2INV", "CHMC3INV", "CHMC4INV" } },
	{ 0x5001, { "CEMC1INV", "CEMC2INV", "CEMC3INV", "CEMC4INV" } },
	{ 0x5002, { "CDMC1INV", "CDMC2INV", "CDMC3INV", "CDMC4INV" } },
	{ 0x5003, { "CIMC1INV", "CIMC2INV", "CIMC3INV", "CIMC4INV" } },
	{ 0x5010, { "CHFC1INV", "CHMC2INV", "CHMC3INV", "CHMC4INV" } },
	{ 0x5011, { "CEFC1INV", "CEMC2INV", "CEMC3INV", "CEMC4INV" } },
	{ 0x5012, { "CDMC1INV", "CDMC2INV", "CDMC3INV", "CDMC4INV" } },
	{ 0x5013, { "CIFC1INV", "CIMC2INV", "CIMC3INV", "CIMC4INV" } },
	{ 0x5100, { "CHMF1INV", "CHMF2INV", "CHMF3INV", "CHMC4INV" } },
	{ 0x5101, { "CEMF1INV", "CEMF2INV", "CEMF3INV", "CEMC4INV" } },
	{ 0x5102, { "CDMF1INV", "CDMF2INV", "CDMF3INV", "CDMC4INV" } },
	{ 0x5103, { "CIMF1INV", "CIMF2INV", "CIMF3INV", "CIMC4INV" } },
	{ 0x5110, { "CHFF1INV", "CHFF2INV", "CHMF3INV", "CHMC4INV" } },
	{ 0x5111, { "CEFF1INV", "CEFF2INV", "CEMF3INV", "CEMC4INV" } },
	{ 0x5112, { "CDMF1INV", "CDMF2INV", "CDMF3INV", "CDMC4INV" } },
	{ 0x5113, { "CIFF1INV", "CIFF2INV", "CIMF3INV", "CIMC4INV" } },
	{ 0x5200, { "CHMW1INV", "CHMW2INV", "CHMW3INV", "CHMW4INV" } },
	{ 0x5201, { "CEMW1INV", "CEMW2INV", "CEMW3INV", "CEMW4INV" } },
	{ 0x5202, { "CDMW1INV", "CDMW2INV", "CDMW3INV", "CDMW4INV" } },
	{ 0x5203, { "CDMW1INV", "CDMW2INV", "CDMW3INV", "CDMW4INV" } },
	{ 0x5210, { "CHFW1INV", "CHMW2INV", "CHMW3INV", "CHMW4INV" } },
	{ 0x5211, { "CEFW1INV", "CEMW2INV", "CEMW3INV", "CEMW4INV" } },
	{ 0x5212, { "CDMW1INV", "CDMW2INV", "CDMW3INV", "CDMW4INV" } },
	{ 0x5213, { "CDMW1INV", "CDMW2INV", "CDMW3INV", "CDMW4INV" } },
	{ 0x5300, { "CHMT1INV", "CHMT2INV", "CHMF3INV", "CHMF4INV" } },
	{ 0x5301, { "CEMT1INV", "CEMT2INV", "CEMF3INV", "CEMF4INV" } },
	{ 0x5302, { "CDMT1INV", "CDMT2INV", "CDMF3INV", "CDMF4INV" } },
	{ 0x5303, { "CIMT1INV", "CIMT2INV", "CIMF3INV", "CIMF4INV" } },
	{ 0x5310, { "CHFT1INV", "CHMT2INV", "CHMF3INV", "CHMF4INV" } },
	{ 0x5311, { "CEFT1INV", "CEMT2INV", "CEMF3INV", "CEMF4INV" } },
	{ 0x5312, { "CDMT1INV", "CDMT2INV", "CDMF3INV", "CDMF4INV" } },
	{ 0x5313, { "CIFT1INV", "CIMT2INV", "CIMF3INV", "CIMF4INV" } },
	{ 0x6000, { "CHMC1INV", "CHMC2INV", "CHMC3INV", "CHMC4INV" } },
	{ 0x6001, { "CEMC1INV", "CEMC2INV", "CEMC3INV", "CEMC4INV" } },
	{ 0x6002, { "CDMC1INV", "CDMC2INV", "CDMC3INV", "CDMC4INV" } },
	{ 0x6003, { "CIMC1INV", "CIMC2INV", "CIMC3INV", "CIMC4INV" } },
	{ 0x6004, { "CIMC1INV", "CIMC2INV", "CIMC3INV", "CIMC4INV" } },
	{ 0x6005, { "COMC1INV", "COMC2INV", "COMC3INV", "COMC4INV" } },
	{ 0x6010, { "CHFC1INV", "CHFC2INV", "CHFC3INV", "CHFC4INV" } },
	{ 0x6011, { "CEFC1INV", "CEFC2INV", "CEFC3INV", "CEFC4INV" } },
	{ 0x6012, { "CDMC1INV", "CDMC2INV", "CDMC3INV", "CDMC4INV" } },
	{ 0x6013, { "CIFC1INV", "CIFC2INV", "CIFC3INV", "CIFC4INV" } },
	{ 0x6014, { "CIFC1INV", "CIFC2INV", "CIFC3INV", "CIFC4INV" } },
	{ 0x6015, { "COFC1INV", "COFC2INV", "COFC3INV", "COFC4INV" } },
	{ 0x6100, { "CHMF1INV", "CHMF2INV", "CHMF3INV", "CHMF4INV" } },
	{ 0x6101, { "CEMF1INV", "CEMF2INV", "CEMF3INV", "CEMF4INV" } },
	{ 0x6102, { "CDMF1INV", "CDMF2INV", "CDMF3INV", "CDMF4INV" } },
	{ 0x6103, { "CIMF1INV", "CIMF2INV", "CIMF3INV", "CIMF4INV" } },
	{ 0x6104, { "CIMF1INV", "CIMF2INV", "CIMF3INV", "CIMF4INV" } },
	{ 0x6105, { "COMF1INV", "COMF2INV", "COMF3INV", "COMF4INV" } },
	{ 0x6110, { "CHFF1INV", "CHFF2INV", "CHFF3INV", "CHFF4INV" } },
	{ 0x6111, { "CEFF1INV", "CEFF2INV", "CEFF3INV", "CEFF4INV" } },
	{ 0x6112, { "CDMF1INV", "CDMF2INV", "CDMF3INV", "CDMF4INV" } },
	{ 0x6113, { "CIFF1INV", "CIFF2INV", "CIFF3INV", "CIFF4INV" } },
	{ 0x6114, { "CIFF1INV", "CIFF2INV", "CIFF3INV", "CIFF4INV" } },
	{ 0x6115, { "COFF1INV", "COFF2INV", "COFF3INV", "COFF4INV" } },
	{ 0x6200, { "CHMW1INV", "CHMW2INV", "CHMW3INV", "CHMW4INV" } },
	{ 0x6201, { "CEMW1INV", "CEMW2INV", "CEMW3INV", "CEMW4INV" } },
	{ 0x6202, { "CDMW1INV", "CDMW2INV", "CDMW3INV", "CDMW4INV" } },
	{ 0x6203, { "CDMW1INV", "CDMW2INV", "CDMW3INV", "CDMW4INV" } },
	{ 0x6204, { "CGMW1INV", "CGMW2INV", "CGMW3INV", "CGMW4INV" } },
	{ 0x6205, { "COMW1INV", "COMW2INV", "COMW3INV", "COMW4INV" } },
	{ 0x6210, { "CHFW1INV", "CHFW2INV", "CHFW3INV", "CHFW4INV" } },
	{ 0x6211, { "CEFW1INV", "CEFW2INV", "CEFW3INV", "CEFW4INV" } },
	{ 0x6212, { "CIFT1INV", "CIFT2INV", "CIFT3INV", "CIFT4INV" } },
	{ 0x6213, { "CIFT1INV", "CIFT2INV", "CIFT3INV", "CIFT4INV" } },
	{ 0x6214, { "CIFT1INV", "CIFT2INV", "CIFT3INV", "CIFT4INV" } },
	{ 0x6215, { "COFW1INV", "COFW2INV", "COFW3INV", "COFW4INV" } },
	{ 0x6300, { "CHMT1INV", "CHMT2INV", "CHMF3INV", "CHMF4INV" } },
	{ 0x6301, { "CEMT1INV", "CEMT2INV", "CEMF3INV", "CEMF4INV" } },
	{ 0x6302, { "CDMT1INV", "CDMT2INV", "CDMF3INV", "CDMF4INV" } },
	{ 0x6303, { "CIMT1INV", "CIMT2INV", "CIMF3INV", "CIMF4INV" } },
	{ 0x6304, { "CIMT1INV", "CIMT2INV", "CIMF3INV", "CIMF4INV" } },
	{ 0x6305, { "COMT1INV", "COMT2INV", "COMF3INV", "COMF4INV" } },
	{ 0x6310, { "CHFT1INV", "CHFT2INV", "CHFF3INV", "CHFF4INV" } },
	{ 0x6311, { "CEFT1INV", "CEFT2INV", "CEFF3INV", "CEFF4INV" } },
	{ 0x6312, { "CDMT1INV", "CDMT2INV", "CDMF3INV", "CDMF4INV" } },
	{ 0x6313, { "CIFT1INV", "CIFT2INV", "CIFF3INV", "CIFF4INV" } },
	{ 0x6314, { "CIFT1INV", "CIFT2INV", "CIFF3INV", "CIFF4INV" } },
	{ 0x6315, { "COFT1INV", "COFT2INV", "COFT3INV", "COFT4INV" } },
	{ 0x6402, { "CMNKINV", "CHMT2INV", "CHMF3INV", "CHMF4INV" } },
	{ 0x6500, { "CHMM1INV", "CHMT2INV", "CHMF3INV", "CHMF4INV" } },
	{ 0x6510, { "CHFM1INV", "CHFT2INV", "CHMF3INV", "CHMF4INV" } },
	{ 0x7200, { "MBER0INV", "MBER0INV", "MBER0INV", "MBER0INV" } },
	{ 0x7201, { "MBER1INV", "MBER1INV", "MBER1INV", "MBER1INV" } },
	{ 0x7300, { "MEAEINV", "MEAEINV", "MEAEINV", "MEAEINV" } },
	{ 0x7310, { "MFIEINV", "MFIEINV", "MFIEINV", "MFIEINV" } },
	{ 0x7311, { "MFIEINV", "MFIEINV", "MFIEINV", "MFIEINV" } },
	{ 0x7900, { "MSLI2INV", "MSLI2INV", "MSLI2INV", "MSLI2INV" } },
	{ 0x7902, { "MSLI2INV", "MSLI2INV", "MSLI2INV", "MSLI2INV" } },
	{ 0x7A00, { "MWYVINV", "MWYVINV", "MWYVINV", "MWYVINV" } },
	{ 0x7A03, { "MSPI3INV", "MSPI3INV", "MSPI3INV", "MSPI3INV" } },
	{ 0x7B00, { "MWLF0INV", "MWLF0INV", "MWLF0INV", "MWLF0INV" } },
	{ 0x7B03, { "MWLF0INV", "MWLF0INV", "MWLF0INV", "MWLF0INV" } },
	{ 0x7E00, { "MGWEINV", "MGWEINV", "MGWEINV", "MGWEINV" } },
	{ 0x7E01, { "MGWEINV", "MGWEINV", "MGWEINV", "MGWEINV" } },
	{ 0x7F00, { "MTROINV", "MTROINV", "MTROINV", "MTROINV" } },
	{ 0x7F01, { "MMININV", "MMININV", "MMININV", "MMININV" } },
	{ 0x7F04, { "MIGOINV", "MIGOINV", "MIGOINV", "MIGOINV" } },
	{ 0x7F32, { "MSLYINV", "MSLYINV", "MSLYINV", "MSLYINV" } },
	{ 0x8000, { "MGNLINV", "MGNLINV", "MGNLINV", "MGNLINV" } },
	{ 0x9000, { "MOGRINV", "MOGRINV", "MOGRINV", "MOGRINV" } },
	{ 0xC300, { "ARATINV", "ARATINV", "ARATINV", "ARATINV" } },
	{ 0xC400, { "ASQUINV", "ASQUINV", "ASQUINV", "ASQUINV" } },
	{ 0xE900, { "MSALINV", "MSALINV", "MSALINV", "MSALINV" } },
};

// Armor level 0-3 of the paperdoll: the first character of the worn armor's
// animation code, "1" (no armor) when it isn't a digit 1-4.
static int
_PaperdollLevel(const Actor* actor)
{
	const std::string armor = actor->ArmorAnimation();
	if (!armor.empty() && armor[0] >= '1' && armor[0] <= '4')
		return armor[0] - '1';
	return 0;
}


std::string
AnimationFactory::PaperdollName(const Actor* actor) const
{
	// BG1's paperdolls are BAMs named after the animation, not in this table.
	if (Core::Get()->Game() != game::GAME_BALDURSGATE) {
		for (const PaperdollEntry& entry : kPaperdolls) {
			if (entry.animation_id == fID)
				return entry.names[_PaperdollLevel(actor)];
		}
	}

	std::string name;
	if (actor->InParty())
		name = _CharacterIdentityPrefix(actor, fBaseName, false);
	else
		name = fBaseName + "1";
	name += "INV";
	return name;
}


static const AnimationEntry*
_FindEntry(uint16 animationID)
{
	auto it = std::find_if(std::begin(kAnimationEntries), std::end(kAnimationEntries),
						[animationID] (const AnimationEntry& entry) {
							return entry.animation_id == animationID;
						});
	return it == std::end(kAnimationEntries) ? NULL : it;
}


/* static */
bool
AnimationFactory::UsesCustomColors(uint16 animationID)
{
	const AnimationEntry* entry = _FindEntry(animationID);
	return entry != NULL && entry->custom_colors;
}


animation_description
AnimationFactory::GetAnimationDescription(Actor* actor)
{
	const AnimationEntry* it = _FindEntry(fID);
	if (it == NULL) {
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
