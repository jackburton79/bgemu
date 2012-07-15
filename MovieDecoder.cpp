#include "Bitmap.h"
#include "GraphicsEngine.h"
#include "MemoryStream.h"
#include "MovieDecoder.h"
#include "Stream.h"
#include "SDL.h"

#include <assert.h>
#include <iostream>

#define CALL_MEMBER_FUNCTION(object, ptrToMember)  ((object).*(ptrToMember))


const static int kOffsets[2][16] = {
		{0, 2, 4, 6, 16, 18, 20, 22, 32, 34, 36, 38, 48, 50, 52, 54 },
		{0, 4, 32, 36, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};


static opcode sOpcodes[16] = {NULL};

// TODO: works for 8x8x8 bitmaps only
static inline void
fill_pixels(uint8 *pixels, uint16 offset, uint8 value, uint16 blockSize)
{
	uint8* ptr = pixels + kOffsets[(blockSize / 2) - 1][offset];
	for (int16 r = 0; r < blockSize; r++) {
		for (int16 c = 0; c < blockSize; c++)
			ptr[c + r * 8] = value;
	}
}


static void
DumpData(uint8 *data, int size)
{
	for (int i = 0; i < size; i++) {
		printf("0x%x ", *(data + i));
		if ((i % 8) == 7)
			printf("\n");
	}
	printf("\n");
}


class Pattern4 {
public:
	Pattern4(const uint8& p0, const uint8& p1, const uint8& p2, const uint8& p3)
	{
		fP[0] = p0;
		fP[1] = p1;
		fP[2] = p2;
		fP[3] = p3;
	}
	inline uint8 PixelValue(const uint8 &bits)
	{
		return fP[bits];
	}
private:
	uint8 fP[4];
};


class Opcode2 {
public:
	Opcode2(const GFX::rect& rect, uint8& byte)
		:
		fRect(rect),
		fByte(byte)
	{
	}

	GFX::rect Rect(bool opcode3) const
	{
		GFX::rect rect = fRect;
		sint8 xValue;
		sint8 yValue;
		if (fByte < 56) {
			xValue = 8 + (fByte % 7);
			yValue = fByte / 7;
		} else {
			xValue = -14 + ((fByte - 56) % 29);
			yValue =  8 + ((fByte - 56) / 29);
		}
		if (opcode3) {
			rect.x -= xValue;
			rect.y -= yValue;
		} else {
			rect.x += xValue;
			rect.y += yValue;
		}

		if (rect.x < 0 || rect.y < 0) {
			throw -1;
		}
		return rect;
	}
private:
	const GFX::rect &fRect;
	const uint8 &fByte;
};


class BitStreamAdapter {
public:
	BitStreamAdapter(Stream* stream);
	uint8 ReadBit();

private:
	Stream* fStream;
	uint8 fByte;
	int8 fBitPos;
};


class TwoBitsStreamAdapter {
public:
	TwoBitsStreamAdapter(Stream* stream);
	uint8 ReadBits();

private:
	Stream* fStream;
	uint8 fByte;
	int8 fBitPos;
};


MovieDecoder::MovieDecoder()
	:
	fDecodingMap(NULL),
	fMapSize(0),
	fNewFrame(NULL),
	fCurrentFrame(NULL)
{
	fActiveRect.x = fActiveRect.y = 0;
	fActiveRect.w = fActiveRect.h = 8;
	fScratchBuffer = GraphicsEngine::CreateBitmap(8, 8, 8);

	sOpcodes[0] = &MovieDecoder::Opcode0_8;
	sOpcodes[1] = &MovieDecoder::Opcode1_8;
	sOpcodes[2] = &MovieDecoder::Opcode2_8;
	sOpcodes[3] = &MovieDecoder::Opcode3_8;
	sOpcodes[4] = &MovieDecoder::Opcode4_8;
	sOpcodes[5] = &MovieDecoder::Opcode5_8;
	sOpcodes[6] = &MovieDecoder::Opcode6_8;
	sOpcodes[7] = &MovieDecoder::Opcode7_8;
	sOpcodes[8] = &MovieDecoder::Opcode8_8;
	sOpcodes[9] = &MovieDecoder::Opcode9_8;
	sOpcodes[10] = &MovieDecoder::OpcodeA_8;
	sOpcodes[11] = &MovieDecoder::OpcodeB_8;
	sOpcodes[12] = &MovieDecoder::OpcodeC_8;
	sOpcodes[13] = &MovieDecoder::OpcodeD_8;
	sOpcodes[14] = &MovieDecoder::OpcodeE_8;
	sOpcodes[15] = &MovieDecoder::OpcodeF_8;

#if DEBUG
	Test();
#endif
}


MovieDecoder::~MovieDecoder()
{
	delete[] fDecodingMap;
	GraphicsEngine::DeleteBitmap(fNewFrame);
	GraphicsEngine::DeleteBitmap(fCurrentFrame);
	GraphicsEngine::DeleteBitmap(fScratchBuffer);
}


bool
MovieDecoder::AllocateBuffer(uint16 width, uint16 height, uint16 version, bool trueColor)
{
	printf("AllocateBuffer(width: %d, height: %d, version: %d, truecolor: %s)\n",
			width, height, version, trueColor ? "yes" : "no");
	fNewFrame = GraphicsEngine::CreateBitmap(width, height, 8);
	fCurrentFrame = GraphicsEngine::CreateBitmap(width, height, 8);
	fVersion = version;

	return true;
}


bool
MovieDecoder::InitVideoMode(uint16 width, uint16 height, uint16 flags)
{
	printf("InitVideoMode(width: %d, height: %d, flags: %d)\n",
			width, height, flags);
	GraphicsEngine* gfxEngine = GraphicsEngine::Get();
	gfxEngine->SaveCurrentMode();
	gfxEngine->SetVideoMode(width, height, 16, 0);

	return true;
}


void
MovieDecoder::SetDecodingMap(uint8 *map, uint32 size)
{
	delete[] fDecodingMap;
		
	fDecodingMap = map;
	fMapSize = size;
}


void
MovieDecoder::SetPalette(uint16 start, uint16 count, uint8 palette[])
{
	for (int32 i = start; i < start + count; i++) {
		fColors[i].r = *palette++ << 2;
		fColors[i].g = *palette++ << 2;
		fColors[i].b = *palette++ << 2;
		fColors[i].a = 0;
	}

	fNewFrame->SetColors(&fColors[start], start, count);
	fCurrentFrame->SetColors(&fColors[start], start, count);
	fScratchBuffer->SetColors(&fColors[start], start, count);
}


void
MovieDecoder::DecodeDataBlock(Stream *stream, uint32 length)
{
	fActiveRect.x = fActiveRect.y = 0;

	uint32 pos = stream->Position();

	int dataOffset = 14;
	//if (fVersion == 2)
		//dataOffset = 16;
	// TODO: WHY ??? FIND OUT
	stream->Seek(dataOffset, SEEK_CUR);

	for (uint32 i = 0; i < fMapSize; i++) {
		for (int b = 0; b < 2; b++) {
			uint8 opcode;
			if (b == 0)
				opcode = fDecodingMap[i] & 0x0F;
			else
				opcode = fDecodingMap[i] >> 4;

			GFX::rect blitRect = fActiveRect;

			CALL_MEMBER_FUNCTION(*this, sOpcodes[opcode])(stream,
					(uint8*)fScratchBuffer->Pixels(), &blitRect);

			fActiveRect.x += 8;
			if (fActiveRect.x >= fNewFrame->Width()) {
				fActiveRect.x = 0;
				fActiveRect.y += 8;
			}
		}
	}

	stream->Seek(pos + length, SEEK_SET);
}


void
MovieDecoder::BlitBackBuffer()
{
	GraphicsEngine::Get()->BlitToScreen(fNewFrame, NULL, NULL);
	GraphicsEngine::Get()->Flip();

	std::swap(fNewFrame, fCurrentFrame);
}


// opcodes
void
MovieDecoder::Opcode0_8(Stream* stream, uint8* pixels, GFX::rect* rect)
{
	GraphicsEngine::Get()->BlitBitmap(fCurrentFrame, rect, fNewFrame, rect);
}


void
MovieDecoder::Opcode1_8(Stream* stream, uint8* pixels, GFX::rect* rect)
{
}


void
MovieDecoder::Opcode2_8(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	uint8 byte = stream->ReadByte();
	GFX::rect rect = Opcode2(fActiveRect, byte).Rect(false);
	GraphicsEngine::Get()->BlitBitmap(fNewFrame, &rect, fNewFrame, blitRect);
}


void
MovieDecoder::Opcode3_8(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	// Same as above but negated
	uint8 byte = stream->ReadByte();
	GFX::rect rect = Opcode2(fActiveRect, byte).Rect(true);
	GraphicsEngine::Get()->BlitBitmap(fNewFrame, &rect, fNewFrame, blitRect);
}


void
MovieDecoder::Opcode4_8(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	uint8 byte = stream->ReadByte();
	GFX::rect rect = fActiveRect;
	rect.x += -8 + (byte & 0x0F);
	rect.y += -8 + (byte >> 4);
	GraphicsEngine::Get()->BlitBitmap(fCurrentFrame, &rect, fNewFrame, blitRect);
}


void
MovieDecoder::Opcode5_8(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	GFX::rect rect = fActiveRect;
	rect.x += (sint8)stream->ReadByte();
	rect.y += (sint8)stream->ReadByte();
	GraphicsEngine::Get()->BlitBitmap(fCurrentFrame, &rect, fNewFrame, blitRect);
}


void
MovieDecoder::Opcode6_8(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
}


void
MovieDecoder::Opcode7_8(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	uint8 p0 = stream->ReadByte();
	uint8 p1 = stream->ReadByte();
	BitStreamAdapter bs(stream);
	if (p0 < p1) {
		for (int32 c = 0; c < 64; c++)
			*pixels++ = bs.ReadBit() ? p1 : p0;
	} else {
		for (int32 i = 0; i < 16; i++)
			fill_pixels(pixels, i, bs.ReadBit() ? p1 : p0, 2);
	}
	GraphicsEngine::Get()->BlitBitmap(fScratchBuffer, NULL, fNewFrame, blitRect);
}


void
MovieDecoder::Opcode8_8(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	uint8 t0 = stream->ReadByte();
	uint8 t1 = stream->ReadByte();
	if (t0 <= t1) {
		stream->Seek(-2, SEEK_CUR);
		// TODO: Improve and simplify the code
		for (int qh = 0; qh < 2; qh++) { // 8x8
			for (int q = 0; q < 2; q++) { // 4x8
				uint8 *p = pixels + (q * 4) + qh * 32;
				uint8 p0 = stream->ReadByte();
				uint8 p1 = stream->ReadByte();
				BitStreamAdapter bs(stream);
				for (int32 c = 0; c < 16; c++) // 4x4
					p[c + 4 * (c / 4)] = bs.ReadBit() ? p1 : p0;

			} // 4x8
		} // 8x8
	} else {
		stream->Seek(4, SEEK_CUR);
		uint8 p2 = stream->ReadByte();
		uint8 p3 = stream->ReadByte();
		stream->Seek(-6, SEEK_CUR);
		if (p2 <= p3) { // LEFT-RIGHT
			{
				BitStreamAdapter bs(stream);
				for (int32 c = 0; c < 32; c++) {
					pixels[c + 4 * (c / 4)] = bs.ReadBit() ? t1 : t0;
				}
				stream->Seek(2, SEEK_CUR);
			}
			{
				pixels += 4;
				BitStreamAdapter bs(stream);
				for (int32 c = 0; c < 32; c++)
					pixels[c + 4 * (c / 4)] = bs.ReadBit() ? p3 : p2;
			}
		} else { // TOP-BOTTOM
			{
				BitStreamAdapter bs(stream);
				for (int32 x = 0; x < 32; x++)
					*pixels++ = bs.ReadBit() ? t1 : t0;
				stream->Seek(2, SEEK_CUR);
			}
			{
				BitStreamAdapter bs(stream);
				for (int32 x = 0; x < 32; x++)
					*pixels++ = bs.ReadBit() ? p3 : p2;
			}
		}
	}
	GraphicsEngine::Get()->BlitBitmap(fScratchBuffer, NULL, fNewFrame, blitRect);
}


void
MovieDecoder::Opcode9_8(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	uint8 p0 = stream->ReadByte();
	uint8 p1 = stream->ReadByte();
	uint8 p2 = stream->ReadByte();
	uint8 p3 = stream->ReadByte();
	Pattern4 op(p0, p1, p2, p3);
	TwoBitsStreamAdapter bs(stream);
	if (p0 <= p1 && p2 <= p3) {
		for (int i = 0; i < 64; i++)
			*pixels++ = op.PixelValue(bs.ReadBits());
	} else if (p0 <= p1 && p2 > p3) {
		for (int i = 0; i < 16; i++) {
			const uint8 value = op.PixelValue(bs.ReadBits());
			fill_pixels(pixels, i, value, 2);
		}
	} else if (p0 > p1 && p2 <= p3 ) {
		for (int i = 0; i < 32; i++) {
			const uint8 value = op.PixelValue(bs.ReadBits());
			*pixels++ = value;
			*pixels++ = value;
		}
	} else {
		for (int i = 0; i < 32; i++) {
			const uint8 value = op.PixelValue(bs.ReadBits());
			uint8 ind = i + (i / 8) * 8;
			pixels[ind] = value;
			pixels[ind + 8] = value;
		}
	}
	GraphicsEngine::Get()->BlitBitmap(fScratchBuffer, NULL, fNewFrame, blitRect);
}


void
MovieDecoder::OpcodeA_8(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	uint8 p0 = stream->ReadByte();
	uint8 p1 = stream->ReadByte();
	uint8 p2 = stream->ReadByte();
	uint8 p3 = stream->ReadByte();
	Pattern4 op0(p0, p1, p2, p3);
	if (p0 <= p1) {
		stream->Seek(-4, SEEK_CUR);
		for (int qh = 0; qh < 2; qh++) { // 8x8
			for (int q = 0; q < 2; q++) { // 4x8
				uint8 *p = pixels + (q * 4) + qh * 32;
				uint8 p00 = stream->ReadByte();
				uint8 p01 = stream->ReadByte();
				uint8 p02 = stream->ReadByte();
				uint8 p03 = stream->ReadByte();
				TwoBitsStreamAdapter bs(stream);
				Pattern4 op(p00, p01, p02, p03);
				for (int c = 0; c < 16; c++) // 4x4
					p[c + 4 * (c / 4)] = op.PixelValue(bs.ReadBits());
				// 4x4
			} // 4x8
		} // 8x8
	} else {
		stream->Seek(8, SEEK_CUR);
		uint8 p4 = stream->ReadByte();
		uint8 p5 = stream->ReadByte();
		stream->Seek(-10, SEEK_CUR);
		if (p4 <= p5) {
			{
				TwoBitsStreamAdapter bs(stream);
				for (int32 c = 0; c < 32; c++) {
					pixels[c + 4 * (c / 4)] = op0.PixelValue(bs.ReadBits());
				}
			}
			{
				uint8 p00 = stream->ReadByte();
				uint8 p01 = stream->ReadByte();
				uint8 p02 = stream->ReadByte();
				uint8 p03 = stream->ReadByte();
				Pattern4 op1(p00, p01, p02, p03);
				pixels += 4;
				TwoBitsStreamAdapter bs(stream);
				for (int32 c = 0; c < 32; c++)
					pixels[c + 4 * (c / 4)] = op1.PixelValue(bs.ReadBits());
			}
		} else {
			{
				TwoBitsStreamAdapter bs(stream);
				for (int32 x = 0; x < 32; x++)
					*pixels++ = op0.PixelValue(bs.ReadBits());
			}
			{
				uint8 p00 = stream->ReadByte();
				uint8 p01 = stream->ReadByte();
				uint8 p02 = stream->ReadByte();
				uint8 p03 = stream->ReadByte();
				Pattern4 op1(p00, p01, p02, p03);
				TwoBitsStreamAdapter bs(stream);
				for (int32 x = 0; x < 32; x++)
					*pixels++ = op1.PixelValue(bs.ReadBits());
			}
		}
	}
	GraphicsEngine::Get()->BlitBitmap(fScratchBuffer, NULL, fNewFrame, blitRect);
}


void
MovieDecoder::OpcodeB_8(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	for (int y = 0; y < 64; y++)
		*pixels++ = stream->ReadByte();
	GraphicsEngine::Get()->BlitBitmap(fScratchBuffer, NULL, fNewFrame, blitRect);
}


void
MovieDecoder::OpcodeC_8(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	for (int i = 0; i < 16; i++) {
		uint8 data = stream->ReadByte();
		fill_pixels(pixels, i, data, 2);
	}
	GraphicsEngine::Get()->BlitBitmap(fScratchBuffer, NULL, fNewFrame, blitRect);
}


void
MovieDecoder::OpcodeD_8(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	for (int i = 0; i < 4; i++) {
		uint8 data = stream->ReadByte();
		fill_pixels(pixels, i, data, 4);
	}
	GraphicsEngine::Get()->BlitBitmap(fScratchBuffer, NULL, fNewFrame, blitRect);
}


void
MovieDecoder::OpcodeE_8(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	uint8 pixel = stream->ReadByte();
	GraphicsEngine::FillRect(fNewFrame, blitRect, pixel);
#if DEBUG
	GraphicsEngine::Get()->BlitBitmap(fNewFrame, &fActiveRect, fScratchBuffer, NULL);
#endif
}


void
MovieDecoder::OpcodeF_8(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	uint8 p0 = stream->ReadByte();
	uint8 p1 = stream->ReadByte();
	for (int32 r = 0; r < 32; r++) {
		*pixels++ = p0;
		*pixels++ = p1;
	}
	GraphicsEngine::Get()->BlitBitmap(fScratchBuffer, NULL, fNewFrame, blitRect);
}



// BitStreamAdapter
inline
BitStreamAdapter::BitStreamAdapter(Stream* stream)
	:
	fStream(stream),
	fByte(0),
	fBitPos(7)
{
}


inline
uint8
BitStreamAdapter::ReadBit()
{
	if (fBitPos == 7)
		fByte = fStream->ReadByte();
	uint8 bit = (fByte >> fBitPos) & 1;
	if (--fBitPos < 0)
		fBitPos = 7;
	return bit;
}


// TwoBitsStreamAdapter
inline
TwoBitsStreamAdapter::TwoBitsStreamAdapter(Stream* stream)
	:
	fStream(stream),
	fByte(0),
	fBitPos(6)
{
}


inline uint8
TwoBitsStreamAdapter::ReadBits()
{
	if (fBitPos == 6)
		fByte = fStream->ReadByte();
	uint8 bits = (fByte >> fBitPos) & 0x3;
	fBitPos -= 2;
	if (fBitPos < 0)
		fBitPos = 6;
	return bits;
}
