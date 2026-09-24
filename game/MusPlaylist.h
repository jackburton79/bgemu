/*
 * MusPlaylist.h
 *
 * A music playlist: a .mus file of the game's music/ directory (see
 * docs/iesdp-gh-pages/file_formats/ie_formats/mus.htm). Its first line names the
 * subfolder of music/ that holds the tracks (BC1 -> music/BC1/BC1<track>.acm),
 * the second the number of entries, then one line per entry:
 *
 *   <track> [<loop track>] [@TAG <interrupt track>]
 *   <track> <loop folder> <loop track>
 *
 * An entry that has a loop track sends the playlist there when it ends instead of
 * on to the next line; without any, the playlist goes on to the next line and
 * back to the first after the last. A loop in another folder than the
 * playlist's (Blank.mus loops to BLANK/BLANKA.acm) becomes an entry of its own
 * at the end of the list, which repeats. The interrupt track is the ending (a
 * fade-out phrase) to play when the music is switched off. A track named
 * SPCn is a silence file in music/ itself. Read the way GemRB reads them.
 */

#pragma once

#include <string>
#include <vector>

struct MusEntry {
	std::string folder;	// the subfolder of the track, "" = the playlist's own
	std::string track;
	std::string loopFolder;	// where the loop track is, "" = the playlist's own
	std::string loopTrack;	// "" = none
	std::string end;	// the interrupt track (in `folder`), "" = none
};


class MusPlaylist {
public:
	// Parses the text of a .mus file. False if it has no name or entries.
	static bool Parse(const std::string& text, MusPlaylist& playlist);
	// Reads `name` (with its .mus extension) from the game's music/ directory.
	static bool Load(const std::string& name, MusPlaylist& playlist);

	// The subfolder holding the tracks.
	const std::string& Folder() const;
	size_t Count() const;
	const MusEntry& At(size_t index) const;
	// The entry of this track (compared without regard to case), -1 if none;
	// `folder` "" is the playlist's own.
	int IndexOf(const std::string& track, const std::string& folder = "") const;

	// The entry played after entry `index`: its loop entry, else the next line
	// (the first after the last).
	int Next(size_t index) const;

	// Where the track of entry `index` is, relative to the game's directory.
	std::string TrackPath(size_t index) const;
	// Where its interrupt track is ("" if it has none).
	std::string InterruptPath(size_t index) const;

private:
	std::string _Path(const std::string& folder, const std::string& track) const;
	bool _SameFolder(const std::string& a, const std::string& b) const;

	std::string fFolder;
	std::vector<MusEntry> fEntries;
};
