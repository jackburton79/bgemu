/*
 * Dialog.cpp
 *
 *  Created on: 30 nov 2020
 *      Author: Stefano Ceccherini
 */

#include "Dialog.h"

#include "Actor.h"
#include "ResManager.h"


// DialogState
DialogState::DialogState(::Actor* initiator, ::Actor* target, const res_ref& resourceResRef)
	:
	fState(NULL),
	fInitiator(initiator),
	fTarget(target),
	fResource(NULL)
{
	fResource = gResManager->GetDLG(resourceResRef);
}


DialogState::~DialogState()
{
	gResManager->ReleaseResource(fResource);
}


void
DialogState::GetNextState(int32& index)
{
	dlg_state nextState = fResource->GetNextState(index);

	delete fState;
	fState = NULL;

	std::string triggerString;
	if (nextState.trigger != -1)
		triggerString = fResource->GetStateTrigger(nextState.trigger);

	fState = new DialogState::State(triggerString, IDTable::GetDialog(nextState.text_ref),
									nextState.transitions_num, nextState.transition_first);
}


void
DialogState::SelectOption(int32 option)
{

}


DialogState::State*
DialogState::CurrentState()
{
	return fState;
}


DialogState::Transition
DialogState::GetTransition(int32 num)
{
	DialogState::Transition transition;
	transition_entry entry = fResource->GetTransition(num);
	if (entry.flags & DLG_TRANSITION_HAS_TEXT)
			transition.text_player = IDTable::GetDialog(entry.text_player);

	if (entry.flags & DLG_TRANSITION_HAS_ACTION) {
		uint32 action = fResource->GetAction(entry.index_action);
		transition.action = IDTable::ActionAt(action);
		std::cout << "action:" << action << std::endl;
	}

	if (entry.flags & DLG_TRANSITION_END) {
		std::cout << "TRANSITION_END" << std::endl;
	}

	return transition;
}


DLGResource*
DialogState::Resource()
{
	return fResource;
}


Actor*
DialogState::Actor()
{
	return fInitiator;
}


// DialogState::State
DialogState::State::State(std::string triggerString, std::string text,
						  int32 numTransitions, int32 transitionIndex)
	:
	fText(text),
	fTrigger(triggerString),
	fTransitonIndex(transitionIndex),
	fNumTransitions(numTransitions)
{
}


std::string
DialogState::State::Trigger() const
{
	return fTrigger;
}


std::string
DialogState::State::Text() const
{
	return fText;
}


int32
DialogState::State::NumTransitions() const
{
	return fNumTransitions;
}


int32
DialogState::State::TransitionIndex() const
{
	return fTransitonIndex;
}
