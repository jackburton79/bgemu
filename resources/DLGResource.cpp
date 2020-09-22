/*
 * DLGResource.cpp
 *
 *  Created on: 04/ott/2013
 *      Author: stefano
 */

#include "DLGResource.h"

#include "MemoryStream.h"

DLGResource::DLGResource(const res_ref& name)
	:
	Resource(name, RES_DLG)
{
}


DLGResource::~DLGResource()
{
}


void
DLGResource::StartDialog()
{
	dlg_state state = _GetStateAt(0);
	std::cout << state.text_ref << std::endl;
}


/* virtual */
bool
DLGResource::Load(Archive* archive, uint32 key)
{
	if (!Resource::Load(archive, key))
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


struct dlg_state
DLGResource::_GetStateAt(int index)
{
	struct dlg_state dlgState;
	fData->ReadAt(fStateTableOffset + index * sizeof(dlg_state), dlgState);
	return dlgState;
}
