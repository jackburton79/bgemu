/*
 * SpellEffect.cpp
 *
 *  Created on: 7 gen 2022
 *      Author: stefano
 */

#include "SpellEffect.h"

SpellEffect::SpellEffect(const std::string& name)
	:
	fName(name)
{
}

SpellEffect::~SpellEffect()
{
}


std::string
SpellEffect::Name() const
{
	return fName;
}
