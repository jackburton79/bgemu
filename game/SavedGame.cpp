#include "SavedGame.h"

#include "Actor.h"
#include "AreaRoom.h"
#include "Core.h"
#include "CreResource.h"
#include "Game.h"
#include "GameJournal.h"
#include "NPCRoster.h"
#include "GamResource.h"
#include "GameTimer.h"
#include "Party.h"
#include "ResManager.h"
#include "RoomBase.h"
#include "ScreenManager.h"
#include "StoreScreen.h"

#include <filesystem>
#include <iostream>
#include <vector>


SavedGame::SavedGame(Game& game)
	:
	fGame(game)
{
}


void
SavedGame::SetDirectory(const std::string& path)
{
	fDirectory = path;
}


const std::string&
SavedGame::Directory() const
{
	return fDirectory;
}


std::string
SavedGame::SlotPath(uint32 index) const
{
	return fDirectory + "/savegame_slot" + std::to_string(index) + ".gam";
}


// Copies every on-disk area checkpoint from one directory to another
// (creating `to` if needed) - shared by Game::Save() (the live session's
// single checkpoint directory -> this save's own archive) and Game::
// Load() (that archive -> the live session's directory), mirroring
// GemRB's own single-cache-directory approach (see AreaRoom::
// AreaCheckpointDir()'s own comment) with a plain directory copy instead
// of a real archive format.
static void
_CopyAreaCheckpoints(const std::string& from, const std::string& to)
{
	std::error_code error;
	if (!std::filesystem::exists(from, error))
		return;

	std::filesystem::create_directories(to, error);
	std::filesystem::copy(from, to,
		std::filesystem::copy_options::recursive
			| std::filesystem::copy_options::overwrite_existing,
		error);
}


bool
SavedGame::Save(const char* name)
{
	::Party* party = fGame.Party();
	if (party == NULL || party->CountActors() == 0) {
		std::cerr << "SavedGame::Save(): no party to save" << std::endl;
		return false;
	}

	RoomBase* room = Core::Get()->CurrentRoom();
	res_ref areaName(room != NULL ? room->Name() : "");

	// Snapshot the area actually being stood in too - _UnloadArea() only
	// checkpoints an area once it's actually left, so without this a save
	// made without ever having left the current area since arriving would
	// come back pristine on a later load.
	if (AreaRoom* areaRoom = dynamic_cast<AreaRoom*>(room))
		areaRoom->WriteCheckpoint();

	// Every area checkpointed at any point this session - not just the
	// current one - lives in AreaRoom's single, session-long checkpoint
	// directory (see its own comment): copy all of it into this save's
	// own archive directory, so an area visited and left long before
	// this save, and never revisited since, is captured too.
	fGame.SaveAreaMapFlags(AreaRoom::AreaCheckpointDir() + "/worldmap.txt");
	_CopyAreaCheckpoints(AreaRoom::AreaCheckpointDir(), std::string(name) + ".arecache");

	GamResource* gam = new GamResource(res_ref("SAVE"));
	gam->SetCurrentArea(areaName);

	for (uint16 i = 0; i < party->CountActors(); i++) {
		Actor* actor = party->ActorAt(i);

		gam->AddPartyMember(MemberFor(actor, areaName), actor->CRE());
	}

	for (uint16 i = 0; i < fGame.NPCs().Count(); i++) {
		Actor* npc = fGame.NPCs().At(i);
		gam->AddOutOfPartyMember(MemberFor(npc, npc->AreaName()), npc->CRE());
	}

	gam->SetVariables(Core::Get()->Vars().All());
	gam->SetGameTime(GameTimer::GameTime());
	gam->SetRealTime(GameTimer::RealTime());
	std::vector<gam_journal_entry> journal;
	for (const journal_entry& entry : fGame.Journal().Entries())
		journal.push_back({ entry.strref, entry.time, entry.chapter, entry.section, entry.group });
	gam->SetJournalEntries(journal);
	// Every party member's own reputation byte is kept in sync by
	// REPUTATIONSET/REPUTATIONINC (scripting/Actions.cpp) - the leader's
	// is as good as any (see GamResource.h's own comment on why this is
	// write-only).
	gam->SetReputation(party->ActorAt(0)->CRE()->Reputation());

	bool result = gam->WriteToFile(name);
	// `gam` is a runtime-built Resource (key 0, never went through
	// GetResource()/the cache) - plain Release() would leak it, since
	// nothing else holds a reference to drop it to 0 and Resource's own
	// destructor is protected (only ResourceManager can call it).
	gResManager->ReleaseResource(gam);
	return result;
}


bool
SavedGame::Load(const char* name)
{
	GamResource* gam = new GamResource(res_ref("SAVE"));
	if (!gam->LoadFromFile(name)) {
		gResManager->ReleaseResource(gam);
		return false;
	}

	// Abandon whatever's currently loaded - a load restores *that save's*
	// world, not whatever the live session still happens to be holding
	// onto (see _ClearAreaCache()'s own comment below). Its own on-unload
	// checkpoint write still happens (into the live session's single
	// checkpoint directory, see AreaRoom::AreaCheckpointDir()), but that's
	// harmless: the wipe-and-restore right below discards it along with
	// everything else already there, unconditionally.
	Core::Get()->UnloadCurrentRoom();

	std::error_code error;
	std::filesystem::remove_all(AreaRoom::AreaCheckpointDir(), error);
	_CopyAreaCheckpoints(std::string(name) + ".arecache", AreaRoom::AreaCheckpointDir());
	fGame.LoadAreaMapFlags(AreaRoom::AreaCheckpointDir() + "/worldmap.txt");

	// The in-memory session cache (live Actor/ARAResource C++ objects
	// from areas visited earlier this session) is separate from the
	// on-disk checkpoints just restored above, and isn't reset by
	// replacing them - drop it too, or a revisited area would resurrect
	// this abandoned session's own state instead of reading back what
	// was just restored from disk.
	fGame.ClearAreaCache();
	// Same for a store's stock: the loaded save's world starts from the
	// stores' original stock.
	fGame.Screens().Find<StoreScreen>()->ClearStores();

	::Party* party = fGame.ResetParty();

	fGame.NPCs().Clear();

	uint32 count = gam->PartyMemberCount();
	std::vector<IE::point> savedPositions;
	for (uint32 i = 0; i < count; i++) {
		gam_party_member member = gam->PartyMemberAt(i);

		Actor* actor = RestoreActor(member, gam->PartyMemberCRE(i));
		party->AddActor(actor);
		savedPositions.push_back(member.position);
	}

	fGame.NPCs().Load(gam, party);

	for (const auto& variable : gam->Variables())
		Core::Get()->Vars().Set(variable.first.c_str(), variable.second);

	GameTimer::SetGameTime(gam->GameTime());
	std::vector<journal_entry> journal;
	for (const gam_journal_entry& entry : gam->JournalEntries())
		journal.push_back({ entry.strref, entry.section, entry.group, entry.chapter, entry.time });
	fGame.Journal().Set(journal);

	res_ref area = gam->CurrentArea();
	gResManager->ReleaseResource(gam);
	if (!Core::Get()->LoadArea(area, "", ""))
		return false;

	// LoadArea() above builds a fresh AreaRoom, which - having no real
	// entrance name to go on, a load isn't an actual area transition -
	// parks every party member at that area's own EntranceAt(0) instead
	// (see AreaRoom::AreaRoom()'s own per-member spawn loop). Put them
	// back where this save actually had them; each Actor() above already
	// got its saved position as a constructor argument, but that's long
	// since been overwritten by the entrance-repositioning above.
	for (uint16 i = 0; i < party->CountActors() && i < savedPositions.size(); i++)
		party->ActorAt(i)->SetPosition(savedPositions[i]);

	if (Actor* leader = party->ActorAt(0)) {
		if (RoomBase* room = Core::Get()->CurrentRoom())
			room->SetAreaOffsetCenter(leader->Position());
	}

	return true;
}


Actor*
SavedGame::RestoreActor(const gam_party_member& member, CREResource* savedCre)
{
	// A character made in character creation ("PLAYER1") has no CRE among the
	// game's files - it was only ever injected into the running game, so a game
	// started afresh doesn't know it. Its saved CRE takes that place.
	// The CRE from the GAM has no reference yet; ours is released below, the
	// resource manager keeps its own if it holds the CRE.
	if (savedCre != nullptr) {
		savedCre->Acquire();
		if (!gResManager->ResourceExists(member.creName, RES_CRE))
			gResManager->InjectResource(member.creName, RES_CRE, savedCre);
	}

	// Actor()'s normal constructor fetches the character's original,
	// unmodified CRE from the game's own files (ResourceManager) - this
	// reuses all of Actor's existing init logic (animation factory,
	// etc.) safely. The saved CRE state (inventory, spellbook, HP,
	// status, ...) is then applied on top of it.
	Actor* actor = new Actor(member.creName.CString(), member.position,
		member.orientation);

	if (savedCre != nullptr) {
		// A created character's CRE is the injected one itself.
		if (actor->CRE() != savedCre)
			actor->CRE()->CopyDataFrom(savedCre);
		actor->RefreshColors();
		gResManager->ReleaseResource(savedCre);
	}

	for (uint32 q = 0; q < Actor::kNumQuickSpells; q++)
		actor->SetQuickSpell(q, member.quickSpells[q]);
	actor->Stats() = member.stats;
	actor->SetNumTimesTalkedTo(member.talkCount);

	return actor;
}


gam_party_member
SavedGame::MemberFor(Actor* actor, const res_ref& areaName)
{
	gam_party_member member;
	member.creName = res_ref(actor->Name());
	member.name = actor->Name();
	member.position = actor->Position();
	member.orientation = (uint16)actor->Orientation();
	member.areaName = areaName;
	for (uint32 q = 0; q < Actor::kNumQuickSpells; q++)
		member.quickSpells[q] = actor->QuickSpell(q);
	member.stats = actor->Stats();
	member.talkCount = actor->NumTimesTalkedTo();
	return member;
}
