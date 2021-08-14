/*
 * Dialog.cpp
 *
 *  Created on: 30 nov 2020
 *      Author: Stefano Ceccherini
 */

#include "Dialog.h"

#include "Actor.h"
#include "Core.h"
#include "Game.h"
#include "GUI.h"
#include "Parsing.h"
#include "ResManager.h"
#include "TextArea.h"

#include <cassert>

// DialogState
DialogHandler::DialogHandler(::Actor* initiator, ::Actor* target, const res_ref& resourceResRef)
	:
	fState(NULL),
	fNextStateIndex(0),
	fInitiator(initiator),
	fTarget(target),
	fResource(NULL),
	fEnd(false)
{
	fResource = gResManager->GetDLG(resourceResRef);
	Continue();
}


DialogHandler::~DialogHandler()
{
	gResManager->ReleaseResource(fResource);
}


DialogHandler::State*
DialogHandler::GetNextValidState()
{
	for (;;) {
		fState = _GetNextState();
		if (fState == NULL)
			break;
		std::vector<trigger_node*> triggerList = Parser::TriggersFromString(fState->Trigger());
		std::cout << "GetNextValidState: Checking triggers... " ;
		if (triggerList.size() == 0) {
			std::cout << "no trigger found." << std::endl;
			break;
		}
		if (Actor()->EvaluateDialogTriggers(triggerList)) {
			std::cout << "a trigger returned true!" << std::endl;
			break;
		}
	}

	return fState;
}


bool
DialogHandler::IsWaitingUserChoice() const
{
	return fTransitions.size() > 1;
}


void
DialogHandler::ShowActorMessage()
{
	TextArea* textArea = GUI::Get()->GetMessagesTextArea();
	if (textArea == NULL) {
		std::cerr << "NULL Text Area!!!" << std::endl;
		return;
	}
	//std::cout << "Display message." << std::endl;
	Core::Get()->DisplayMessage(Actor()->LongName().c_str(), fState->Text().c_str());
}


void
DialogHandler::ShowPlayerOptions()
{
	TextArea* textArea = GUI::Get()->GetMessagesTextArea();
	if (textArea == NULL) {
		std::cerr << "NULL Text Area!!!" << std::endl;
		return;
	}
	for (int32 t = 0; t < CountTransitions(); t++) {
		DialogHandler::Transition transition = TransitionAt(t);
		if (!transition.text_player.empty()) {
			std::string option("-");
			textArea->AddDialogText(option.c_str(), transition.text_player.c_str(), t);
		}
	}
}


void
DialogHandler::SelectOption(int32 option)
{
	assert(fTransitions.size() >= (size_t)option);

	Transition *transition = &fTransitions.at(option);
	std::cout << "SelectOption: " << transition->text_player << std::endl;
	std::cout << "END ? " << ((transition->entry.flags & DLG_TRANSITION_END) ? "YES" : "no" )<< std::endl;

	delete fState;
	fState = NULL;
	std::cout << "next resource: " << transition->entry.resource_next_state << std::endl;
	std::cout << "next index: " << transition->entry.index_next_state << std::endl;
	if (transition->entry.resource_next_state != fResource->Name()) {
		gResManager->ReleaseResource(fResource);
		fResource = NULL;
		std::cout << "Getting resource..." << std::endl;
		fResource = gResManager->GetDLG(transition->entry.resource_next_state);
	}
	std::cout << "Getting next state..." << std::endl;
	fNextStateIndex = transition->entry.index_next_state;

	if (transition->entry.flags & DLG_TRANSITION_END)
		fEnd = true;
	/*
	if (transition.entry.index_action != -1) {
		// TODO: Execute action
		std::cout << "Action: " << fCurrentTransition.entry.index_action << std::endl;
		uint32 action = fResource->GetAction(fCurrentTransition.entry.index_action);
		std::cout << "Action: " << IDTable::ActionAt(action) << std::endl;
	}

	if (transition.entry.flags & DLG_TRANSITION_HAS_JOURNAL)
		std::cout << "text journal: " << fCurrentTransition.entry.text_journal << std::endl;
*/
}


void
DialogHandler::Continue()
{
	if (fEnd) {
		// TODO: Not nice. TerminateDialog deletes this object
		Game::Get()->TerminateDialog();
		return;
	}
	std::cout << "DialogHandler::Continue()" << std::endl;
	fState = GetNextValidState();
	std::cout << "state: " << (fState ? fState->Text() : "NULL") << std::endl;

	if (fState) {
		ShowActorMessage();
		ShowPlayerOptions();
	} else {
		// TODO: Not nice. TerminateDialog deletes this object
		Game::Get()->TerminateDialog();
	}
}


DialogHandler::State*
DialogHandler::CurrentState()
{
	return fState;
}


DialogHandler::Transition
DialogHandler::TransitionAt(int32 index)
{
	return fTransitions.at(index);
}


int32
DialogHandler::CountTransitions() const
{
	return fTransitions.size();
}


void
DialogHandler::HandleTransition(Transition& transition)
{
}


DialogHandler::State*
DialogHandler::_GetNextState()
{
	std::cout << "_GetNextState():" << std::endl;
	delete fState;
	fState = NULL;
	fTransitions.clear();

	/*if (fCurrentTransition != NULL && (fCurrentTransition->entry.flags & DLG_TRANSITION_END)) {
		std::cout << "_GetNextState(): TRANSITION_END, return NULL" << std::endl;
		return NULL;
	}*/

	dlg_state nextState;
	try {
//		std::cout << "DialogState::GetNextState(" << fNextStateIndex << ")" << std::endl;
		nextState = fResource->GetStateAt(fNextStateIndex);
	} catch (std::exception& e ) {
		fNextStateIndex = 0;
		std::cerr << e.what() << std::endl;
		return NULL;
	}

	std::string triggerString;
	if (nextState.trigger != -1)
		triggerString = fResource->GetStateTrigger(nextState.trigger);

	//std::cout << "New DialogState::State()" << std::endl;
	fState = new DialogHandler::State(triggerString, IDTable::GetDialog(nextState.text_ref),
									nextState.transitions_num, nextState.transition_first);

	fNextStateIndex++;

	std::cout << "Get Transitions..." << std::endl;
	// Get Transitions for this state
	for (int32 i = 0; i < fState->NumTransitions(); i++) {
		DialogHandler::Transition transition;
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
	std::cout << " found " << fTransitions.size() << " transitions." << std::endl;

	return fState;
}


DialogHandler::Transition
DialogHandler::_GetTransition(int32 num)
{
	DialogHandler::Transition transition;
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
DialogHandler::Resource()
{
	return fResource;
}


Actor*
DialogHandler::Actor()
{
	return fInitiator;
}


// DialogState::State
DialogHandler::State::State(std::string triggerString, std::string text,
						  int32 numTransitions, int32 transitionIndex)
	:
	fText(text),
	fTrigger(triggerString),
	fTransitonIndex(transitionIndex),
	fNumTransitions(numTransitions)
{
}


std::string
DialogHandler::State::Trigger() const
{
	return fTrigger;
}


std::string
DialogHandler::State::Text() const
{
	return fText;
}


int32
DialogHandler::State::NumTransitions() const
{
	return fNumTransitions;
}


int32
DialogHandler::State::TransitionIndex() const
{
	return fTransitonIndex;
}
