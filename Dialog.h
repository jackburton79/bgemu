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

	class Transition {
	public:
		std::string text_player;
		std::string action;
		transition_entry entry;
	};

	State* CurrentState();
	State* GetNextValidState();

	bool IsWaitingUserChoice() const;

	void ShowActorMessage();
	void ShowPlayerOptions();

	void SelectOption(int32 option);
	void Continue();

	typedef std::vector<Transition> TransitionList;
	Transition TransitionAt(int32 index);
	int32 CountTransitions() const;

	void HandleTransition(Transition& transition);

	DLGResource* Resource();
	::Actor* Actor();

private:
	State* fState;
	int32 fNextStateIndex;
	::Actor* fInitiator;
	::Actor* fTarget;
	TransitionList fTransitions;
	DLGResource* fResource;
	bool fEnd;

	State* _GetNextState();
	Transition _GetTransition(int32 num);
};



#endif /* DIALOG_H_ */
