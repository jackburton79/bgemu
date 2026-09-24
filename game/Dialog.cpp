/*
 * Dialog.cpp
 *
 *  Created on: 30 nov 2020
 *      Author: Stefano Ceccherini
 */

#include "Dialog.h"

#include "2DAResource.h"
#include "TLKResource.h"
#include "Actor.h"
#include "AreaRoom.h"
#include "Core.h"
#include "CreResource.h"
#include "Game.h"
#include "GameJournal.h"
#include "GUI.h"
#include "Parsing.h"
#include "Party.h"
#include "ResManager.h"
#include "Script.h"
#include "TextArea.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <sstream>

// "Journal updated" (11359 in both BG1's and BG2's strings table).
static const uint32 kJournalChangedStrRef = 11359;

// DialogState
DialogHandler::DialogHandler(::Actor* initiator, ::Actor* target, const res_ref& resourceResRef)
	:
	fStatus(DialogState::Advancing),
	fInitiator(initiator),
	fFirstInitiator(initiator),
	fTarget(target),
	fCurrentState(0),
	fInitialState(true),
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

	// _AdvanceState() can reach a state with no player-choice text at
	// all, auto-advancing straight into _ExecuteTransition() (see
	// there) - if that transition's actions include one that
	// synchronously terminates the dialog (e.g. STARTCUTSCENE), `this`
	// is deleted before getting back here. Game::InDialogMode() is safe
	// to call regardless (it only reads a Game member, never `this`);
	// reading fStatus below would not be.
	if (!Game::Get()->InDialogMode())
		return false;

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


// A transition with no trigger of its own is always available; one with
// DLG_TRANSITION_HAS_TRIGGER is gated exactly like a state trigger (see
// _AdvanceState()) - same trigger-text format, same AND/OR evaluation,
// just read from the transition-trigger table instead of the state one.
bool
DialogHandler::_TransitionTriggerPasses(const transition_entry& transition)
{
	if (!transition.HasTrigger())
		return true;

	std::string trigger = fResource->GetTransitionTrigger(transition.index_trigger);
	auto triggers = Parser::TriggersFromString(trigger);
	bool valid = Script::EvaluateTriggerList(fInitiator, triggers);
	for (trigger_params* t : triggers)
		delete t;
	return valid;
}


void
DialogHandler::_BuildTransitions(const dlg_state& state)
{
	fTransitions.clear();

	for (int32 i = 0; i < state.transitions_num; ++i) {
		transition_entry transition = fResource->GetTransition(state.transition_first + i);
		if (_TransitionTriggerPasses(transition))
			fTransitions.push_back(transition);
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

	// Voiced lines: the TLK entry names the recording.
	if (TLKEntry* entry = IDTable::GetTLKEntry(state.text_ref)) {
		Core::Get()->PlaySound(entry->sound_ref);
		delete entry;
	}

	std::string fullText;
	fullText.append(Actor()->LongName()).append(": ");
	fullText.append(npcText);
	textArea->AddText(fullText.c_str());

	// Mirrored to stdout for the same headless-testing reason as
	// ShowPlayerOptions() above.
	std::cout << fullText << std::endl;
}


// A transition with a journal note adds it to the journal (GemRB's
// DialogHandler::UpdateJournalForTransition()): bits 6/8 of the flags say
// unsolved/solved quest and, in BG2 - the only game with journal sections -
// choose the section (both/neither = the plain "info" journal); the entry's
// group is the flags' upper half. When that changed the journal, the
// message area says so, followed by the note's first line.
void
DialogHandler::_UpdateJournal(const transition_entry& transition)
{
	if (!(transition.flags & DLG_TRANSITION_HAS_JOURNAL))
		return;

	uint8 section = GameJournal::SECTION_USER;
	if (Core::Get()->Game() == game::GAME_BALDURSGATE2) {
		static const uint8 kSections[4] = {
			GameJournal::SECTION_INFO, GameJournal::SECTION_QUEST, GameJournal::SECTION_DONE, GameJournal::SECTION_USER
		};
		int index = 0;
		if (transition.flags & DLG_TRANSITION_JOURNAL_UNSOLVED)
			index |= 1;
		if (transition.flags & DLG_TRANSITION_JOURNAL_SOLVED)
			index |= 2;
		section = kSections[index];
	}

	const uint32 strref = (uint32)transition.text_journal;
	const uint8 group = (uint8)(((uint32)transition.flags >> 16) & 0xff);
	if (!Game::Get()->Journal().Add(strref, section, group))
		return;

	std::string message = IDTable::GetDialog(kJournalChangedStrRef);
	std::string note = IDTable::GetDialog(strref);
	note.erase(std::min(note.find('\n'), note.size()));
	if (!note.empty())
		message += " - " + note;
	if (TextArea* textArea = GUI::Get()->GetMessagesTextArea())
		textArea->AddText(message.c_str());
	std::cout << message << std::endl;
}


void
DialogHandler::_ExecuteTransition(const transition_entry& transition)
{
	_UpdateJournal(transition);

	if (transition.HasActions()) {
		std::string actions = fResource->GetAction(transition.index_action);

		auto actionList = Parser::ActionsFromString(actions);
		for (auto* params : actionList) {
			fInitiator->AddAction(params);
			// AddAction() takes its own reference (Acquire()) - this loop
			// still holds the one ActionFromString() handed it (refcount
			// starts at 1, see action_params::action_params()), which
			// must be dropped here or it never reaches 0.
			params->Release();
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
		// A state of another dialog file: it is that file's owner who speaks
		// and whose script the actions run for (the party banter of one
		// creature is answered by another, for instance).
		::Actor* owner = _FindDialogOwner(transition.resource_next_state);
		if (owner == NULL) {
			std::cerr << "Dialog: can't redirect to " << transition.resource_next_state
				<< std::endl;
			fStatus = DialogState::Finished;
			return;
		}
		fInitiator = owner;

		gResManager->ReleaseResource(fResource);

		fResource =	gResManager->GetDLG(transition.resource_next_state);
	}

	fCurrentState = transition.index_next_state;

	fStatus = DialogState::Advancing;
}


// Same search as GemRB's DialogHandler: the creature the conversation started
// with if it has that dialog, else any creature of the area that does, else -
// for a banter file - the creature INTERDIA.2DA lists it for.
::Actor*
DialogHandler::_FindDialogOwner(const res_ref& dialog) const
{
	auto hasDialog = [&dialog](::Actor* actor) {
		return actor != NULL && actor->CRE() != NULL
			&& strcasecmp(actor->CRE()->DialogFile().CString(), dialog.CString()) == 0;
	};
	if (hasDialog(fFirstInitiator))
		return fFirstInitiator;

	AreaRoom* room = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
	if (room == NULL)
		return NULL;
	for (int32 i = 0; i < room->ActorsCount(); i++) {
		if (hasDialog(room->ActorAt(i)))
			return room->ActorAt(i);
	}

	// A banter file names its creature only through INTERDIA.2DA (row =
	// scripting name, FILE column = dialog).
	TWODAResource* table = gResManager->Get2DA(res_ref("INTERDIA"));
	if (table == NULL)
		return NULL;
	::Actor* owner = NULL;
	for (int32 row = 0; row < table->CountRows() && owner == NULL; row++) {
		if (strcasecmp(table->ValueAt(row, 0).c_str(), dialog.CString()) != 0)
			continue;
		owner = dynamic_cast<::Actor*>(room->GetObject(table->RowName(row).c_str()));
	}
	gResManager->ReleaseResource(table);
	return owner;
}


bool
DialogHandler::_StateTriggerPasses(const dlg_state& state)
{
	if (state.trigger == -1)
		return true;

	std::string trigger = fResource->GetStateTrigger(state.trigger);
	auto triggers = Parser::TriggersFromString(trigger);
	const bool valid = fInitiator->EvaluateDialogTriggers(triggers);
	// trigger_params isn't refcounted like action_params - just a
	// plain heap object EvaluateDialogTriggers() only reads, so
	// this loop (the only owner) must delete it directly.
	for (trigger_params* t : triggers)
		delete t;
	return valid;
}


void
DialogHandler::_AdvanceState()
{
	if (fInitialState) {
		// The opening state is the first one, in trigger-index order, whose
		// trigger holds - see DLGResource::InitialStates().
		fInitialState = false;
		for (int32 index : fResource->InitialStates()) {
			dlg_state state = fResource->GetStateAt(index);
			if (_StateTriggerPasses(state)) {
				fCurrentState = index;
				_ShowCurrentState(state);
				return;
			}
		}
		fStatus = DialogState::Finished;
		return;
	}

	// A transition leads straight to its target state; its trigger is not
	// consulted (as in GemRB's DialogHandler).
	dlg_state state;
	try {
		state = fResource->GetStateAt(fCurrentState);
	} catch (...) {
		fStatus = DialogState::Finished;
		return;
	}
	_ShowCurrentState(state);
}


transition_entry
DialogHandler::_ReadTransition(int32 num)
{
	return fResource->GetTransition(num);
}


bool
DialogHandler::Involves(const ::Actor* actor) const
{
	return actor == fInitiator || actor == fFirstInitiator || actor == fTarget;
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
	std::string playerName = Game::Get()->Party()->ActorAt(0)->LongName();
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

	// TLK strings routinely embed literal CR/LF for paragraph breaks.
	// Collapsed to a single space so both paths always see one coherent line.
	std::string collapsed;
	collapsed.reserve(text.size());
	bool lastWasSpace = false;
	for (char c : text) {
		if (c == '\r' || c == '\n')
			c = ' ';
		if (c == ' ' && lastWasSpace)
			continue;
		collapsed += c;
		lastWasSpace = (c == ' ');
	}
	text = collapsed;
}

