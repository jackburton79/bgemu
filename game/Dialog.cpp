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
	fInitiator(initiator),
	fTarget(target),
	fCurrentState(0),
	fResource(NULL),
	fEnd(false)
{
	fResource = gResManager->GetDLG(resourceResRef);
}


DialogHandler::~DialogHandler()
{
	gResManager->ReleaseResource(fResource);
}


bool
DialogHandler::IsWaitingUserChoice() const
{
	for (size_t index = 0; index < fTransitions.size(); index++) {
		transition_entry transition = fTransitions.at(index);
		if (transition.HasPlayerText())
			return true;
	}
	return false;
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

	if (textArea == NULL)
		return 0;

	fVisibleTransitions.clear();

	int32 optionNumber = 1;

	for (size_t index = 0; index < fTransitions.size(); ++index) {
		const transition_entry& transition = fTransitions[index];

		if (!transition.HasPlayerText())
			continue;

		fVisibleTransitions.push_back(index);

		std::ostringstream s;
		s << optionNumber << "-";

		std::string fullString = s.str();
		fullString += IDTable::GetDialog(transition.text_player);

		textArea->AddDialogText(fullString.c_str(), optionNumber);

		optionNumber++;
	}

	return optionNumber - 1;
}


void
DialogHandler::SelectOption(int32 option)
{
	assert(option >= 0);
	assert(option < fVisibleTransitions.size());

	int32 transitionIndex = fVisibleTransitions[option];

	transition_entry transition = fResource->GetTransition(transitionIndex);
	_ExecuteTransition(transition);
}


bool
DialogHandler::Continue()
{
	if (fStatus == DialogState::Finished)
		return false;

	if (fStatus == DialogState::WaitingForPlayer)
		return true;

	_Advance();

	return fStatus != DialogState::Finished;
}


void
DialogHandler::HandleTransition(transition_entry transition)
{
	//std::cout << "DialogHandler::HandleTransition" << std::endl;

	if (transition.HasPlayerText()) {
		//std::cout << "PlayerText: " << transition.text_player << std::endl;
		// Write selected option to text area
		TextArea* textArea = GUI::Get()->GetMessagesTextArea();
		std::string text = IDTable::GetDialog(transition.text_player);
		if (textArea != NULL)
			textArea->AddText(text.c_str());
	}

	if (transition.HasActions()) {
		std::string actionString = fResource->GetAction(transition.index_action);
		//std::cout << "Actions: " << actionString << std::endl;
		// TODO: Cleanup

		::Actor* actor = Actor();
		//std::cout << "add list to " << actor->Name() << " queue" << std::endl;
		std::vector<action_params*> actionList = Parser::ActionsFromString(actionString);
		for (std::vector<action_params*>::iterator i = actionList.begin();
				i != actionList.end(); i++) {
			action_params* params = *i;
			bool canContinue = false;

			Action* action = Script::GetAction(actor, params, canContinue);
			if (action != NULL)
				actor->AddAction(action);
		}
		//std::cout << "Finished Adding actions" << std::endl;
	}

	/*
	if (transition.flags & DLG_TRANSITION_HAS_JOURNAL)
		std::cout << "text journal: " << transition.text_journal << std::endl;
	*/
	// Prepare next state
	if (transition.HasNextState()) {
		delete fState;
		fState = NULL;
		//std::cout << "next resource: " << transition.resource_next_state << std::endl;
		//std::cout << "next index: " << transition.index_next_state << std::endl;
		if (fResource->Name().compare(transition.resource_next_state.CString()) != 0) {
			gResManager->ReleaseResource(fResource);
			fResource = NULL;

			fResource = gResManager->GetDLG(transition.resource_next_state);
		}

		fNextStateIndex = transition.index_next_state;
	} else {
		fEnd = true;
		//std::cout << "TRANSITION_END" << std::endl;
	}
}


void
DialogHandler::_AdvanceState()
{
	for (;;) {
		dlg_state state = fResource->GetStateAt(fCurrentState);

		if (StateTriggersAreTrue(state)) {
			_ShowCurrentState(state);
			return;
		}

		fCurrentState++;
	}
}


void
DialogHandler::_ShowCurrentState(const dlg_state& state)
{
	ShowNPCText(state);

	BuildVisibleTransitions(state);

	if (fVisibleTransitions.empty()) {
		...
	}
	else {
		fStatus = DialogStatus::WaitingForPlayer;
	}
}


void
DialogHandler::_ExecuteTransition(const transition_entry& transition)
{
	RunActions(transition);

	if (!transition.HasNextState()) {
		fStatus =
			DialogStatus::Finished;
		return;
	}

	LoadNextState(transition);

	fStatus = DialogStatus::Advancing;
}


void
DialogHandler::_Advance()
{
	for (;;) {
		dlg_state state;

		try {
			state = fResource->GetStateAt(fCurrentState);
		} catch (...) {
			fStatus = FINISHED;
			return;
		}

		bool valid = true;

		if (state.trigger != -1) {
			std::string trigger = fResource->GetStateTrigger(state.trigger);

			auto triggers = Parser::TriggersFromString(trigger);

			valid = fInitiator->EvaluateDialogTriggers(triggers);
		}

		if (valid) {
			_ShowState(state);
			return;
		}

		fCurrentState++;
	}
}


DialogHandler::State*
DialogHandler::_GetNextState()
{
	//std::cout << "_GetNextState():" << std::endl;
	delete fState;
	fState = NULL;

	//std::cout << "Clearing transitions" << std::endl;
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
	//std::cout << "Getting transition for state " << fNextStateIndex << std::endl;

	fNextStateIndex++;

	for (int32 i = 0; i < fState->NumTransitions(); i++) {
		transition_entry transition = _ReadTransition(fState->TransitionIndex() + i);
		fTransitions.push_back(transition);
	}
	//std::cout << " found " << fTransitions.size() << " transitions." << std::endl;

	return fState;
}


transition_entry
DialogHandler::_ReadTransition(int32 num)
{
	transition_entry transition = fResource->GetTransition(num);
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

