/*
 * GamResource.cpp
 */

#include "GamResource.h"

#include "CreResource.h"
#include "FileStream.h"
#include "MemoryStream.h"

#include <cstring>
#include <iostream>


#define GAM_SIGNATURE "GAME"
#define GAM_VERSION_2_0 "V2.0"

static const uint32 kHeaderSize = 0xB4;
static const uint32 kNPCStructSize = 0x160;
static const uint32 kVariableSize = 0x54;
static const uint32 kJournalEntrySize = 0x0c;
// "0x0008 4 (dword) Game time (300 units == 1 hour)" - GameTimer's own
// CINGAME clock (see GameTimer.h) is tracked in plain seconds instead
// (3600 seconds/hour), hence the /12 and *12 conversions below.
static const uint32 kGameTimeUnitsPerHour = 300;
static const uint32 kSecondsPerHour = 3600;


/* static */
Resource*
GamResource::Create(const res_ref& name)
{
	return new GamResource(name);
}


GamResource::GamResource(const res_ref& name)
	:
	Resource(name, RES_GAM)
{
}


GamResource::~GamResource()
{
}


void
GamResource::SetCurrentArea(const res_ref& areaName)
{
	fPendingArea = areaName;
}


void
GamResource::AddPartyMember(const gam_party_member& member, const CREResource* cre)
{
	_PendingMember pending;
	pending.info = member;
	pending.cre = cre;
	fPendingMembers.push_back(pending);
}


void
GamResource::AddOutOfPartyMember(const gam_party_member& member, const CREResource* cre)
{
	_PendingMember pending;
	pending.info = member;
	pending.cre = cre;
	fPendingNPCs.push_back(pending);
}


void
GamResource::SetVariables(const std::vector<std::pair<std::string, int32>>& variables)
{
	fPendingVariables = variables;
}


void
GamResource::SetGameTime(uint32 seconds)
{
	fPendingGameTime = seconds;
}


void
GamResource::SetRealTime(uint32 seconds)
{
	fPendingRealSeconds = seconds;
}


void
GamResource::SetJournalEntries(const std::vector<gam_journal_entry>& entries)
{
	fPendingJournalEntries = entries;
}


void
GamResource::SetReputation(sint8 reputation)
{
	fPendingReputation = reputation;
}


// Writes one NPC struct (party member or out-of-party NPC - same layout)
// and its embedded CRE.
static void
write_member(MemoryStream& buffer, uint32 structOffset, uint16 order,
	uint32 creOffset, const gam_party_member& info, const CREResource* cre)
{
	uint32 creSize = cre->DataSize();

	uint16 selection = 0;
	buffer.WriteAt(structOffset + 0x00, &selection, sizeof(selection));
	buffer.WriteAt(structOffset + 0x02, &order, sizeof(order));
	buffer.WriteAt(structOffset + 0x04, &creOffset, sizeof(creOffset));
	buffer.WriteAt(structOffset + 0x08, &creSize, sizeof(creSize));
	char name[8];
	memset(name, 0, sizeof(name));
	memcpy(name, info.name.c_str(), sizeof(name));
	buffer.WriteAt(structOffset + 0x0c, name, sizeof(name));
	uint32 orientation = info.orientation;
	buffer.WriteAt(structOffset + 0x14, &orientation, sizeof(orientation));
	buffer.WriteAt(structOffset + 0x18, &info.areaName, sizeof(res_ref));
	uint16 x = (uint16)info.position.x;
	uint16 y = (uint16)info.position.y;
	buffer.WriteAt(structOffset + 0x20, &x, sizeof(x));
	buffer.WriteAt(structOffset + 0x22, &y, sizeof(y));
	// Quick spells 1-3 (8-byte resrefs from 0x9c); the rest of 0x24
	// onward (happiness, quick weapon/item slots, character stats,
	// voice set) is left zeroed, not modeled by this engine - quick
	// items are just the CRE's own quick item slots.
	for (int q = 0; q < 3; q++)
		buffer.WriteAt(structOffset + 0x9c + q * 8, &info.quickSpells[q], sizeof(res_ref));

	cre->WriteDataTo(&buffer, creOffset);
}


bool
GamResource::WriteToFile(const char* path) const
{
	const uint32 partyCount = (uint32)fPendingMembers.size();
	const uint32 npcCount = (uint32)fPendingNPCs.size();

	uint32 partyOffset = kHeaderSize;
	uint32 npcOffset = partyOffset + kNPCStructSize * partyCount;
	uint32 creOffset = npcOffset + kNPCStructSize * npcCount;

	uint32 totalCreSize = 0;
	for (const _PendingMember& member : fPendingMembers)
		totalCreSize += member.cre->DataSize();
	for (const _PendingMember& member : fPendingNPCs)
		totalCreSize += member.cre->DataSize();

	uint32 variablesOffset = creOffset + totalCreSize;
	uint32 journalOffset = variablesOffset + kVariableSize * fPendingVariables.size();
	uint32 totalSize = journalOffset + kJournalEntrySize * fPendingJournalEntries.size();

	MemoryStream buffer(totalSize);
	// MemoryStream(size) doesn't zero-initialize - every field this
	// engine doesn't model (formation, weather, GUI flags, quick-slots,
	// character stats, voice set, ...) relies on starting at 0.
	memset(buffer.Data(), 0, totalSize);

	// Header - see IESDP gam_v2.0. Fields this engine has no matching
	// concept for (formation, weather, GUI flags, familiar, stored/
	// pocket-plane locations, party gold pool)
	// are left zeroed rather than guessed.
	buffer.WriteAt(0x00, GAM_SIGNATURE, 4);
	buffer.WriteAt(0x04, GAM_VERSION_2_0, 4);
	// CINGAME, converted from GameTimer's own seconds to this field's
	// native "300 units == 1 hour".
	uint32 gameTimeUnits = fPendingGameTime * kGameTimeUnitsPerHour / kSecondsPerHour;
	buffer.WriteAt(0x08, &gameTimeUnits, sizeof(gameTimeUnits));
	buffer.WriteAt(0x1c, &partyCount, sizeof(uint16)); // count excl. protagonist (informational only)
	buffer.WriteAt(0x20, &partyOffset, sizeof(partyOffset));
	buffer.WriteAt(0x24, &partyCount, sizeof(partyCount));
	// 0x28/0x2c party inventory offset/count: no shared party-level
	// inventory exists in this engine (only per-CRE inventory - see
	// Actor::AddItem() from Phase 1), left as 0/0.
	buffer.WriteAt(0x30, &npcOffset, sizeof(npcOffset)); // out-of-party NPCs
	buffer.WriteAt(0x34, &npcCount, sizeof(npcCount));
	buffer.WriteAt(0x38, &variablesOffset, sizeof(variablesOffset));
	uint32 varCount = (uint32)fPendingVariables.size();
	buffer.WriteAt(0x3c, &varCount, sizeof(varCount));
	buffer.WriteAt(0x40, &fPendingArea, sizeof(res_ref)); // Main area
	uint32 journalCount = (uint32)fPendingJournalEntries.size();
	buffer.WriteAt(0x4c, &journalCount, sizeof(journalCount));
	buffer.WriteAt(0x50, &journalOffset, sizeof(journalOffset));
	// "(*10)" per IESDP - real reputation is always a whole 0-20 number
	// (see CREResource::Reputation()'s own comment), so this is always a
	// multiple of 10 in practice, same as real IE's own saves.
	int32 reputationX10 = (int32)fPendingReputation * 10;
	buffer.WriteAt(0x54, &reputationX10, sizeof(reputationX10));
	buffer.WriteAt(0x58, &fPendingArea, sizeof(res_ref)); // Current area
	// "Game time (real seconds)" - unlike 0x08's CINGAME clock (only
	// advances via explicit time-skips), this is meant to be real-world
	// elapsed playtime; see SetRealTime()'s own comment for what this
	// engine actually has to offer it.
	buffer.WriteAt(0x74, &fPendingRealSeconds, sizeof(fPendingRealSeconds));

	// NPC structs (party first, then out-of-party) + embedded CRE data.
	uint32 currentCreOffset = creOffset;
	for (uint32 i = 0; i < partyCount; i++) {
		const _PendingMember& member = fPendingMembers[i];
		write_member(buffer, partyOffset + i * kNPCStructSize, (uint16)i,
			currentCreOffset, member.info, member.cre);
		currentCreOffset += member.cre->DataSize();
	}
	for (uint32 i = 0; i < npcCount; i++) {
		const _PendingMember& member = fPendingNPCs[i];
		write_member(buffer, npcOffset + i * kNPCStructSize, 0xffff,
			currentCreOffset, member.info, member.cre);
		currentCreOffset += member.cre->DataSize();
	}

	// GLOBAL variables - only the fields real IE itself reads (name +
	// dword-bit type + int value), matching Variables' own int-only
	// model (see IESDP gam_v2.0's own note: "the engine implementation
	// only reads and writes INT variables").
	for (uint32 i = 0; i < fPendingVariables.size(); i++) {
		uint32 varOffset = variablesOffset + i * kVariableSize;
		char name[32];
		memset(name, 0, sizeof(name));
		strncpy(name, fPendingVariables[i].first.c_str(), sizeof(name) - 1);
		buffer.WriteAt(varOffset + 0x00, name, sizeof(name));
		uint16 type = 0x20; // dword value
		buffer.WriteAt(varOffset + 0x20, &type, sizeof(type));
		int32 value = fPendingVariables[i].second;
		buffer.WriteAt(varOffset + 0x24, &value, sizeof(value)); // dword value
		buffer.WriteAt(varOffset + 0x28, &value, sizeof(value)); // int value (same)
	}

	// Journal entries: strref, time (file units, 300 to a game hour),
	// chapter, section bits and - a GemRB extension in place of the location
	// flag - the entry's group. 0x09 (read by character) stays zero.
	for (uint32 i = 0; i < fPendingJournalEntries.size(); i++) {
		const gam_journal_entry& entry = fPendingJournalEntries[i];
		uint32 entryOffset = journalOffset + i * kJournalEntrySize;
		buffer.WriteAt(entryOffset + 0x00, &entry.strref, sizeof(uint32));
		uint32 units = entry.time / (kSecondsPerHour / kGameTimeUnitsPerHour);
		buffer.WriteAt(entryOffset + 0x04, &units, sizeof(units));
		buffer.WriteAt(entryOffset + 0x08, &entry.chapter, sizeof(uint8));
		buffer.WriteAt(entryOffset + 0x0a, &entry.section, sizeof(uint8));
		buffer.WriteAt(entryOffset + 0x0b, &entry.group, sizeof(uint8));
	}

	// FileStream's constructor throws on failure (e.g. the save directory
	// doesn't exist) rather than leaving an invalid stream to check.
	try {
		FileStream file(path, FileStream::WRITE_ONLY | FileStream::CREATE);
		// FileStream doesn't implement WriteAt() (only the base Stream's
		// version, which throws) - Write() is fine here since this is a
		// single sequential dump of a freshly created file.
		file.Write(buffer.Data(), buffer.Size());
	} catch (std::exception& e) {
		std::cerr << "GamResource::WriteToFile(" << path << "): " << e.what() << std::endl;
		return false;
	}
	return true;
}


bool
GamResource::LoadFromFile(const char* path)
{
	try {
		FileStream file(path, FileStream::READ_ONLY);
		delete fData;
		fData = new MemoryStream(file.Size());
		file.ReadAt(0, fData->Data(), file.Size());
	} catch (std::exception& e) {
		std::cerr << "GamResource::LoadFromFile(" << path << "): " << e.what() << std::endl;
		return false;
	}

	if (!CheckSignature(GAM_SIGNATURE) || !CheckVersion(GAM_VERSION_2_0))
		return false;

	return true;
}


uint32
GamResource::PartyMemberCount() const
{
	uint32 count;
	fData->ReadAt(0x24, count);
	return count;
}


gam_party_member
GamResource::PartyMemberAt(uint32 index) const
{
	uint32 offset;
	fData->ReadAt(0x20, offset);
	return _MemberAt(offset + index * kNPCStructSize);
}


CREResource*
GamResource::PartyMemberCRE(uint32 index) const
{
	uint32 offset;
	fData->ReadAt(0x20, offset);
	return _MemberCRE(offset + index * kNPCStructSize);
}


uint32
GamResource::OutOfPartyCount() const
{
	uint32 count;
	fData->ReadAt(0x34, count);
	return count;
}


gam_party_member
GamResource::OutOfPartyAt(uint32 index) const
{
	uint32 offset;
	fData->ReadAt(0x30, offset);
	return _MemberAt(offset + index * kNPCStructSize);
}


CREResource*
GamResource::OutOfPartyCRE(uint32 index) const
{
	uint32 offset;
	fData->ReadAt(0x30, offset);
	return _MemberCRE(offset + index * kNPCStructSize);
}


gam_party_member
GamResource::_MemberAt(uint32 structOffset) const
{
	gam_party_member member;
	char name[9];
	memset(name, 0, sizeof(name));
	fData->ReadAt(structOffset + 0x0c, name, 8);
	member.name = name;

	uint32 orientation;
	fData->ReadAt(structOffset + 0x14, orientation);
	member.orientation = (uint16)orientation;

	fData->ReadAt(structOffset + 0x18, member.areaName);

	uint16 x, y;
	fData->ReadAt(structOffset + 0x20, x);
	fData->ReadAt(structOffset + 0x22, y);
	member.position.x = x;
	member.position.y = y;

	for (int q = 0; q < 3; q++)
		fData->ReadAt(structOffset + 0x9c + q * 8, member.quickSpells[q]);

	// The 8-byte "Character Name" field doubles as the CRE resref here:
	// this engine's Actor(creName, position, face) constructor (the only
	// one StartingParty/SavedGame::Load() ever use) sets the object's own name
	// to the CRE resref itself (see Actor.cpp), so what was written to
	// this field on Save() is exactly the resref Load() needs to
	// construct a matching CREResource below.
	member.creName = res_ref(name);

	return member;
}


CREResource*
GamResource::_MemberCRE(uint32 structOffset) const
{
	uint32 creOffset, creSize;
	fData->ReadAt(structOffset + 0x04, creOffset);
	fData->ReadAt(structOffset + 0x08, creSize);
	// A creature that has no state of its own yet (BALDUR.GAM's) carries
	// no CRE: it is just the resource its name refers to.
	if (creOffset == 0 || creSize == 0)
		return NULL;

	gam_party_member member = _MemberAt(structOffset);
	CREResource* cre = new CREResource(member.creName);
	cre->Load(fData, creOffset, creSize);
	return cre;
}


res_ref
GamResource::CurrentArea() const
{
	res_ref area;
	fData->ReadAt(0x58, area);
	return area;
}


std::vector<std::pair<std::string, int32>>
GamResource::Variables() const
{
	uint32 offset, count;
	fData->ReadAt(0x38, offset);
	fData->ReadAt(0x3c, count);

	std::vector<std::pair<std::string, int32>> variables;
	for (uint32 i = 0; i < count; i++) {
		uint32 varOffset = offset + i * kVariableSize;
		char name[33];
		memset(name, 0, sizeof(name));
		fData->ReadAt(varOffset + 0x00, name, 32);
		int32 value;
		fData->ReadAt(varOffset + 0x28, value);
		variables.push_back(std::make_pair(std::string(name), value));
	}
	return variables;
}


uint32
GamResource::GameTime() const
{
	uint32 units;
	fData->ReadAt(0x08, units);
	return units * kSecondsPerHour / kGameTimeUnitsPerHour;
}


std::vector<gam_journal_entry>
GamResource::JournalEntries() const
{
	uint32 offset, count;
	fData->ReadAt(0x50, offset);
	fData->ReadAt(0x4c, count);

	std::vector<gam_journal_entry> entries;
	for (uint32 i = 0; i < count; i++) {
		const uint32 entryOffset = offset + i * kJournalEntrySize;
		gam_journal_entry entry;
		uint32 units;
		fData->ReadAt(entryOffset + 0x00, entry.strref);
		fData->ReadAt(entryOffset + 0x04, units);
		fData->ReadAt(entryOffset + 0x08, entry.chapter);
		fData->ReadAt(entryOffset + 0x0a, entry.section);
		fData->ReadAt(entryOffset + 0x0b, entry.group);
		entry.time = units * (kSecondsPerHour / kGameTimeUnitsPerHour);
		// A real save's location byte (0xff = "internal TLK") isn't a group.
		if (entry.group == 0xff)
			entry.group = 0;
		entries.push_back(entry);
	}
	return entries;
}
