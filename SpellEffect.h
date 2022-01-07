/*
 * SpellEffect.h
 *
 *  Created on: 7 gen 2022
 *      Author: Stefano Ceccherini
 */

#ifndef SPELLEFFECT_H_
#define SPELLEFFECT_H_

#include <string>

class SpellEffect {
public:
	SpellEffect(const std::string& name);
	~SpellEffect();

	std::string Name() const;

private:
	std::string fName;
};

#endif /* SPELLEFFECT_H_ */
