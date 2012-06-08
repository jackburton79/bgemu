#ifndef __MOVIEDECODER_H
#define __MOVIEDECODER_H

#include "Bitmap.h"
#include "IETypes.h"

#include <vector>

#define DEBUG 0

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
	uint32 fMapSize;
	
	Bitmap *fNewFrame;
	Bitmap *fCurrentFrame;
	Bitmap *fScratchBuffer;
	
	Color fColors[256];
	GFX::rect fActiveRect;

#if DEBUG
	void TestInit(uint8 opcode, const uint8 data[], uint32 size);
	void TestFinish(const uint8 expectedResult[], uint32 dataSize);
	void TestOpcode7A();
	void TestOpcode7B();
	void TestOpcode8A();
	void TestOpcode8B();
	void TestOpcode8C();
	void TestOpcode9A();
	void TestOpcode9B();
	void TestOpcodeA2();
	void TestOpcodeB();
	void TestOpcodeC();
	void TestOpcodeD();
	void TestOpcodeE();
	void TestOpcodeF();

#endif
};

#endif // __MOVIEDECODER_H
