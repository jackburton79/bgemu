#include "JournalScreen.h"

#include "Button.h"
#include "Core.h"
#include "Game.h"
#include "GameJournal.h"
#include "Label.h"
#include "ResManager.h"
#include "TextArea.h"
#include "Window.h"
#include "2DAResource.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

// The same window's chapter title area (5), the previous/next chapter
// buttons (3/4) and - BG2 only - the section tabs (6-9: quests, completed
// quests, journal, personal notes) and the sort order button (10); ids and
// captions per GemRB's GUIJRNL.py.
static const uint32 kJournalChapterTitleID = 5;
static const uint32 kJournalPrevChapterID = 3;
static const uint32 kJournalNextChapterID = 4;
static const uint32 kJournalOrderID = 10;
static const struct { uint32 controlID; uint8 section; uint32 captionStrRef; }
kJournalSectionTabs[] = {
	{ 6, GameJournal::SECTION_QUEST, 45485 },
	{ 7, GameJournal::SECTION_DONE, 45486 },
	{ 8, GameJournal::SECTION_INFO, 15333 },
	{ 9, GameJournal::SECTION_USER, 45487 },
};
// BG2's window also has a label (right of the chapter title) that shows a
// literal "<NO TEXT>" placeholder; GemRB leaves its use (a sort-method
// caption) commented out, so it stays blank.
static const uint32 kJournalBlankLabelID = 268435466;
static const uint32 kJournalOrderStrRef = 4627;
static const uint32 kJournalChapterStrRef = 15873;	// BG2: "Chapter <CurrentChapter>"
static const uint32 kJournalBG1ChapterFirstStrRef = 16202;	// BG1: one title per chapter
static const uint32 kJournalDateStrRef = 15980;
static const uint32 kJournalDayMonthStrRef = 15981;


// GUIJRNL.CHU window 2 (confirmed via a real dump): id 1 is the main
// scrollable entries text_area (with its own scrollbar at id 2, same
// pairing convention as GUIREC's saves/resistances area).
static const uint32 kJournalEntriesAreaID = 1;


JournalScreen::JournalScreen(Game& game)
	:
	PanelScreen(game, "GUIJRNL"),
	fChapter(0),
	fSection(GameJournal::SECTION_QUEST),
	fReverse(false)
{
}


/* virtual */
uint32
JournalScreen::CommandBarButton() const
{
	return 2;
}


// Opens on the current chapter.
/* virtual */
void
JournalScreen::OnOpen()
{
	const int32 chapter = Core::Get()->Vars().Get("CHAPTER");
	fChapter = chapter > 65535 ? 0 : chapter;
	Window* window = GetWindow(kContentWindow);
	if (window == NULL)
		return;

	for (const auto& tab : kJournalSectionTabs) {
		if (Button* button = dynamic_cast<Button*>(window->GetControlByID(tab.controlID)))
			button->SetText(IDTable::GetDialog(tab.captionStrRef));
	}
	if (Button* order = dynamic_cast<Button*>(window->GetControlByID(kJournalOrderID)))
		order->SetText(IDTable::GetDialog(kJournalOrderStrRef));
}


/* virtual */
void
JournalScreen::PanelControlInvoked(uint16 windowID, uint32 controlID)
{
	if (windowID != kContentWindow)
		return;

	const int32 currentChapter = Core::Get()->Vars().Get("CHAPTER");
	if (controlID == kJournalPrevChapterID) {
		// Chapters start at 1 in BG2 (unless a game already runs at 0),
		// at 0 in BG1.
		const int32 first = Core::Get()->Game() == game::GAME_BALDURSGATE2 && currentChapter > 0
			? 1 : 0;
		if (fChapter > first)
			fChapter--;
	} else if (controlID == kJournalNextChapterID) {
		if (fChapter < currentChapter)
			fChapter++;
	} else if (controlID == kJournalOrderID) {
		fReverse = !fReverse;
	} else {
		for (const auto& tab : kJournalSectionTabs) {
			if (tab.controlID == controlID)
				fSection = tab.section;
		}
	}
	RefreshContent();
}


// Replaces every "<TOKEN>" of `text` with its value.
static std::string
_ReplaceTokens(std::string text, const std::map<std::string, std::string>& tokens)
{
	for (const auto& token : tokens) {
		const std::string pattern = "<" + token.first + ">";
		for (size_t at = text.find(pattern); at != std::string::npos; at = text.find(pattern))
			text.replace(at, pattern.length(), token.second);
	}
	return text;
}


// The date line above a journal entry ("Day 12, 3rd of Kythorn 1369"-style
// - whatever the game's own string 15980 says), worked out from the
// entry's game time the way GemRB's GUIJRNL.py does: hours and days since
// the start, the year and calendar position counted from YEARS.2DA's start
// values, the month from MONTHS.2DA.
static std::string
_JournalDateLine(uint32 gameSeconds)
{
	int32 startTime = 0, startYear = 0;
	if (TWODAResource* years = gResManager->Get2DA("YEARS")) {
		startTime = years->IntegerValueFor("STARTTIME", "VALUE") / 4500;
		startYear = years->IntegerValueFor("STARTYEAR", "VALUE");
		gResManager->ReleaseResource(years);
	}

	const uint32 hours = gameSeconds / 3600;
	const uint32 days = hours / 24;
	int32 dayAndMonth = startTime + (int32)(days % 365);

	std::map<std::string, std::string> tokens;
	if (TWODAResource* months = gResManager->Get2DA("MONTHS")) {
		int32 month = 1;
		for (int32 row = 0; row < months->CountRows(); row++) {
			const int32 length = months->IntegerValueAt(row, 0);
			if (dayAndMonth < length) {
				tokens["DAY"] = std::to_string(dayAndMonth + 1);
				tokens["MONTHNAME"] = IDTable::GetDialog(months->IntegerValueAt(row, 1));
				tokens["MONTH"] = std::to_string(month);
				break;
			}
			dayAndMonth -= length;
			// Single days (festivals) aren't months.
			if (length != 1)
				month++;
		}
		gResManager->ReleaseResource(months);
	}

	std::map<std::string, std::string> lineTokens;
	lineTokens["GAMEDAYS"] = std::to_string(days);	// BG2's name for it...
	lineTokens["GAMEDAY"] = std::to_string(days);	// ...and BG1's
	lineTokens["HOUR"] = std::to_string(hours % 24);
	lineTokens["YEAR"] = std::to_string(startYear + (int32)(days / 365));
	lineTokens["DAYANDMONTH"] = _ReplaceTokens(IDTable::GetDialog(kJournalDayMonthStrRef), tokens);
	return _ReplaceTokens(IDTable::GetDialog(kJournalDateStrRef), lineTokens);
}


// One journal note in the text area: its title (first line, BG2 notes have
// one), the date it was made and the rest. TextArea has no newline
// handling, so each line is added on its own; an empty one becomes a blank
// line.
static void
_AddJournalText(TextArea* area, const std::string& text)
{
	size_t start = 0;
	while (start <= text.length()) {
		size_t end = text.find('\n', start);
		if (end == std::string::npos)
			end = text.length();
		std::string line = text.substr(start, end - start);
		area->AddText(line.empty() ? " " : line.c_str());
		start = end + 1;
	}
}


// Fills the journal screen: the chapter's title and, in the text area, the
// entries made in the shown chapter (in BG2 only those of the shown
// section), each with the date it was made.
void
JournalScreen::RefreshContent()
{
	Window* window = GetWindow(kContentWindow);
	if (window == NULL)
		return;

	const bool hasSections = Core::Get()->Game() == game::GAME_BALDURSGATE2;

	if (TextArea* title = dynamic_cast<TextArea*>(window->GetControlByID(kJournalChapterTitleID))) {
		title->ClearText();
		std::string text;
		if (hasSections) {
			text = _ReplaceTokens(IDTable::GetDialog(kJournalChapterStrRef),
				{ { "CurrentChapter", std::to_string(fChapter) } });
		} else {
			text = IDTable::GetDialog(kJournalBG1ChapterFirstStrRef + fChapter);
		}
		if (!text.empty())
			title->AddText(text.c_str());
	}
	if (hasSections) {
		if (Label* blank = dynamic_cast<Label*>(window->GetControlByID(kJournalBlankLabelID)))
			blank->SetText("");
		for (const auto& tab : kJournalSectionTabs) {
			if (Button* button = dynamic_cast<Button*>(window->GetControlByID(tab.controlID)))
				button->SetToggled(tab.section == fSection);
		}
	}

	TextArea* entriesArea = dynamic_cast<TextArea*>(window->GetControlByID(kJournalEntriesAreaID));
	if (entriesArea == NULL)
		return;
	entriesArea->ClearText();

	std::vector<const journal_entry*> shown;
	for (const journal_entry& entry : fGame.Journal().Entries()) {
		if (entry.chapter == fChapter && (!hasSections || entry.section == fSection))
			shown.push_back(&entry);
	}
	if (fReverse)
		std::reverse(shown.begin(), shown.end());

	for (const journal_entry* entry : shown) {
		std::string text = IDTable::GetDialog(entry->strref);
		std::string body;
		if (hasSections) {
			// A BG2 note is "title\nbody": the date goes between them.
			size_t split = text.find('\n');
			_AddJournalText(entriesArea, text.substr(0, split));
			entriesArea->AddText(_JournalDateLine(entry->time).c_str());
			if (split != std::string::npos)
				body = text.substr(split + 1);
		} else {
			// BG1 notes have no title: the date, then the text.
			entriesArea->AddText(_JournalDateLine(entry->time).c_str());
			body = text;
		}
		_AddJournalText(entriesArea, body);
		entriesArea->AddText(" ");
	}
}
