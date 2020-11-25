/*
 * DLGResource.h
 *
 *  Created on: 04/ott/2013
 *      Author: stefano
 */

#ifndef DLGRESOURCE_H_
#define DLGRESOURCE_H_

#include "Resource.h"


class DialogState {
public:
	std::string trigger;
	std::string text;
	uint32 transition_index;
	uint32 transition_count;
};


class DialogTransition {
public:
	std::string text_player;
	std::string action;
};


struct dlg_state;
class DLGResource : public Resource {
public:
	DLGResource(const res_ref& name);

	virtual bool Load(Archive* archive, uint32 key);

	DialogState GetNextState(int32& index);
	DialogTransition GetTransition(int32 index);

	static Resource* Create(const res_ref& name);

private:
	virtual ~DLGResource();

	void _GetStateAt(int index, dlg_state& state);
	std::string _GetStateTrigger(int triggerIndex);

	uint32 fNumStates;
	uint32 fStateTableOffset;
	uint32 fNumTransitions;
	uint32 fTransitionsTableOffset;
	uint32 fStateTriggersTableOffset;
	uint32 fStateTriggersNum;
	uint32 fTransitionTriggersTableOffset;
	uint32 fTransitionTriggersNum;
	uint32 fActionsTableOffset;
	uint32 fActionsNum;
};

#endif /* DLGRESOURCE_H_ */
