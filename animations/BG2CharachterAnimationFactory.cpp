/*
 * BGAnimationFactory.cpp
 *
 *  Created on: 16/nov/2014
 *      Author: stefano
 */

#include "BG2CharachterAnimationFactory.h"

#include "Actor.h"
#include "Animation.h"
#include "CreResource.h"

BG2CharachterAnimationFactory::BG2CharachterAnimationFactory(
		const char* baseName, const uint16 id)
	:
	AnimationFactory(baseName, id)
{
}


BG2CharachterAnimationFactory::~BG2CharachterAnimationFactory()
{
}


/* virtual */
animation_description
BG2CharachterAnimationFactory::GetAnimationDescription(Actor* actor)
{
	//std::cout << "BG2AnimationFactory::AnimationFor" << std::endl;
	int o = actor->Orientation();
	animation_description description;
	description.sequence_number = o;
	description.mirror = false;

	// TODO: In theory, fBaseName should already be correct, but in
	// practice, it's not
	if (actor->InParty()) {
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
		std::string armorAnimation = actor->ArmorType();
		std::cout << armorAnimation << std::endl;
		// TODO: Correct ? Fighters seems always to have full plate
		if (description.bam_name[3] == 'F')
			description.bam_name.append("4");
		else
			description.bam_name.append(armorAnimation.substr(0, 1));
	} else {
		description.bam_name = fBaseName;
		description.bam_name.append("1");
	}
	switch (actor->AnimationAction()) {
		case ACT_WALKING:
			description.bam_name.append("G11");
			break;
		case ACT_STANDING:
			description.bam_name.append("G1");
			description.sequence_number = o + 9;
			break;
		case ACT_ATTACKING:
			description.bam_name.append("A1");
			break;
		case ACT_DEAD:
			description.bam_name.append("G15");
			description.sequence_number = 48 + o;
			break;
		case ACT_CAST_SPELL:
			std::cout << "CAST SPELL: " << fBaseName << std::endl;
			description.bam_name.append("C1");
			//description.sequence_number = o + 9;
			break;
		default:
			std::cerr << "BG2CharachterAnimationFactory::GetAnimationDescription(): UNIMPLEMENTED ";
			std::cerr << fBaseName << ", action " << actor->AnimationAction() << ", orientation " << o << std::endl;
			break;
	}
	if (o >= IE::ORIENTATION_EXT_NNE && o <= IE::ORIENTATION_EXT_SSE) {
		description.mirror = true;
		description.sequence_number -= (o - 8) * 2;
	}

	std::cout << description.bam_name << std::endl;
	return description;
}


std::string
BG2CharachterAnimationFactory::_RaceCharacter(uint8 race) const
{
	std::cout << "race: " << (int)race << std::endl;
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
BG2CharachterAnimationFactory::_ClassCharacter(uint8 c) const
{
	std::cout << "class: " << (int)c << std::endl;
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
			return "B";
		default:
			return "B";
	}
}


std::string
BG2CharachterAnimationFactory::_GenderCharacter(uint8 gender) const
{
	switch (gender) {
		case 2:
			return "F";
		case 1:
		default:
			return "M";
	}
}
