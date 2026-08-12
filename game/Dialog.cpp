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
	fStatus(DialogState::Advancing),
	fInitiator(initiator),
	fTarget(target),
	fCurrentState(0),
	fResource(NULL)
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
	return fStatus == DialogState::WaitingForPlayer;
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
	assert(size_t(option) < fVisibleTransitions.size());

	const transition_entry& transition = fTransitions.at(fVisibleTransitions.at(option));

	_ExecuteTransition(transition);
}


bool
DialogHandler::Continue()
{
	if (fStatus == DialogState::Finished)
		return false;

	if (fStatus == DialogState::WaitingForPlayer)
		return true;

	_AdvanceState();

	return fStatus != DialogState::Finished;
}


void
DialogHandler::_ShowCurrentState(const dlg_state& state)
{
	_ShowTriggerText(state);

	_BuildTransitions(state);

	const int32 numOptions = ShowPlayerOptions();

	if (numOptions == 0) {
		if (!fTransitions.empty()) {
			_ExecuteTransition(fTransitions.front());
			return;
		}

		fStatus = DialogState::Finished;
		return;
	}

	fStatus = DialogState::WaitingForPlayer;
}


bool
DialogHandler::_TransitionVisible(const transition_entry& transition)
{
	return transition.HasPlayerText();
}


void
DialogHandler::_BuildTransitions(const dlg_state& state)
{
	fTransitions.clear();

	for (int32 i = 0; i < state.transitions_num; ++i) {
		fTransitions.push_back(fResource->GetTransition(state.transition_first + i));
	}
}


void
DialogHandler::_ShowTriggerText(const dlg_state& state)
{
	TextArea* textArea = GUI::Get()->GetMessagesTextArea();
	if (textArea == NULL) {
		std::cerr << "NULL Text Area!!!" << std::endl;
		return;
	}
	std::string fullText;
	fullText.append(Actor()->LongName()).append(": ");
	fullText.append(IDTable::GetDialog(state.text_ref));
	textArea->AddText(fullText.c_str());
}


void
DialogHandler::_ExecuteTransition(const transition_entry& transition)
{
	if (transition.HasActions()) {
		std::string actions = fResource->GetAction(transition.index_action);

		auto actionList = Parser::ActionsFromString(actions);
		for (auto* params : actionList) {
			bool canContinue = false;

			Action* action = Script::GetAction(fInitiator, params, canContinue);
			if (action != nullptr)
				fInitiator->AddAction(action);
		}
	}

	if (!transition.HasNextState()) {
		fStatus = DialogState::Finished;
		return;
	}

	if (fResource->Name() != transition.resource_next_state.CString()) {
		gResManager->ReleaseResource(fResource);

		fResource =	gResManager->GetDLG(transition.resource_next_state);
	}

	fCurrentState = transition.index_next_state;

	fStatus = DialogState::Advancing;
}


void
DialogHandler::_AdvanceState()
{
	for (;;) {
		dlg_state state;

		try {
			state = fResource->GetStateAt(fCurrentState);
		} catch (...) {
			fStatus = DialogState::Finished;
			return;
		}

		bool valid = true;
		if (state.trigger != -1) {
			std::string trigger = fResource->GetStateTrigger(state.trigger);
			auto triggers = Parser::TriggersFromString(trigger);
			valid = fInitiator->EvaluateDialogTriggers(triggers);
		}

		if (valid) {
			_ShowCurrentState(state);
			return;
		}

		fCurrentState++;
	}
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

