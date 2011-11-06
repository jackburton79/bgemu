#ifndef __MOVIEDECODER_H
#define __MOVIEDECODER_H

#include "SDL.h"
#include "Types.h"

class TStream;
class MovieDecoder {
public:
	MovieDecoder();
	~MovieDecoder();
	
	void AllocateSurface(uint16 width, uint16 height);
	
	void SetDecodingMap(uint8 *map, uint32 size);
	void SetPalette(uint16 start, uint16 count, uint8 palette[]);
	void DecodeDataBlock(TStream *stream, uint32 length);
	
	SDL_Surface *CurrentFrame() const;
	
private:
	uint8 *fDecodingMap;
	uint8 *fDecodingPointer;
	uint32 fMapSize;
	
	SDL_Surface *fCurrentFrame;
	SDL_Surface *fPreviousFrame;
	
	SDL_Rect fActiveRect;
};

#endif // __MOVIEDECODER_H