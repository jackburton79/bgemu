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
DLGResource::GetNextState(int32& index)
{
	if ((uint32)index == fNumStates)
		throw std::out_of_range("GetNextState()");

	dlg_state state;
	_GetStateAt(index, state);

	index++;

	return state;
}



transition_entry
DLGResource::GetTransition(int32 index)
{
	transition_entry entry;
	fData->ReadAt(fTransitionsTableOffset + index * sizeof(transition_entry), entry);
	return entry;
}


uint32
DLGResource::GetAction(int32 index)
{
	uint32 action;
	fData->ReadAt(fActionsTableOffset + index * sizeof(uint32), action);
	return action;
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
	state_trigger stateTrigger;
	uint32 offset = fStateTriggersTableOffset
			+ triggerIndex * sizeof(state_trigger);
	fData->Seek(offset, SEEK_SET);
	fData->Read(stateTrigger);

	char triggerData[64];
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
