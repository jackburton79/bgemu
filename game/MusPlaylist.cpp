/*
 * MusPlaylist.cpp - see MusPlaylist.h
 */

#include "MusPlaylist.h"

#include "GameFiles.h"

#include <fstream>
#include <sstream>
#include <strings.h>

static std::vector<std::string>
_Tokens(const std::string& line)
{
	std::vector<std::string> tokens;
	std::istringstream stream(line);
	std::string token;
	while (stream >> token)
		tokens.push_back(token);
	return tokens;
}


/* static */
bool
MusPlaylist::Parse(const std::string& text, MusPlaylist& playlist)
{
	playlist.fEntries.clear();

	std::istringstream stream(text);
	std::string line;
	if (!std::getline(stream, line))
		return false;
	std::vector<std::string> nameTokens = _Tokens(line);
	if (nameTokens.empty())
		return false;
	playlist.fFolder = nameTokens[0];

	if (!std::getline(stream, line))
		return false;
	std::vector<std::string> countTokens = _Tokens(line);
	int count = countTokens.empty() ? 0 : atoi(countTokens[0].c_str());

	while (count > 0 && std::getline(stream, line)) {
		const std::vector<std::string> tokens = _Tokens(line);
		count--;
		if (tokens.empty())
			continue;

		MusEntry entry;
		entry.track = tokens[0];
		size_t i = 1;
		if (i < tokens.size() && tokens[i][0] != '@') {
			// "<track> <loop track> @TAG <end>", or "<track> <folder> <loop track>".
			const std::string second = tokens[i++];
			if (i < tokens.size() && tokens[i][0] == '@') {
				entry.loopTrack = second;
			} else if (i < tokens.size()) {
				entry.loopFolder = second;
				entry.loopTrack = tokens[i++];
			}
		}
		// The "@TAG" marker, then the interrupt track.
		if (i < tokens.size())
			i++;
		if (i < tokens.size())
			entry.end = tokens[i];
		playlist.fEntries.push_back(entry);
	}

	// A loop into another folder that isn't an entry of the list: an entry of
	// its own that loops to itself.
	const size_t parsed = playlist.fEntries.size();
	for (size_t i = 0; i < parsed; i++) {
		const MusEntry entry = playlist.fEntries[i];
		if (entry.loopTrack.empty()
				|| playlist.IndexOf(entry.loopTrack, entry.loopFolder) >= 0) {
			continue;
		}
		MusEntry loop;
		loop.folder = entry.loopFolder;
		loop.track = entry.loopTrack;
		loop.loopFolder = entry.loopFolder;
		loop.loopTrack = entry.loopTrack;
		playlist.fEntries.push_back(loop);
	}
	return !playlist.fEntries.empty();
}


/* static */
bool
MusPlaylist::Load(const std::string& name, MusPlaylist& playlist)
{
	const std::string path = FindGameFile("music/" + name);
	if (path.empty())
		return false;

	std::ifstream file(path);
	std::stringstream text;
	text << file.rdbuf();
	return Parse(text.str(), playlist);
}


const std::string&
MusPlaylist::Folder() const
{
	return fFolder;
}


size_t
MusPlaylist::Count() const
{
	return fEntries.size();
}


const MusEntry&
MusPlaylist::At(size_t index) const
{
	return fEntries.at(index);
}


bool
MusPlaylist::_SameFolder(const std::string& a, const std::string& b) const
{
	const std::string& left = a.empty() ? fFolder : a;
	const std::string& right = b.empty() ? fFolder : b;
	return strcasecmp(left.c_str(), right.c_str()) == 0;
}


int
MusPlaylist::IndexOf(const std::string& track, const std::string& folder) const
{
	for (size_t i = 0; i < fEntries.size(); i++) {
		if (strcasecmp(fEntries[i].track.c_str(), track.c_str()) == 0
				&& _SameFolder(fEntries[i].folder, folder)) {
			return (int)i;
		}
	}
	return -1;
}


int
MusPlaylist::Next(size_t index) const
{
	if (index >= fEntries.size())
		return -1;

	const MusEntry& entry = fEntries[index];
	if (!entry.loopTrack.empty()) {
		const int target = IndexOf(entry.loopTrack, entry.loopFolder);
		if (target >= 0)
			return target;
	}
	return (int)((index + 1) % fEntries.size());
}


std::string
MusPlaylist::_Path(const std::string& folderName, const std::string& track) const
{
	if (strncasecmp(track.c_str(), "SPC", 3) == 0)
		return "music/" + track + ".acm";
	const std::string& folder = folderName.empty() ? fFolder : folderName;
	return "music/" + folder + "/" + folder + track + ".acm";
}


std::string
MusPlaylist::TrackPath(size_t index) const
{
	if (index >= fEntries.size())
		return "";
	return _Path(fEntries[index].folder, fEntries[index].track);
}


std::string
MusPlaylist::InterruptPath(size_t index) const
{
	if (index >= fEntries.size())
		return "";
	// "end" is a marker for "no interrupt track" in a few files.
	const MusEntry& entry = fEntries[index];
	if (entry.end.empty() || strcasecmp(entry.end.c_str(), "end") == 0)
		return "";
	return _Path(entry.folder, entry.end);
}
