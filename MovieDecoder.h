#ifndef __MOVIEDECODER_H
#define __MOVIEDECODER_H

#include "Bitmap.h"
#include "IETypes.h"

#include <vector>

class Stream;
class MovieDecoder {
public:
	MovieDecoder();
	~MovieDecoder();
	
	bool AllocateBuffer(uint16 width, uint16 height);
	bool InitVideoMode(uint16 width, uint16 height, uint16 flags);
	void BlitBackBuffer();
	
	void SetDecodingMap(uint8 *map, uint32 size);
	void SetPalette(uint16 start, uint16 count, uint8 palette[]);
	void DecodeDataBlock(Stream *stream, uint32 length);
	
	Bitmap *CurrentFrame();

	void Test();

private:
	uint8 *fDecodingMap;
	uint8 *fDecodingPointer;
	uint32 fMapSize;
	
	Bitmap *fNewFrame;
	Bitmap *fCurrentFrame;
	Bitmap *fScratchBuffer;
	
	Color fColors[256];
	GFX::rect fActiveRect;
};

#endif // __MOVIEDECODER_H
