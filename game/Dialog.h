/*
 * Dialog.h
 *
 *  Created on: 30 nov 2020
 *      Author: stefano
 */

#ifndef DIALOG_H_
#define DIALOG_H_

#include "DLGResource.h"

enum class DialogState {
	Advancing,
	WaitingForPlayer,
	Finished
};


class Actor;
class DLGResource;
class DialogHandler {
public:
	DialogHandler(::Actor* initiator, ::Actor* target, const res_ref& resourceResRef);
	~DialogHandler();

	bool IsWaitingUserChoice() const;

	int32 ShowPlayerOptions();

	void SelectOption(int32 option);

	// returns false if dialog has ended
	bool Continue();

	void HandleTransition(transition_entry transition);

	DLGResource* Resource();
	// Whether the conversation holds on to this creature.
	bool Involves(const ::Actor* actor) const;
	::Actor* Actor();

private:
	DialogState	fStatus;
	::Actor* fInitiator;
	// Whose dialog the conversation started with: the first place to look
	// when a transition switches to another dialog file.
	::Actor* fFirstInitiator;
	::Actor* fTarget;
	int32 fCurrentState;
	// Until the first state is shown: which one opens the conversation is
	// decided by the states' triggers; afterwards transitions name their
	// target state outright.
	bool fInitialState;
	std::vector<transition_entry> fTransitions;
	std::vector<size_t> fVisibleTransitions;

	DLGResource* fResource;

	void _AdvanceState();
	bool _StateTriggerPasses(const dlg_state& state);
	void _ShowCurrentState(const dlg_state& state);
	bool _TransitionTriggerPasses(const transition_entry& transition);
	void _BuildTransitions(const dlg_state& state);
	void _ShowTriggerText(const dlg_state& state);
	void _ExecuteTransition(const transition_entry& transition);
	void _UpdateJournal(const transition_entry& transition);
	// The creature that owns a dialog file a transition switches to (see the
	// .cpp), NULL if there is none in the area.
	::Actor* _FindDialogOwner(const res_ref& dialog) const;

	transition_entry _ReadTransition(int32 num);
	void _FillPlaceHolders(std::string& text);
};



#endif /* DIALOG_H_ */
