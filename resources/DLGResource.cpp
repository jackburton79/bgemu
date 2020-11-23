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


struct dlg_state {
	int32 text_ref;
	int32 transition_first;
	int32 transitions_num;
	int32 trigger;
};


struct transition_table {
	int32 flags;
	int32 text_player;
	int32 text_journal;
	int32 index_trigger;
	int32 index_action;
	res_ref resource_next_state;
	int32 index_next_state;
};


enum dlg_transition_flags {
	DLG_TRANSITION_HAS_TEXT = 1,
	DLG_TRANSITION_HAS_ACTION = 1 << 1,
	DLG_TRANSITION_END = 1 << 2,
	DLG_TRANSITION_HAS_JOURNAL = 1 << 3,
	DLG_TRANSITION_INTERRUPT = 1 << 4,
	DLG_TRANSITION_JOURNAL_UNSOLVED = 1 << 5,
	DLG_TRANSITION_JOURNAL_NOTE = 1 << 6,
	DLG_TRANSITION_JOURNAL_SOLVED = 1 << 7
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


DialogState
DLGResource::GetNextState(int32& index)
{
	if ((uint32)index == fNumStates)
		throw std::out_of_range("GetNextState()");

	dlg_state state;
	_GetStateAt(index, state);
	std::string triggerString = "";
	if (state.trigger != -1)
		triggerString = _GetStateTrigger(state.trigger);

	DialogState dialogState;
	dialogState.text = IDTable::GetDialog(state.text_ref);
	dialogState.trigger = triggerString;

	index++;

	return dialogState;
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


void
DLGResource::_GetStateAt(int index, dlg_state& dlgState)
{
	fData->ReadAt(fStateTableOffset + index * sizeof(dlg_state), dlgState);
}


std::string
DLGResource::_GetStateTrigger(int triggerIndex)
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

