/*
 * GamResource.h
 *
 * Reads/writes the GAM (save game) format - see IESDP gam_v2.0.htm.
 *
 * This engine only models a subset of what a real save actually carries:
 * party composition/position/CRE state, GLOBAL variables, elapsed game
 * time (GameTimer), out-of-party NPCs, journal entries (Game::fJournal - strref, time,
 * chapter and section; the group is kept in the location byte, a GemRB
 * extension) and party reputation (read from the
 * party leader's own CRE - every party member's is kept in sync by
 * REPUTATIONSET/REPUTATIONINC already, see scripting/Actions.cpp - so
 * this is a write-only convenience for external tools inspecting the
 * file; Load() gets it from each member's own embedded CRE like
 * everything else CRE-level, not from this header field). The file this
 * class writes is a structurally valid GAM v2.0 file (correct signature/
 * header/NPC-struct layout, so a real IE tool could parse it), but every
 * field this engine has no matching concept for (formation, weather, GUI
 * flags, familiar info, stored/pocket-plane locations, per-character
 * quick-slots/stats/voice set) is written as zero rather than guessed -
 * see WriteToFile()'s implementation for the exact list.
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
	res_ref quickSpells[3];	// the action bar's quick spell slots
};

struct gam_journal_entry {
	uint32 strref;
	uint32 time;		// game seconds
	uint8 chapter;
	uint8 section;		// journal section bits: 1 quest, 2 completed, 4 info
	uint8 group;
};


class GamResource : public Resource {
public:
	GamResource(const res_ref& name);
	static Resource* Create(const res_ref& name);

	// Building a save (Game::Save() calls these, then WriteToFile()).
	void SetCurrentArea(const res_ref& areaName);
	void AddPartyMember(const gam_party_member& member, const CREResource* cre);
	// Out-of-party NPCs (the GAM's second creature table): same struct
	// as a party member, kept apart from the party by the file layout.
	void AddOutOfPartyMember(const gam_party_member& member, const CREResource* cre);
	void SetVariables(const std::vector<std::pair<std::string, int32>>& variables);
	// seconds: CINGAME, GameTimer::GameTime()'s own unit - converted to
	// the header's native "300 units == 1 hour" on write.
	void SetGameTime(uint32 seconds);
	// The header's separate "real seconds" field - meant to be real-world
	// playtime accumulated across a save's whole lifetime; this engine
	// only tracks it per-session (GameTimer::RealTime(), reset at every
	// process start), so this is an approximation like everything else
	// undermodeled here, not a real accumulator.
	void SetRealTime(uint32 seconds);
	// In display order. `time` is in game seconds here (the file itself
	// counts 300 units to a game hour, converted on write/read).
	void SetJournalEntries(const std::vector<gam_journal_entry>& entries);
	// reputation: plain 0-20 value (CREResource::Reputation()'s own
	// units) - multiplied by 10 on write to match the header field.
	void SetReputation(sint8 reputation);
	bool WriteToFile(const char* path) const;

	// Reading a save (Game::Load() calls LoadFromFile(), then these).
	bool LoadFromFile(const char* path);
	uint32 PartyMemberCount() const;
	gam_party_member PartyMemberAt(uint32 index) const;
	// Loads (constructs) the embedded CRE for the given party member.
	// Caller must gResManager->ReleaseResource() it, same as any other
	// resource fetched via ResourceManager.
	CREResource* PartyMemberCRE(uint32 index) const;
	// Same for the out-of-party NPCs (which is all a new game's
	// BALDUR.GAM carries: the companions and story NPCs and where each
	// starts out).
	uint32 OutOfPartyCount() const;
	gam_party_member OutOfPartyAt(uint32 index) const;
	CREResource* OutOfPartyCRE(uint32 index) const;
	res_ref CurrentArea() const;
	std::vector<std::pair<std::string, int32>> Variables() const;
	uint32 GameTime() const;
	std::vector<gam_journal_entry> JournalEntries() const;

private:
	virtual ~GamResource();

	struct _PendingMember {
		gam_party_member info;
		const CREResource* cre;
	};

	gam_party_member _MemberAt(uint32 structOffset) const;
	CREResource* _MemberCRE(uint32 structOffset) const;

	std::vector<_PendingMember> fPendingMembers;
	std::vector<_PendingMember> fPendingNPCs;
	std::vector<std::pair<std::string, int32>> fPendingVariables;
	std::vector<gam_journal_entry> fPendingJournalEntries;
	uint32 fPendingGameTime = 0;
	uint32 fPendingRealSeconds = 0;
	sint8 fPendingReputation = 0;
	res_ref fPendingArea;
};

#endif // GAMRESOURCE_H_
