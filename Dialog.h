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
class DialogState {
public:
	DialogState(::Actor* initiator, ::Actor* target, const res_ref& resourceResRef);
	~DialogState();

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

	class Transition {
	public:
		std::string text_player;
		std::string action;
		transition_entry entry;
	};

	State* CurrentState();
	State* GetNextState();
	State* GetNextValidState();

	void SelectOption(int32 option);

	typedef std::vector<Transition> TransitionList;
	Transition TransitionAt(int32 index);
	int32 CountTransitions() const;
	Transition& CurrentTransition();

	DLGResource* Resource();
	::Actor* Actor();

private:
	State* fState;
	int32 fNextStateIndex;
	::Actor* fInitiator;
	::Actor* fTarget;
	TransitionList fTransitions;
	Transition fCurrentTransition;
	DLGResource* fResource;

	Transition _GetTransition(int32 num);
};



#endif /* DIALOG_H_ */
