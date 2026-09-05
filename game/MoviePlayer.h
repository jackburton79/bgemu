#ifndef __MOVIEPLAYER_H
#define __MOVIEPLAYER_H

class MVEResource;

// Drives movie playback: pumps an MVEResource chunk by chunk, paces
// frames according to the timing the stream itself specifies, presents
// each decoded frame to the screen, and handles pause (P)/quit (Q) input
// - none of which the resource (a pure MVE container-format parser) or
// the decoder (the video codec) know about. See MVEResource for how the
// two communicate (DecodeNextChunk()/ConsumeFrameReady()/CurrentFrame()/
// FrameDelay()/LastFrameTime()).
class MoviePlayer {
public:
	void Play(MVEResource* resource);
};

#endif // __MOVIEPLAYER_H
