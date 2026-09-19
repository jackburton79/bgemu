/*
 * DLGResource.h
 *
 *  Created on: 04/ott/2013
 *      Author: stefano
 */

#ifndef DLGRESOURCE_H_
#define DLGRESOURCE_H_

#include "Resource.h"

#include <vector>


struct dlg_state {
	int32 text_ref;
	int32 transition_first;
	int32 transitions_num;
	int32 trigger;
};


enum dlg_transition_flags {
	DLG_TRANSITION_HAS_TEXT = 1,
	DLG_TRANSITION_HAS_TRIGGER = 1 << 1,
	DLG_TRANSITION_HAS_ACTION = 1 << 2,
	DLG_TRANSITION_END = 1 << 3,
	DLG_TRANSITION_HAS_JOURNAL = 1 << 4,
	DLG_TRANSITION_INTERRUPT = 1 << 5,
	DLG_TRANSITION_JOURNAL_UNSOLVED = 1 << 6,
	DLG_TRANSITION_JOURNAL_NOTE = 1 << 7,
	DLG_TRANSITION_JOURNAL_SOLVED = 1 << 8
};


struct transition_entry {
	bool HasPlayerText() const;
	bool HasActions() const;
	bool HasNextState() const;
	bool HasTrigger() const;

	int32 flags;
	int32 text_player;
	int32 text_journal;
	int32 index_trigger;
	int32 index_action;
	res_ref resource_next_state;
	int32 index_next_state;
};


class DLGResource : public Resource {
public:
	DLGResource(const res_ref& name);

	virtual bool Load(Archive* archive, uint32 key);

	dlg_state GetStateAt(int32 index);
	std::string GetStateTrigger(int triggerIndex);
	// The states a conversation may open with, in the order they are tried:
	// by their state-trigger index (not by position in the state table).
	// States with no trigger can only be reached through a transition.
	std::vector<int32> InitialStates();
	std::string GetTransitionTrigger(int triggerIndex);
	transition_entry GetTransition(int32 index);

	std::string GetAction(int32 index);

	static Resource* Create(const res_ref& name);

private:
	virtual ~DLGResource();

	void _GetStateAt(int index, dlg_state& state);
	std::string _GetTriggerText(uint32 tableOffset, uint32 count, int index);

	uint32 fNumStates = 0;
	uint32 fStateTableOffset = 0;
	uint32 fNumTransitions = 0;
	uint32 fTransitionsTableOffset = 0;
	uint32 fStateTriggersTableOffset = 0;
	uint32 fStateTriggersNum = 0;
	uint32 fTransitionTriggersTableOffset = 0;
	uint32 fTransitionTriggersNum = 0;
	uint32 fActionsTableOffset = 0;
	uint32 fActionsNum = 0;
};

#endif /* DLGRESOURCE_H_ */
