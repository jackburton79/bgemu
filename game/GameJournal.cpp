#include "GameJournal.h"

#include "Core.h"
#include "GameTimer.h"

#include <algorithm>


bool
GameJournal::Add(uint32 strref, uint8 section, uint8 group)
{
	const uint8 chapter = (uint8)Core::Get()->Vars().Get("CHAPTER");

	for (journal_entry& entry : fEntries) {
		if (entry.strref != strref)
			continue;
		// Already there: nothing to do in the same section.
		if (entry.section == section)
			return false;
		// Finishing a quest of a group replaces the group with this entry.
		if (section == SECTION_DONE && group != 0) {
			RemoveGroup(group);
			break;
		}
		entry.section = section;
		entry.group = group;
		entry.chapter = chapter;
		entry.time = GameTimer::GameTime();
		return true;
	}

	journal_entry entry;
	entry.strref = strref;
	entry.section = section;
	entry.group = group;
	entry.chapter = chapter;
	entry.time = GameTimer::GameTime();
	fEntries.push_back(entry);
	return true;
}


void
GameJournal::Remove(uint32 strref)
{
	auto it = std::find_if(fEntries.begin(), fEntries.end(),
		[strref](const journal_entry& entry) { return entry.strref == strref; });
	if (it != fEntries.end())
		fEntries.erase(it);
}


void
GameJournal::RemoveGroup(uint8 group)
{
	fEntries.erase(std::remove_if(fEntries.begin(), fEntries.end(),
		[group](const journal_entry& entry) { return entry.group == group; }),
		fEntries.end());
}


const std::vector<journal_entry>&
GameJournal::Entries() const
{
	return fEntries;
}


std::vector<uint32>
GameJournal::Strrefs() const
{
	std::vector<uint32> strrefs;
	for (const journal_entry& entry : fEntries)
		strrefs.push_back(entry.strref);
	return strrefs;
}


void
GameJournal::Set(const std::vector<journal_entry>& entries)
{
	fEntries = entries;
}
