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
#include "Party.h"
#include "ResManager.h"
#include "Script.h"
#include "TextArea.h"

#include <cassert>
#include <sstream>

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
	std::cout << "DialogHandler::GetNextValidState()" << std::endl;
	for (;;) {
		fState = _GetNextState();
		if (fState == NULL)
			break;
		std::cout << "DialogHandler::GetNextValidState(): got state" << std::endl;
		std::vector<trigger_params*> triggerList = Parser::TriggersFromString(fState->Trigger());
		std::cout << "GetNextValidState: Checking triggers... " << std::endl;
		if (triggerList.size() == 0) {
			std::cout << "GetNextValidState: no trigger found." << std::endl;
			break;
		}
		if (Actor()->EvaluateDialogTriggers(triggerList)) {
			std::cout << "GetNextValidState: a trigger returned true!" << std::endl;
			break;
		}
	}

	std::cout << "GetNextValidState: text: " << (fState ? fState->Text() : "NULL") << std::endl;

	return fState;
}


bool
DialogHandler::IsWaitingUserChoice() const
{
	return fTransitions.size() > 1;
}


void
DialogHandler::ShowTriggerText()
{
	TextArea* textArea = GUI::Get()->GetMessagesTextArea();
	if (textArea == NULL) {
		std::cerr << "NULL Text Area!!!" << std::endl;
		return;
	}
	std::string fullText;
	fullText.append(Actor()->LongName()).append(": ");
	fullText.append(fState->Text());
	textArea->AddText(fullText.c_str());
}


int32
DialogHandler::ShowPlayerOptions()
{
	TextArea* textArea = GUI::Get()->GetMessagesTextArea();
	if (textArea == NULL) {
		std::cerr << "NULL Text Area!!!" << std::endl;
		return 0;
	}

	int32 numOptions = 0;
	for (size_t index = 0; index < fTransitions.size(); index++) {
		transition_entry transition = fTransitions.at(index);
		if (transition.flags & DLG_TRANSITION_HAS_TEXT) {
			std::ostringstream s;
			s << (index + 1) << "-";
			std::string fullString;
			fullString.append(s.str());

			std::string text = IDTable::GetDialog(transition.text_player);
			fullString.append(text.c_str());
			textArea->AddDialogText(fullString.c_str(), index + 1);
			numOptions++;
		}
	}

	return numOptions;
}


void
DialogHandler::SelectOption(int32 option)
{
	assert(fTransitions.size() >= (size_t)option);

	transition_entry transition = fTransitions.at(option);
	HandleTransition(transition);
}


void
DialogHandler::Continue()
{
	std::cout << "DialogHandler::Continue()" << std::endl;

	if (fEnd) {
		// TODO: Not nice. TerminateDialog deletes this object
		Game::Get()->TerminateDialog();
		return;
	}

	fState = GetNextValidState();

	if (fState) {
		ShowTriggerText();
		// TODO: see if it's correct
		int32 numOptions = ShowPlayerOptions();
		// TODO: Handle all transactions ?
		if (numOptions == 0 && fTransitions.size() >= 1) {
			transition_entry transition = fTransitions.at(0);
			HandleTransition(transition);
		}
	} else {
		std::cout << "DialogHandler::Continue(): next state is NULL. Terminating dialog" << std::endl;
		// TODO: Not nice. TerminateDialog deletes this object
		Game::Get()->TerminateDialog();
	}
}


DialogHandler::State*
DialogHandler::CurrentState()
{
	return fState;
}


void
DialogHandler::HandleTransition(transition_entry transition)
{
	std::cout << "DialogHandler::HandleTransition" << std::endl;

	if (transition.HasPlayerText()) {
		std::cout << "PlayerText: " << transition.text_player << std::endl;
		// Write selected option to text area
		TextArea* textArea = GUI::Get()->GetMessagesTextArea();
		std::string text = IDTable::GetDialog(transition.text_player);
		if (textArea != NULL)
			textArea->AddText(text.c_str());
	}
	if (!transition.HasNextState()) {
		fEnd = true;
		std::cout << "TRANSITION_END" << std::endl;
	}
	if (transition.HasActions()) {
		std::string actionString = fResource->GetAction(transition.index_action);
		std::cout << "Actions: " << actionString << std::endl;
		// TODO: Cleanup

		::Actor* actor = Actor();
		std::cout << "add list to " << actor->Name() << " queue" << std::endl;
		std::vector<action_params*> actionList = Parser::ActionsFromString(actionString);
		for (std::vector<action_params*>::iterator i = actionList.begin();
				i != actionList.end(); i++) {
			action_params* params = *i;
			bool canContinue = false;

			Action* action = Script::GetAction(actor, params, canContinue);
			if (action != NULL)
				actor->AddAction(action);
		}
	}

	if (transition.flags & DLG_TRANSITION_HAS_JOURNAL)
		std::cout << "text journal: " << transition.text_journal << std::endl;

	// Prepare next state
	if (transition.HasNextState()) {
		delete fState;
		fState = NULL;
		std::cout << "next resource: " << transition.resource_next_state << std::endl;
		std::cout << "next index: " << transition.index_next_state << std::endl;
		if (fResource->Name().compare(transition.resource_next_state.CString()) != 0) {
			gResManager->ReleaseResource(fResource);
			fResource = NULL;

			fResource = gResManager->GetDLG(transition.resource_next_state);
		}

		fNextStateIndex = transition.index_next_state;
	}
}


DialogHandler::State*
DialogHandler::_GetNextState()
{
	std::cout << "_GetNextState():" << std::endl;
	delete fState;
	fState = NULL;

	std::cout << "Clearing transitions" << std::endl;
	fTransitions.clear();

	dlg_state nextState;
	try {
		nextState = fResource->GetStateAt(fNextStateIndex);
	} catch (std::exception& e ) {
		fNextStateIndex = 0;
		std::cerr << e.what() << std::endl;
		return NULL;
	}

	std::string triggerString;
	if (nextState.trigger != -1)
		triggerString = fResource->GetStateTrigger(nextState.trigger);

	std::string text = IDTable::GetDialog(nextState.text_ref);
	//_FillPlaceHolders(text);
	fState = new DialogHandler::State(triggerString, text,
									nextState.transitions_num, nextState.transition_first);

	// Get Transitions for this state
	std::cout << "Getting transition for state " << fNextStateIndex << std::endl;

	fNextStateIndex++;

	for (int32 i = 0; i < fState->NumTransitions(); i++) {
		transition_entry transition = _ReadTransition(fState->TransitionIndex() + i);
		fTransitions.push_back(transition);
	}
	std::cout << " found " << fTransitions.size() << " transitions." << std::endl;

	return fState;
}


transition_entry
DialogHandler::_ReadTransition(int32 num)
{
	std::cout << "DialogHandler::_ReadTransition(" << num << ")" << std::endl;
	transition_entry transition = fResource->GetTransition(num);
	if (transition.HasPlayerText()) {
		std::cout << "- has text: " << IDTable::GetDialog(transition.text_player) << std::endl;
	}
	if (transition.HasActions()) {
		std::cout << "- has action: " << transition.index_action << std::endl;
	}
	if (transition.HasNextState()) {
		std::cout << "- has next" << std::endl;
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


void
DialogHandler::_FillPlaceHolders(std::string& text)
{
	// TODO: Fill other placeholders
	std::string playerName = Game::Get()->Party()->ActorAt(0)->Name();
	std::string charPlaceHolder = "<CHARNAME>";
	size_t i = text.find(charPlaceHolder);
	if (i != std::string::npos) {
		text.replace(i, charPlaceHolder.length(), playerName.c_str());
	}
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
