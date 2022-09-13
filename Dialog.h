/*
 * Dialog.h
 *
 *  Created on: 30 nov 2020
 *      Author: stefano
 */

#ifndef DIALOG_H_
#define DIALOG_H_

#include "DLGResource.h"

class Actor;
class DLGResource;
class DialogHandler {
public:
	DialogHandler(::Actor* initiator, ::Actor* target, const res_ref& resourceResRef);
	~DialogHandler();

	class State {
	public:
		State(std::string triggerString, std::string text,
			  int32 numTransitions, int32 transitionIndex);
		std::string Trigger() const;
		std::string Text() const;
		int32 NumTransitions() const;
		int32 TransitionIndex() const;

	private:
		std::string fText;
		std::string fTrigger;
		int32 fTransitonIndex;
		int32 fNumTransitions;
	};

	State* CurrentState();
	State* GetNextValidState();

	bool IsWaitingUserChoice() const;

	void ShowTriggerText();
	int32 ShowPlayerOptions();

	void SelectOption(int32 option);
	void Continue();

	transition_entry TransitionAt(int32 index);
	int32 CountTransitions() const;

	void HandleTransition(transition_entry transition);

	DLGResource* Resource();
	::Actor* Actor();

private:
	State* fState;
	int32 fNextStateIndex;
	::Actor* fInitiator;
	::Actor* fTarget;

	typedef std::vector<transition_entry> TransitionList;
	TransitionList fTransitions;

	DLGResource* fResource;
	bool fEnd;

	State* _GetNextState();
	transition_entry _ReadTransition(int32 num);
	void _FillPlaceHolders(std::string& text);
};



#endif /* DIALOG_H_ */
