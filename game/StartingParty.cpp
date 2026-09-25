#include "StartingParty.h"

#include "Actor.h"
#include "CharGenData.h"
#include "Core.h"
#include "CreResource.h"
#include "MemoryStream.h"
#include "Party.h"
#include "ResManager.h"

#include <fstream>
#include <strings.h>
#include <iostream>
#include <sstream>


void
StartingParty::SetMembers(const std::vector<std::string>& names)
{
	fMembers = names;
}


void
StartingParty::SetCharacterSpec(const char* path)
{
	fCharacterSpec = path != NULL ? path : "";
}


void
StartingParty::UseBuilder(bool use)
{
	fUseBuilder = use;
}


CharacterBuilder&
StartingParty::Builder()
{
	return fBuilder;
}


::Party*
StartingParty::Create()
{
	IE::point point = { 20, 20 };
	::Party* party = new ::Party();

	if (fUseBuilder && _CreatePlayer(party, point))
		return party;

	if (!fCharacterSpec.empty() && _CreateFromSpec(party, point))
		return party;

	if (!fMembers.empty()) {
		for (const std::string& name : fMembers)
			party->AddActor(new Actor(name.c_str(), point, 0));
		return party;
	}

	// No -P/--party override given: fall back to the original hardcoded
	// default party. TODO: a real character-creation flow (race/class/
	// stats/kit) is still missing - this is only "pick existing CREs to
	// start with", not "create a character from scratch".
	if (Core::Get()->Game() == game::GAME_BALDURSGATE)
		party->AddActor(new Actor("AJANTI", point, 0));
	else
		party->AddActor(new Actor("ANOMEN10", point, 0));
	return party;
}


bool
StartingParty::_CreateFromSpec(::Party* party, const IE::point& position)
{
	std::ifstream file(fCharacterSpec);
	if (!file) {
		std::cerr << "character spec: cannot open " << fCharacterSpec << std::endl;
		return false;
	}

	CharacterBuilder& builder = fBuilder;
	builder.Reset();
	bool doRoll = false;
	bool doStartingKit = false;
	static const char* kAbilityNames[] = { "str", "dex", "con", "int", "wis", "chr" };

	std::string line;
	while (std::getline(file, line)) {
		size_t hash = line.find('#');
		if (hash != std::string::npos)
			line.erase(hash);
		std::istringstream stream(line);
		std::string field, value;
		if (!(stream >> field))
			continue;
		stream >> value;
		for (char& c : field) c = (char)tolower((unsigned char)c);

		if (field == "roll") {
			doRoll = true;
		} else if (field == "startingkit") {
			// The finish of the character creation: gold, quarterstaff...
			doStartingKit = true;
		} else if (field == "gender") {
			builder.SetGender(value);
		} else if (field == "race") {
			builder.SetRace(value);
		} else if (field == "class") {
			builder.SetClass(value);
		} else if (field == "kit") {
			builder.SetKit(value);
		} else if (field == "alignment") {
			builder.SetAlignment(value);
		} else if (field == "name") {
			// The rest of the line, so names can contain spaces.
			std::string rest;
			std::getline(stream, rest);
			builder.SetName(value + rest);
		} else if (field == "portrait") {
			builder.SetPortraits(value + "S", value + "M");
		} else if (field == "portrait_small") {
			builder.SetPortraits(value, builder.PortraitLarge());
		} else if (field == "portrait_large") {
			builder.SetPortraits(builder.PortraitSmall(), value);
		} else if (field.rfind("color_", 0) == 0) {
			builder.SetColor(field.substr(6), atoi(value.c_str()));
		} else if (field == "spell") {
			if (!builder.AddSpell(value))
				std::cerr << "character spec: unknown spell " << value << std::endl;
		} else if (field == "strextra") {
			builder.SetStrengthExtra(atoi(value.c_str()));
		} else if (field == "prof") {
			// prof <WEAPON> <stars>, e.g. "prof LARGE_SWORD 2".
			int stars = 0;
			stream >> stars;
			size_t count = 0;
			const CharGenProficiency* profs = CharGenData::Proficiencies(count);
			bool found = false;
			for (size_t i = 0; i < count; i++) {
				if (strcasecmp(profs[i].name, value.c_str()) != 0)
					continue;
				found = true;
				if (!builder.SetProficiency((int)i, stars))
					std::cerr << "character spec: " << value << " can't have "
						<< stars << " stars" << std::endl;
			}
			if (!found)
				std::cerr << "character spec: unknown proficiency " << value << std::endl;
		} else if (field.rfind("skill_", 0) == 0) {
			// skill_pickpockets, skill_openlocks, skill_findtraps, skill_stealth
			if (!builder.SetThiefSkill(field.substr(6), atoi(value.c_str())))
				std::cerr << "character spec: unknown skill " << field << std::endl;
		} else {
			for (int i = 0; i < CharacterBuilder::kNumAbilities; i++) {
				if (field == kAbilityNames[i])
					builder.SetAbility(i, atoi(value.c_str()));
			}
		}
	}

	if (doRoll)
		builder.RollAbilities();
	if (doStartingKit)
		builder.ApplyStartingKit();

	return _CreatePlayer(party, position);
}


bool
StartingParty::_CreatePlayer(::Party* party, const IE::point& position)
{
	CharacterBuilder& builder = fBuilder;
	std::vector<std::string> problems;
	if (!builder.IsComplete(problems)) {
		std::cerr << "character spec: incomplete -" << std::endl;
		for (const std::string& p : problems)
			std::cerr << "  " << p << std::endl;
		return false;
	}

	std::vector<uint8> creData;
	if (!builder.BuildCREData(creData))
		return false;

	MemoryStream stream(creData.data(), creData.size(), false);
	CREResource* cre = new CREResource(res_ref("PLAYER1"));
	cre->Acquire(); // resources start at refcount 0
	if (!cre->Load(&stream, 0, creData.size())) {
		gResManager->ReleaseResource(cre); // 1 -> 0, deleted
		return false;
	}
	cre->Init();
	gResManager->InjectResource(res_ref("PLAYER1"), RES_CRE, cre);
	gResManager->ReleaseResource(cre); // drop our ref; InjectResource holds its own

	Actor* player = new Actor("PLAYER1", position, 0);
	if (!builder.Name().empty())
		player->SetLongName(builder.Name().c_str());
	party->AddActor(player);

	// The gold the character starts with is the party's.
	if (builder.Gold() > 0)
		Core::Get()->AddPartyGold(builder.Gold());

	std::cout << "Created character:" << std::endl;
	builder.Print();
	return true;
}
