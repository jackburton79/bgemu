#ifndef __MOVIEDECODER_H
#define __MOVIEDECODER_H

#include "Bitmap.h"
#include "IETypes.h"

#include <vector>

#define DEBUG 1

class GraphicsEngine;
class Stream;
class MovieDecoder {
public:
	MovieDecoder();
	~MovieDecoder();
	
	bool AllocateBuffer(uint16 width, uint16 height, uint16 version, bool trueColor);
	bool InitVideoMode(uint16 width, uint16 height, uint16 flags);
	void BlitBackBuffer();
	
	void SetDecodingMap(uint8 *map, uint32 size);
	void SetPalette(uint16 start, uint16 count, uint8 palette[]);
	void DecodeDataBlock(Stream *stream, uint32 length);
	
	Bitmap *CurrentFrame();

	int Test();

private:
	uint8 *fDecodingMap;
	uint32 fMapSize;
	
	Bitmap *fNewFrame;
	Bitmap *fCurrentFrame;
	Bitmap *fScratchBuffer;
	
	Color fColors[256];
	GFX::rect fActiveRect;
	uint16 fVersion;

	void Opcode0(Stream* stream, uint8* pixels, GFX::rect* rect);
	void Opcode1(Stream* stream, uint8* pixels, GFX::rect* rect);
	void Opcode2(Stream* stream, uint8* pixels, GFX::rect* rect);
	void Opcode3(Stream* stream, uint8* pixels, GFX::rect* rect);
	void Opcode4(Stream* stream, uint8* pixels, GFX::rect* rect);
	void Opcode5(Stream* stream, uint8* pixels, GFX::rect* rect);
	void Opcode6(Stream* stream, uint8* pixels, GFX::rect* rect);
	void Opcode7(Stream* stream, uint8* pixels, GFX::rect* rect);
	void Opcode8(Stream* stream, uint8* pixels, GFX::rect* rect);
	void Opcode9(Stream* stream, uint8* pixels, GFX::rect* rect);
	void OpcodeA(Stream* stream, uint8* pixels, GFX::rect* rect);
	void OpcodeB(Stream* stream, uint8* pixels, GFX::rect* rect);
	void OpcodeC(Stream* stream, uint8* pixels, GFX::rect* rect);
	void OpcodeD(Stream* stream, uint8* pixels, GFX::rect* rect);
	void OpcodeE(Stream* stream, uint8* pixels, GFX::rect* rect);
	void OpcodeF(Stream* stream, uint8* pixels, GFX::rect* rect);

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

typedef void (MovieDecoder::*opcode)(Stream* stream, uint8* pixels, GFX::rect* rect);

#endif // __MOVIEDECODER_H
