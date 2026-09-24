/*
 * PlaylistStream.h
 *
 * Plays a MusPlaylist as one AudioStream: the tracks one after the other,
 * following the playlist's loops, without end - until it is asked to end, when
 * it plays the current entry's interrupt track and finishes.
 */

#pragma once

#include "ACMStream.h"
#include "AudioStream.h"
#include "MusPlaylist.h"

#include <atomic>
#include <memory>
#include <string>

class PlaylistStream : public AudioStream {
public:
	// Opens the first playable track from `startIndex` on. Check Valid().
	PlaylistStream(const MusPlaylist& playlist, size_t startIndex = 0);

	bool Valid() const;

	virtual uint16 Channels() const;
	virtual uint32 SampleRate() const;
	virtual size_t Read(uint8* buffer, size_t bytes);

	// Main thread: after the track now playing, play its interrupt track (if it
	// has one) and finish.
	void RequestEnd();

	// The entry playing now, -1 when its interrupt track plays.
	int CurrentIndex() const;
	// The name of the track playing now (the interrupt track's, in that case).
	std::string CurrentTrack() const;
	const MusPlaylist& Playlist() const;

private:
	bool _OpenTrack(const std::string& path);
	// Opens the next track to play; false if there is none (the end).
	bool _OpenNext();

	MusPlaylist fPlaylist;
	std::unique_ptr<ACMStream> fTrack;
	uint16 fChannels;
	uint32 fSampleRate;

	std::atomic<int> fIndex;		// the current entry (or the last, during the interrupt track)
	std::atomic<bool> fPlayingInterrupt;
	std::atomic<bool> fEndRequested;
	bool fStarted;
	bool fFinished;
	std::string fInterruptPath;
};
