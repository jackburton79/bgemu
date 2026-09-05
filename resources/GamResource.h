/*
 * GamResource.h
 *
 * Reads/writes the GAM (save game) format - see IESDP gam_v2.0.htm.
 *
 * This engine only models a subset of what a real save actually carries
 * (party composition/position/CRE state and GLOBAL variables - the only
 * two kinds of persistent state that exist anywhere in this codebase
 * today). The file this class writes is a structurally valid GAM v2.0
 * file (correct signature/header/NPC-struct layout, so a real IE tool
 * could parse it), but every field this engine has no matching concept
 * for (formation, weather, GUI flags, journal entries, familiar info,
 * stored/pocket-plane locations, per-character quick-slots/stats/voice
 * set) is written as zero rather than guessed - see WriteToFile()'s
 * implementation for the exact list.
 */

#ifndef GAMRESOURCE_H_
#define GAMRESOURCE_H_

#include "Resource.h"

#include <string>
#include <utility>
#include <vector>

class CREResource;

// One party member, as written to (or read from) a save file.
struct gam_party_member {
	res_ref creName;	// the CRE resource this member was loaded from
	std::string name;	// script/object name (truncated to 8 bytes on write)
	IE::point position;
	uint16 orientation;
	res_ref areaName;
};

class GamResource : public Resource {
public:
	GamResource(const res_ref& name);
	static Resource* Create(const res_ref& name);

	// Building a save (Game::Save() calls these, then WriteToFile()).
	void SetCurrentArea(const res_ref& areaName);
	void AddPartyMember(const gam_party_member& member, const CREResource* cre);
	void SetVariables(const std::vector<std::pair<std::string, int32>>& variables);
	bool WriteToFile(const char* path) const;

	// Reading a save (Game::Load() calls LoadFromFile(), then these).
	bool LoadFromFile(const char* path);
	uint32 PartyMemberCount() const;
	gam_party_member PartyMemberAt(uint32 index) const;
	// Loads (constructs) the embedded CRE for the given party member.
	// Caller must gResManager->ReleaseResource() it, same as any other
	// resource fetched via ResourceManager.
	CREResource* PartyMemberCRE(uint32 index) const;
	res_ref CurrentArea() const;
	std::vector<std::pair<std::string, int32>> Variables() const;

private:
	virtual ~GamResource();

	struct _PendingMember {
		gam_party_member info;
		const CREResource* cre;
	};

	std::vector<_PendingMember> fPendingMembers;
	std::vector<std::pair<std::string, int32>> fPendingVariables;
	res_ref fPendingArea;
};

#endif // GAMRESOURCE_H_
