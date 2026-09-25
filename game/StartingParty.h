#pragma once

#include "CharacterBuilder.h"
#include "IETypes.h"

#include <string>
#include <vector>

class Party;


// The party a new game starts with: the hardcoded default (Ajanti in BG1,
// Anomen in BG2), the creatures named with --party, or - with a character
// spec file (--character) - a protagonist built from scratch by the
// CharacterBuilder and injected as "PLAYER1".
class StartingParty {
public:
	// Overrides the hardcoded default with this list of CRE resrefs (loaded
	// in order, all at the same default spawn point) - set from the command
	// line (see bgemu.cpp's --party option). Leaving this empty keeps the
	// default.
	void SetMembers(const std::vector<std::string>& names);
	// Path to a character-creation spec file. When set, Create() builds the
	// party leader from it (via CharacterBuilder), injects the resulting CRE
	// as "PLAYER1", and starts with that instead of the default party.
	void SetCharacterSpec(const char* path);

	// Create() starts with the character the builder holds (made in character
	// creation), once it is complete, instead of the default party.
	void UseBuilder(bool use);

	// A new party; the caller owns it.
	::Party* Create();

	// Headless character-creation state, also driven by the Char-* console
	// commands.
	CharacterBuilder& Builder();

private:
	// Parses the spec into the builder, builds the CRE, injects it as
	// "PLAYER1", and adds it to `party`. Returns false (and adds nothing) on
	// any parse/build failure.
	bool _CreateFromSpec(::Party* party, const IE::point& position);
	// Builds the CRE from the builder's choices, injects it as "PLAYER1" and
	// adds the character to `party`; false if the builder isn't complete.
	bool _CreatePlayer(::Party* party, const IE::point& position);

	std::vector<std::string> fMembers;
	std::string fCharacterSpec;
	CharacterBuilder fBuilder;
	bool fUseBuilder = false;
};
