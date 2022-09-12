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
	for (;;) {
		fState = _GetNextState();
		if (fState == NULL)
			break;
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


void
DialogHandler::ShowPlayerOptions()
{
	TextArea* textArea = GUI::Get()->GetMessagesTextArea();
	if (textArea == NULL) {
		std::cerr << "NULL Text Area!!!" << std::endl;
		return;
	}
	for (int32 index = 0; index < CountTransitions(); index++) {
		DialogHandler::Transition transition = TransitionAt(index);
		if (!transition.text_player.empty()) {
			std::ostringstream s;
			s << (index + 1) << "-";
			std::string fullString;
			fullString.append(s.str());
			fullString.append(transition.text_player.c_str());
			textArea->AddDialogText(fullString.c_str(), index + 1);
		}
	}
}


void
DialogHandler::SelectOption(int32 option)
{
	assert(fTransitions.size() >= (size_t)option);

	Transition *transition = &fTransitions.at(option);
	std::cout << "SelectOption: " << transition->text_player << std::endl;

	// Write selected option to text area
	TextArea* textArea = GUI::Get()->GetMessagesTextArea();
	if (textArea != NULL)
		textArea->AddText(transition->text_player.c_str());

	std::cout << "END ? " << ((transition->entry.flags & DLG_TRANSITION_END) ? "YES" : "no" )<< std::endl;

	delete fState;
	fState = NULL;
	std::cout << "next resource: " << transition->entry.resource_next_state << std::endl;
	std::cout << "next index: " << transition->entry.index_next_state << std::endl;
	if (fResource->Name().compare(transition->entry.resource_next_state.CString()) != 0) {
		gResManager->ReleaseResource(fResource);
		fResource = NULL;
		std::cout << "Getting resource..." << std::endl;
		fResource = gResManager->GetDLG(transition->entry.resource_next_state);
	}
	std::cout << "Getting next state..." << std::endl;
	fNextStateIndex = transition->entry.index_next_state;

	if (transition->entry.flags & DLG_TRANSITION_END)
		fEnd = true;

	if (transition->entry.index_action != -1) {
		// TODO: Execute action
		std::cout << "Action: " << transition->entry.index_action << std::endl;
		uint32 action = fResource->GetAction(transition->entry.index_action);
		std::cout << "Action: " << IDTable::ActionAt(action) << std::endl;
	}

	if (transition->entry.flags & DLG_TRANSITION_HAS_JOURNAL)
		std::cout << "text journal: " << transition->entry.text_journal << std::endl;

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
		ShowTriggerText();
		// TODO: see if it's correct
		if (fTransitions.size() == 1) {
			Transition& transition = fTransitions.at(0);
			if (transition.entry.flags & DLG_TRANSITION_END) {
				fEnd = true;
			}
		}
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
	_FillPlaceHolders(text);
	fState = new DialogHandler::State(triggerString, text,
									nextState.transitions_num, nextState.transition_first);

	fNextStateIndex++;

	// Get Transitions for this state
	for (int32 i = 0; i < fState->NumTransitions(); i++) {
		DialogHandler::Transition transition = _GetTransition(fState->TransitionIndex() + i);
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
