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

struct DialogContext {
	int32 currentState;
	std::vector<transition_entry> visibleTransitions;
};

class Actor;
class DLGResource;
class DialogHandler {
public:
	DialogHandler(::Actor* initiator, ::Actor* target, const res_ref& resourceResRef);
	~DialogHandler();

	bool IsWaitingUserChoice() const;

	void ShowTriggerText();
	int32 ShowPlayerOptions();

	void SelectOption(int32 option);

	// returns false if dialog has ended
	bool Continue();

	void HandleTransition(transition_entry transition);

	DLGResource* Resource();
	::Actor* Actor();

private:
	DialogState	fStatus;
	DialogContext fContext;
	::Actor* fInitiator;
	::Actor* fTarget;
	int32 fCurrentState;
	std::vector<transition_entry> fTransitions;
	std::vector<size_t> fVisibleTransitions;

	DLGResource* fResource;
	bool fEnd;

	void _AdvanceState();
	void _ShowCurrentState();
	void _ExecuteTransition(const transition_entry& transition);

	void _Advance();

	transition_entry _ReadTransition(int32 num);
	void _FillPlaceHolders(std::string& text);
};



#endif /* DIALOG_H_ */
