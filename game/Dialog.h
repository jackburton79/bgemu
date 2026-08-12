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
	::Actor* Actor();

private:
	DialogState	fStatus;
	::Actor* fInitiator;
	::Actor* fTarget;
	int32 fCurrentState;
	std::vector<transition_entry> fTransitions;
	std::vector<size_t> fVisibleTransitions;

	DLGResource* fResource;

	void _AdvanceState();
	void _ShowCurrentState(const dlg_state& state);
	bool _TransitionVisible(const transition_entry& transition);
	void _BuildTransitions(const dlg_state& state);
	void _ShowTriggerText(const dlg_state& state);
	void _ExecuteTransition(const transition_entry& transition);

	void _Advance();

	transition_entry _ReadTransition(int32 num);
	void _FillPlaceHolders(std::string& text);
};



#endif /* DIALOG_H_ */
