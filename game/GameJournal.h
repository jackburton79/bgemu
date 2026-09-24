#pragma once

#include "IETypes.h"

#include <vector>


// One journal entry. `section` is one of GameJournal::Section; `time` is in
// game seconds (GameTimer::GameTime()) at the moment the entry was added or
// last moved between sections, `chapter` the CHAPTER global then.
struct journal_entry {
	uint32 strref;
	uint8 section;
	uint8 group;
	uint8 chapter;
	uint32 time;
};


// The journal: entries added by ADDJOURNALENTRY, moved/removed by
// SETQUESTDONE/ERASEJOURNALENTRY, and by dialog transitions that carry a
// journal note (DialogHandler). Semantics follow GemRB's Game::
// AddJournalEntry(). What the journal screen shows is JournalScreen's.
class GameJournal {
public:
	enum Section {
		SECTION_USER = 0,
		SECTION_QUEST = 1,
		SECTION_DONE = 2,
		SECTION_INFO = 4
	};

	// An entry is unique per strref - adding one that exists in the same
	// section changes nothing (returns false), in another section moves it
	// there (or, finishing a quest that belongs to a group, replaces the
	// whole group with it).
	bool Add(uint32 strref, uint8 section, uint8 group = 0);
	void Remove(uint32 strref);
	void RemoveGroup(uint8 group);

	const std::vector<journal_entry>& Entries() const;
	// Strrefs only, in order - for the console and tests.
	std::vector<uint32> Strrefs() const;
	void Set(const std::vector<journal_entry>& entries);

private:
	std::vector<journal_entry> fEntries;
};
