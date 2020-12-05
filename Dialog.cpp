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

	// Get Transitions for this state
	for (int32 i = 0; i < fState->NumTransitions(); i++) {
		DialogState::Transition transition;
		transition.entry = fResource->GetTransition(fState->TransitionIndex() + i);
		if (transition.entry.flags & DLG_TRANSITION_HAS_TEXT)
			transition.text_player = IDTable::GetDialog(transition.entry.text_player);
		if (transition.entry.flags & DLG_TRANSITION_HAS_ACTION) {
			uint32 action = fResource->GetAction(transition.entry.index_action);
			transition.action = IDTable::ActionAt(action);
			std::cout << "action:" << action << std::endl;
		}

		if (transition.entry.flags & DLG_TRANSITION_END) {
			std::cout << "TRANSITION_END" << std::endl;
		}
		fTransitions.push_back(transition);
	}
}


void
DialogState::SelectOption(int32 option)
{
	DialogState::Transition transition = fTransitions.at(option);
	std::cout << "SelectOption: " << transition.text_player << std::endl;
	if (!(transition.entry.flags & DLG_TRANSITION_END)) {
		delete fState;
		fState = NULL;
		std::cout << "next resource: " << transition.entry.resource_next_state << std::endl;
		std::cout << "next index: " << transition.entry.index_next_state << std::endl;
	}
	if (transition.entry.index_action != -1) {
		// TODO: Execute action
		std::cout << "Action: " << transition.entry.index_action << std::endl;
		uint32 action = fResource->GetAction(transition.entry.index_action);
		std::cout << "Action: " << IDTable::ActionAt(action) << std::endl;
	}
	if (transition.entry.flags & DLG_TRANSITION_HAS_JOURNAL)
		std::cout << "text journal: " << transition.entry.text_journal << std::endl;

}


DialogState::State*
DialogState::CurrentState()
{
	return fState;
}


DialogState::Transition
DialogState::TransitionAt(int32 index)
{
	return fTransitions.at(index);
}


int32
DialogState::CountTransitions() const
{
	return fTransitions.size();
}


DialogState::Transition
DialogState::_GetTransition(int32 num)
{
	DialogState::Transition transition;
	transition.entry = fResource->GetTransition(num);
	if (transition.entry.flags & DLG_TRANSITION_HAS_TEXT)
		transition.text_player = IDTable::GetDialog(transition.entry.text_player);

	if (transition.entry.flags & DLG_TRANSITION_HAS_ACTION) {
		uint32 action = fResource->GetAction(transition.entry.index_action);
		transition.action = IDTable::ActionAt(action);
		std::cout << "action:" << action << std::endl;
	}

	if (transition.entry.flags & DLG_TRANSITION_END) {
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