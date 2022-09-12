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


struct state_trigger {
	uint32 offset;
	uint32 length;
};


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


std::string
DLGResource::GetAction(int32 index)
{
	char rawData[32];
	uint32 offset;
	uint32 length;
	off_t pos = fData->Position();
	fData->Seek(fActionsTableOffset + index * (2 * sizeof(uint32)), SEEK_SET);
	fData->Read(&offset, sizeof(offset));
	fData->Read(&length, sizeof(length));
	fData->Seek(pos, SEEK_SET);

	fData->ReadAt(offset, rawData, length);

	return std::string(rawData, length);
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

#define TRIGGER_LENGTH 64

std::string
DLGResource::GetStateTrigger(int triggerIndex)
{
	if ((size_t)triggerIndex >= fStateTriggersNum) {
		std::cerr << "trigger out of range" << std::endl;
		return "";
	}

	state_trigger stateTrigger;
	uint32 offset = fStateTriggersTableOffset
			+ triggerIndex * sizeof(state_trigger);
	fData->Seek(offset, SEEK_SET);
	fData->Read(stateTrigger);

	if (stateTrigger.length > TRIGGER_LENGTH) {
		std::cerr << "trigger length too big" << std::endl;
		return "";
	}

	char triggerData[TRIGGER_LENGTH];
	fData->ReadAt(stateTrigger.offset, triggerData, stateTrigger.length);
	triggerData[stateTrigger.length] = '\0';

	std::string string = triggerData;
	return string;
}


void
DLGResource::_GetStateAt(int index, dlg_state& dlgState)
{
	fData->ReadAt(fStateTableOffset + index * sizeof(dlg_state), dlgState);
}
