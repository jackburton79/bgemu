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
	void GetNextState(int32 index);
	void SelectOption(int32 option);

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
	};

	State* CurrentState();
	Transition GetTransition(int32 num);

	DLGResource* Resource();
	::Actor* Actor();

private:
	State* fState;
	::Actor* fInitiator;
	::Actor* fTarget;
	DLGResource* fResource;
};



#endif /* DIALOG_H_ */
