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
#include <map>
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

		std::string playerText = IDTable::GetDialog(transition.text_player);
		_FillPlaceHolders(playerText);

		std::ostringstream s;
		s << optionNumber << "-" << playerText;
		textArea->AddDialogText(s.str().c_str(), optionNumber);

		// Mirrored to stdout (0-based, matching what Select-DialogOption
		// expects - see shell/Commands.cpp) so headless/-x test runs can
		// see and drive the dialog without a GUI/mouse.
		std::cout << "Response " << (optionNumber - 1) << ": " << playerText << std::endl;

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
	std::string npcText = IDTable::GetDialog(state.text_ref);
	_FillPlaceHolders(npcText);

	std::string fullText;
	fullText.append(Actor()->LongName()).append(": ");
	fullText.append(npcText);
	textArea->AddText(fullText.c_str());

	// Mirrored to stdout for the same headless-testing reason as
	// ShowPlayerOptions() above.
	std::cout << fullText << std::endl;
}


void
DialogHandler::_ExecuteTransition(const transition_entry& transition)
{
	if (transition.HasActions()) {
		std::string actions = fResource->GetAction(transition.index_action);

		auto actionList = Parser::ActionsFromString(actions);
		for (auto* params : actionList) {
			fInitiator->AddAction(params);
		}
	}

	// An instant action just queued above (e.g. STARTCUTSCENE, which
	// real BG1 dialogue actually ends transitions with - Gorion's intro)
	// can synchronously reach Game::TerminateDialog(), which deletes
	// this very DialogHandler out from under us (found via a real
	// use-after-free crash report). Once that's happened there's
	// nothing left on `this` to safely read below - bail out before
	// touching fResource/fCurrentState/fStatus.
	if (!Game::Get()->InDialogMode())
		return;

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
	return fResource->GetTransition(num);
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


static void
_ReplaceAll(std::string& text, const std::string& token, const std::string& value)
{
	size_t pos = 0;
	while ((pos = text.find(token, pos)) != std::string::npos) {
		text.replace(pos, token.length(), value);
		pos += value.length();
	}
}


// Was declared and implemented but never actually called until now (so
// <CHARNAME>/<GABBER> were showing up literally, unsubstituted, in real
// dialog text - see the Fase 10 plan notes on this batch) - now wired
// into both _ShowTriggerText() and ShowPlayerOptions() above.
void
DialogHandler::_FillPlaceHolders(std::string& text)
{
	std::string playerName = Game::Get()->Party()->ActorAt(0)->Name();
	_ReplaceAll(text, "<CHARNAME>", playerName);

	// <GABBER> is conventionally "whoever is on the other side of this
	// exchange" - defaults to the dialog target (the party member being
	// talked to), unless a script already set an explicit "GABBER"
	// token via SetGabber()/SETGABBER (checked first, since the loop
	// below would otherwise find nothing left to replace).
	const std::map<std::string, std::string>& tokens = Game::Get()->Tokens();
	if (fTarget != NULL && tokens.find("GABBER") == tokens.end())
		_ReplaceAll(text, "<GABBER>", fTarget->Name());

	for (const auto& token : tokens)
		_ReplaceAll(text, "<" + token.first + ">", token.second);
}

