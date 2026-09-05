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
GamResource::SetVariables(const std::vector<std::pair<std::string, int32>>& variables)
{
	fPendingVariables = variables;
}


bool
GamResource::WriteToFile(const char* path) const
{
	uint32 npcOffset = kHeaderSize;
	uint32 creOffset = npcOffset + kNPCStructSize * fPendingMembers.size();

	uint32 totalCreSize = 0;
	for (const _PendingMember& member : fPendingMembers)
		totalCreSize += member.cre->DataSize();

	uint32 variablesOffset = creOffset + totalCreSize;
	uint32 totalSize = variablesOffset + kVariableSize * fPendingVariables.size();

	MemoryStream buffer(totalSize);
	// MemoryStream(size) doesn't zero-initialize - every field this
	// engine doesn't model (formation, weather, GUI flags, quick-slots,
	// character stats, voice set, ...) relies on starting at 0.
	memset(buffer.Data(), 0, totalSize);

	// Header - see IESDP gam_v2.0. Fields this engine has no matching
	// concept for (formation, weather, GUI flags, journal, familiar,
	// stored/pocket-plane locations, party-level reputation/gold pool)
	// are left zeroed rather than guessed.
	buffer.WriteAt(0x00, GAM_SIGNATURE, 4);
	buffer.WriteAt(0x04, GAM_VERSION_2_0, 4);
	uint32 zero32 = 0;
	buffer.WriteAt(0x08, &zero32, sizeof(zero32)); // game time
	uint32 partyCount = (uint32)fPendingMembers.size();
	buffer.WriteAt(0x1c, &partyCount, sizeof(uint16)); // count excl. protagonist (informational only)
	buffer.WriteAt(0x20, &npcOffset, sizeof(npcOffset));
	buffer.WriteAt(0x24, &partyCount, sizeof(partyCount));
	// 0x28/0x2c party inventory offset/count: no shared party-level
	// inventory exists in this engine (only per-CRE inventory - see
	// Actor::AddItem() from Phase 1), left as 0/0.
	buffer.WriteAt(0x30, &creOffset, sizeof(creOffset)); // non-party NPCs: none, point past party data
	uint32 zeroCount = 0;
	buffer.WriteAt(0x34, &zeroCount, sizeof(zeroCount));
	buffer.WriteAt(0x38, &variablesOffset, sizeof(variablesOffset));
	uint32 varCount = (uint32)fPendingVariables.size();
	buffer.WriteAt(0x3c, &varCount, sizeof(varCount));
	buffer.WriteAt(0x40, &fPendingArea, sizeof(res_ref)); // Main area
	buffer.WriteAt(0x58, &fPendingArea, sizeof(res_ref)); // Current area

	// NPC structs + embedded CRE data.
	uint32 currentCreOffset = creOffset;
	for (uint32 i = 0; i < fPendingMembers.size(); i++) {
		const _PendingMember& member = fPendingMembers[i];
		uint32 structOffset = npcOffset + i * kNPCStructSize;
		uint32 creSize = member.cre->DataSize();

		uint16 selection = 0;
		buffer.WriteAt(structOffset + 0x00, &selection, sizeof(selection));
		uint16 order = (uint16)i;
		buffer.WriteAt(structOffset + 0x02, &order, sizeof(order));
		buffer.WriteAt(structOffset + 0x04, &currentCreOffset, sizeof(currentCreOffset));
		buffer.WriteAt(structOffset + 0x08, &creSize, sizeof(creSize));
		char name[8];
		memset(name, 0, sizeof(name));
		memcpy(name, member.info.name.c_str(), sizeof(name));
		buffer.WriteAt(structOffset + 0x0c, name, sizeof(name));
		uint32 orientation = member.info.orientation;
		buffer.WriteAt(structOffset + 0x14, &orientation, sizeof(orientation));
		buffer.WriteAt(structOffset + 0x18, &member.info.areaName, sizeof(res_ref));
		uint16 x = (uint16)member.info.position.x;
		uint16 y = (uint16)member.info.position.y;
		buffer.WriteAt(structOffset + 0x20, &x, sizeof(x));
		buffer.WriteAt(structOffset + 0x22, &y, sizeof(y));
		// 0x24 onward (happiness, quick-slots, character stats, voice
		// set): left zeroed, not modeled by this engine.

		member.cre->WriteDataTo(&buffer, currentCreOffset);
		currentCreOffset += creSize;
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
	uint32 npcOffset;
	fData->ReadAt(0x20, npcOffset);
	uint32 structOffset = npcOffset + index * kNPCStructSize;

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

	// The 8-byte "Character Name" field doubles as the CRE resref here:
	// this engine's Actor(creName, position, face) constructor (the only
	// one Game::CreateParty()/Load() ever use) sets the object's own name
	// to the CRE resref itself (see Actor.cpp), so what was written to
	// this field on Save() is exactly the resref Load() needs to
	// construct a matching CREResource below.
	member.creName = res_ref(name);

	return member;
}


CREResource*
GamResource::PartyMemberCRE(uint32 index) const
{
	uint32 npcOffset;
	fData->ReadAt(0x20, npcOffset);
	uint32 structOffset = npcOffset + index * kNPCStructSize;

	uint32 creOffset, creSize;
	fData->ReadAt(structOffset + 0x04, creOffset);
	fData->ReadAt(structOffset + 0x08, creSize);

	gam_party_member member = PartyMemberAt(index);
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
