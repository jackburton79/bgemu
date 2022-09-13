/*
 * DLGResource.cpp
 *
 *  Created on: 04/ott/2013
 *      Author: stefano
 */

#include "DLGResource.h"

#include "MemoryStream.h"
#include "ResManager.h"

#define DLG_SIGNATURE "DLG "
#define DLG_VERSION_1 "V1.0"

#define DATALENGTH 128


/* static */
Resource*
DLGResource::Create(const res_ref& name)
{
	return new DLGResource(name);
}


DLGResource::DLGResource(const res_ref& name)
	:
	Resource(name, RES_DLG)
{
}


DLGResource::~DLGResource()
{
}


dlg_state
DLGResource::GetStateAt(int32 index)
{
	if ((uint32)index == fNumStates)
		throw std::out_of_range("DLGResource::GetStateAt() out of range");

	dlg_state state;
	_GetStateAt(index, state);

	return state;
}


transition_entry
DLGResource::GetTransition(int32 index)
{
	transition_entry entry;
	fData->ReadAt(fTransitionsTableOffset + index * sizeof(transition_entry), entry);
	return entry;
}


/* virtual */
bool
DLGResource::Load(Archive* archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;

	if (!CheckSignature(DLG_SIGNATURE) ||
		!CheckVersion(DLG_VERSION_1))
		return false;

	fData->ReadAt(0x0008, fNumStates);
	fData->ReadAt(0x000c, fStateTableOffset);
	fData->ReadAt(0x0010, fNumTransitions);
	fData->ReadAt(0x0014, fTransitionsTableOffset);
	fData->ReadAt(0x0018, fStateTriggersTableOffset);
	fData->ReadAt(0x001c, fStateTriggersNum);
	fData->ReadAt(0x0020, fTransitionTriggersTableOffset);
	fData->ReadAt(0x0024, fTransitionTriggersNum);
	fData->ReadAt(0x0028, fActionsTableOffset);
	fData->ReadAt(0x002c, fActionsNum);

	return true;
}


std::string
DLGResource::GetStateTrigger(int triggerIndex)
{
	if ((size_t)triggerIndex >= fStateTriggersNum) {
		std::cerr << "DLGResource::GetStateTrigger(): out of range" << std::endl;
		return "";
	}

	const off_t pos = fData->Position();
	fData->Seek(fStateTriggersTableOffset + triggerIndex * (2 * sizeof(uint32)), SEEK_SET);
	uint32 offset;
	fData->Read(&offset, sizeof(offset));
	uint32 length;
	fData->Read(&length, sizeof(length));
	fData->Seek(pos, SEEK_SET);

	if (length > DATALENGTH) {
		std::cerr << "trigger length too big" << std::endl;
		return "";
	}

	char triggerData[DATALENGTH];
	fData->ReadAt(offset, triggerData, length);
	triggerData[length] = '\0';

	return std::string(triggerData);
}


std::string
DLGResource::GetAction(int32 index)
{
	if ((size_t)index >= fActionsNum) {
		std::cerr << "DLGResource::GetAction() out of range" << std::endl;
		return "";
	}

	const off_t pos = fData->Position();
	fData->Seek(fActionsTableOffset + index * (2 * sizeof(uint32)), SEEK_SET);
	uint32 offset;
	fData->Read(&offset, sizeof(offset));
	uint32 length;
	fData->Read(&length, sizeof(length));
	fData->Seek(pos, SEEK_SET);

	if (length > DATALENGTH) {
		std::cerr << "action string length too big (" << std::dec << length << ")" << std::endl;
		return "";
	}

	char rawData[DATALENGTH];
	fData->ReadAt(offset, rawData, length);
	rawData[length] = '\0';

	return std::string(rawData, length);
}


void
DLGResource::_GetStateAt(int index, dlg_state& dlgState)
{
	fData->ReadAt(fStateTableOffset + index * sizeof(dlg_state), dlgState);
}
