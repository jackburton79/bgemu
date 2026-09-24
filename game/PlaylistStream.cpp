/*
 * PlaylistStream.cpp - see PlaylistStream.h
 */

#include "PlaylistStream.h"

#include "GameFiles.h"
#include "Log.h"

#include <iostream>

PlaylistStream::PlaylistStream(const MusPlaylist& playlist, size_t startIndex)
	:
	fPlaylist(playlist),
	fChannels(0),
	fSampleRate(0),
	fIndex((int)startIndex),
	fPlayingInterrupt(false),
	fEndRequested(false),
	fStarted(false),
	fFinished(false)
{
	// Open the first track now: the sound engine needs the stream's format
	// before it starts pulling.
	if (startIndex >= fPlaylist.Count())
		startIndex = 0;
	fIndex = (int)startIndex;
	for (size_t attempt = 0; attempt < fPlaylist.Count(); attempt++) {
		const size_t index = (size_t)fIndex.load();
		if (_OpenTrack(fPlaylist.TrackPath(index))) {
			fInterruptPath = fPlaylist.InterruptPath(index);
			fStarted = true;
			return;
		}
		fIndex = fPlaylist.Next(index);
	}
	fFinished = true;
}


bool
PlaylistStream::Valid() const
{
	return fStarted;
}


uint16
PlaylistStream::Channels() const
{
	return fChannels;
}


uint32
PlaylistStream::SampleRate() const
{
	return fSampleRate;
}


bool
PlaylistStream::_OpenTrack(const std::string& relativePath)
{
	const std::string path = FindGameFile(relativePath);
	std::unique_ptr<ACMStream> stream(path.empty() ? nullptr : ACMStream::Open(path));
	if (!stream) {
		std::cerr << Log::Yellow << "PlaylistStream: can't open track "
			<< relativePath << Log::Normal << std::endl;
		return false;
	}
	if (fChannels == 0) {
		fChannels = stream->Channels();
		fSampleRate = stream->SampleRate();
	} else if (stream->Channels() != fChannels || stream->SampleRate() != fSampleRate) {
		std::cerr << Log::Yellow << "PlaylistStream: track " << relativePath
			<< " has another format than the playlist's, skipped" << Log::Normal << std::endl;
		return false;
	}
	fTrack = std::move(stream);
	return true;
}


bool
PlaylistStream::_OpenNext()
{
	if (fFinished)
		return false;

	if (fPlayingInterrupt) {
		fFinished = true;
		return false;
	}

	const size_t current = (size_t)fIndex.load();
	if (fEndRequested) {
		// The current entry's ending, then the end of the playlist.
		fFinished = true;
		if (!fInterruptPath.empty() && _OpenTrack(fInterruptPath)) {
			fPlayingInterrupt = true;
			fFinished = false;
			return true;
		}
		return false;
	}

	size_t index = current;
	for (size_t attempt = 0; attempt < fPlaylist.Count(); attempt++) {
		const int next = fPlaylist.Next(index);
		if (next < 0)
			break;
		index = (size_t)next;
		if (_OpenTrack(fPlaylist.TrackPath(index))) {
			fIndex = (int)index;
			fInterruptPath = fPlaylist.InterruptPath(index);
			return true;
		}
	}
	fFinished = true;
	return false;
}


size_t
PlaylistStream::Read(uint8* buffer, size_t bytes)
{
	size_t total = 0;
	while (total < bytes) {
		if (!fTrack) {
			if (!_OpenNext())
				break;
		}
		const size_t got = fTrack->Read(buffer + total, bytes - total);
		if (got == 0) {
			fTrack.reset();
			continue;
		}
		total += got;
	}
	return total;
}


void
PlaylistStream::RequestEnd()
{
	fEndRequested = true;
}


int
PlaylistStream::CurrentIndex() const
{
	return fPlayingInterrupt ? -1 : fIndex.load();
}


std::string
PlaylistStream::CurrentTrack() const
{
	if (fPlayingInterrupt) {
		const int index = fIndex.load();
		return index >= 0 ? fPlaylist.At((size_t)index).end : "";
	}
	const int index = fIndex.load();
	return index >= 0 && (size_t)index < fPlaylist.Count() ? fPlaylist.At((size_t)index).track : "";
}


const MusPlaylist&
PlaylistStream::Playlist() const
{
	return fPlaylist;
}
