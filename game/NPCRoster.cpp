#include "NPCRoster.h"

#include "Actor.h"
#include "AreaRoom.h"
#include "Core.h"
#include "CreResource.h"
#include "GamResource.h"
#include "Party.h"
#include "ResManager.h"
#include "SavedGame.h"

#include <algorithm>
#include <strings.h>


NPCRoster::~NPCRoster()
{
	Clear();
}


uint16
NPCRoster::Count() const
{
	return fNPCs.size();
}


Actor*
NPCRoster::At(uint16 index) const
{
	return index < fNPCs.size() ? fNPCs[index] : NULL;
}


bool
NPCRoster::Contains(const Actor* actor) const
{
	return std::find(fNPCs.begin(), fNPCs.end(), actor) != fNPCs.end();
}


Actor*
NPCRoster::Find(const char* name) const
{
	for (Actor* npc : fNPCs) {
		if (strcasecmp(name, npc->Name()) == 0)
			return npc;
		const CREResource* cre = npc->CRE();
		if (cre != NULL && !cre->DeathVariable().empty()
				&& strcasecmp(name, cre->DeathVariable().c_str()) == 0) {
			return npc;
		}
	}
	return NULL;
}


void
NPCRoster::Add(Actor* actor)
{
	if (actor != NULL)
		fNPCs.push_back(actor);
}


void
NPCRoster::Remove(Actor* actor)
{
	auto i = std::find(fNPCs.begin(), fNPCs.end(), actor);
	if (i != fNPCs.end()) {
		(*i)->Release();
		fNPCs.erase(i);
	}
}


void
NPCRoster::Adopt(Actor* actor)
{
	if (AreaRoom* room = actor->Area())
		room->ForgetPlacedActor(actor);
	actor->Acquire();
	Add(actor);
}


void
NPCRoster::Move(Actor* npc, const res_ref& area, const IE::point& position,
	int orientation)
{
	AreaRoom* room = npc->Area();
	if (room != NULL && strcasecmp(room->Name(), area.CString()) != 0) {
		room->RemoveObject(npc);
		npc->SetArea(NULL);
		// The room's own reference: the list's keeps the NPC alive.
		npc->Release();
	}

	npc->SetAreaName(area);
	npc->SetPosition(position);
	if (orientation >= 0)
		npc->SetOrientation(orientation);

	// Moved into the room that is loaded right now, from an area that isn't.
	if (npc->Area() == NULL) {
		AreaRoom* current = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
		if (current != NULL && strcasecmp(current->Name(), area.CString()) == 0) {
			npc->Acquire();
			current->AddObject(npc);
			npc->SetPosition(position);
		}
	}
}


void
NPCRoster::Load(GamResource* gam, const Party* party)
{
	for (uint32 i = 0; i < gam->OutOfPartyCount(); i++) {
		gam_party_member member = gam->OutOfPartyAt(i);

		// Someone already in the party (a -P override or the default
		// party names the same creatures) can't also be standing
		// somewhere else.
		bool inParty = false;
		for (uint16 p = 0; p < party->CountActors(); p++) {
			if (strcasecmp(party->ActorAt(p)->Name(), member.creName.CString()) == 0)
				inParty = true;
		}
		if (inParty)
			continue;

		Actor* npc = SavedGame::RestoreActor(member, gam->OutOfPartyCRE(i));
		npc->SetAreaName(member.areaName);
		Add(npc);
	}
}


void
NPCRoster::LoadStarting(const Party* party)
{
	// A new game starts from BALDUR.GAM: its out-of-party NPC table says
	// where every companion and story character begins.
	GamResource* gam = gResManager->GetGAM(res_ref("BALDUR"));
	if (gam == NULL)
		return;
	Load(gam, party);
	gResManager->ReleaseResource(gam);
}


void
NPCRoster::Clear()
{
	for (Actor* actor : fNPCs)
		actor->Release();
	fNPCs.clear();
}
