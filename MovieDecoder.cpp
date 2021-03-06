/* Documentation for the format taken from:
	http://wiki.multimedia.cx/index.php?title=Interplay_Video
*/

#include "Bitmap.h"
#include "GraphicsEngine.h"
#include "MemoryStream.h"
#include "MovieDecoder.h"
#include "Stream.h"

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


class Pattern4 {
public:
	Pattern4(const uint8& p0, const uint8& p1, const uint8& p2, const uint8& p3)
	{
		fP[0] = p0;
		fP[1] = p1;
		fP[2] = p2;
		fP[3] = p3;
	}
	inline uint8 PixelValue(const uint8 &bits) const
	{
		return fP[bits];
	}
private:
	uint8 fP[4];
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
	fCurrentFrame(NULL),
	fVersion(0)
{
	fActiveRect.x = fActiveRect.y = 0;
	fActiveRect.w = fActiveRect.h = 8;
	fScratchBuffer = new Bitmap(8, 8, 8);

	memset(sOpcodes, 0, sizeof(sOpcodes));

	sOpcodes[0] = &MovieDecoder::Opcode0;
	sOpcodes[1] = &MovieDecoder::Opcode1;
	sOpcodes[2] = &MovieDecoder::Opcode2;
	sOpcodes[3] = &MovieDecoder::Opcode3;
	sOpcodes[4] = &MovieDecoder::Opcode4;
	sOpcodes[5] = &MovieDecoder::Opcode5;
	sOpcodes[6] = &MovieDecoder::Opcode6;
	sOpcodes[7] = &MovieDecoder::Opcode7;
	sOpcodes[8] = &MovieDecoder::Opcode8;
	sOpcodes[9] = &MovieDecoder::Opcode9;
	sOpcodes[10] = &MovieDecoder::OpcodeA;
	sOpcodes[11] = &MovieDecoder::OpcodeB;
	sOpcodes[12] = &MovieDecoder::OpcodeC;
	sOpcodes[13] = &MovieDecoder::OpcodeD;
	sOpcodes[14] = &MovieDecoder::OpcodeE;
	sOpcodes[15] = &MovieDecoder::OpcodeF;
}


MovieDecoder::~MovieDecoder()
{
	delete[] fDecodingMap;
	if (fNewFrame != NULL)
		fNewFrame->Release();
	if (fCurrentFrame != NULL)
		fCurrentFrame->Release();
	if (fScratchBuffer != NULL)
		fScratchBuffer->Release();
}


bool
MovieDecoder::AllocateBuffer(uint16 width, uint16 height, uint16 version, bool trueColor)
{
	std::cout << "MovieDecoder::AllocateBuffer(width: ";
	std::cout << std::dec << width << ", height: " << height;
	std::cout << ", version: " << version << ", truecolor: ";
	std::cout << (trueColor ? "yes" : "no") << ")" << std::endl;

	fNewFrame = new Bitmap(width, height, 8);
	fCurrentFrame = new Bitmap(width, height, 8);

	fVersion = version;

	fMapSize = ((width * height) / (8 * 8)) / 2;
	return true;
}


bool
MovieDecoder::InitVideoMode(uint16 width, uint16 height, uint16 flags)
{
	GraphicsEngine::Get()->SetVideoMode(width, height, 16, 0);

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
	// TODO: WHY ??? FIND OUT
	if (fVersion == 99) {
		// We use version == 99
		// for tests
		dataOffset = 0;
	}
	//if (fVersion == 2)
		//dataOffset = 16;

	stream->Seek(dataOffset, SEEK_CUR);

	const uint32 loopMax = fMapSize * 2;
	for (uint32 i = 0; i < loopMax; i++) {
		const uint32 index = i >> 1;
		uint8 opcode = (i & 1) ?
				((fDecodingMap[index] >> 4) & 0x0F) :
				(fDecodingMap[index] & 0x0F);

		GFX::rect blitRect = fActiveRect;

		if (sOpcodes[opcode]) {
			CALL_MEMBER_FUNCTION(*this, sOpcodes[opcode])(stream,
				(uint8*)fScratchBuffer->Pixels(), &blitRect);
		}

		fActiveRect.x += 8;
		if (fActiveRect.x >= fNewFrame->Width()) {
			fActiveRect.x = 0;
			fActiveRect.y += 8;
		}
	}

	stream->Seek(pos + length, SEEK_SET);
}


void
MovieDecoder::BlitBackBuffer()
{
	GraphicsEngine::Get()->BlitToScreen(fNewFrame, NULL, NULL);
	GraphicsEngine::Get()->Update();

	std::swap(fNewFrame, fCurrentFrame);
}


// opcodes
void
MovieDecoder::Opcode0(Stream* stream, uint8* pixels, GFX::rect* rect)
{
	// Block is copied from corresponding block from current frame.
	// (i.e. this block is unchanged). 
	GraphicsEngine::Get()->BlitBitmap(fCurrentFrame, rect, fNewFrame, rect);
}


void
MovieDecoder::Opcode1(Stream* stream, uint8* pixels, GFX::rect* rect)
{
	// Block is unmodified. This appears to mean that it has the same value it had 2 frames ago,
	// but the net effect is that nothing is done to this block of 8x8 pixels. 
}


void
MovieDecoder::Opcode2(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	// Block is copied from nearby (below and to the right) within the new frame.
	// The offset within the buffer from which to grab the patch of 8 pixels is
	// given by grabbing a byte B from the data stream, which is broken into a
	// positive x and y offset according to the following mapping:
	/*  if B < 56:
			x = 8 + (B % 7)
			y = B / 7
		else
			x = -14 + ((B - 56) % 29)
			y =   8 + ((B - 56) / 29)
	*/
	// (where % is the 'modulo' operator) 
	
	GFX::rect rect = fActiveRect;
	uint8 byte = stream->ReadByte();
	if (byte < 56) {
		rect.x += 8 + (byte % 7);
		rect.y += byte / 7;
	} else {
		rect.x += -14 + ((byte - 56) % 29);
		rect.y +=  8 + ((byte - 56) / 29);
	}
	GraphicsEngine::Get()->BlitBitmap(fNewFrame, &rect, fNewFrame, blitRect);
}


void
MovieDecoder::Opcode3(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	// Same as above but negated
	uint8 byte = stream->ReadByte();
	GFX::rect rect = fActiveRect;
	if (byte < 56) {
		rect.x += - (8 + (byte % 7));
		rect.y += - (byte / 7);
	} else {
		rect.x += - (-14 + ((byte - 56) % 29));
		rect.y += - (8 + ((byte - 56) / 29));
	}
	GraphicsEngine::Get()->BlitBitmap(fNewFrame, &rect, fNewFrame, blitRect);
}


void
MovieDecoder::Opcode4(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	// Similar to 0x2 and 0x3, except this method copies from the "current"
	// frame, rather than the "new" frame, and instead of the lopsided mapping
	// they use, this one uses one which is symmetric and centered around the
	// top-left corner of the block. This uses only 1 byte still, though, so the
	// range is decreased, since we have to encode all directions in a single
	// byte. The byte we pull from the data stream, I'll call B. Call the
	// highest 4 bits of B BH and the lowest 4 bytes BL. Then the offset from
	// which to copy the data is:
	/*
		x = -8 + BL
		y = -8 + BH
	*/

	uint8 byte = stream->ReadByte();
	GFX::rect rect = fActiveRect;
	rect.x += -8 + (byte & 0x0F);
	rect.y += -8 + ((byte >> 4) & 0x0F);
	GraphicsEngine::Get()->BlitBitmap(fCurrentFrame, &rect, fNewFrame, blitRect);
}


void
MovieDecoder::Opcode5(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	// Similar to 0x4, but instead of one byte for the offset, this uses two
	// bytes to encode a larger range, the first being the x offset as a signed
	// 8-bit value, and the second being the y offset as a signed 8-bit value.
	
	GFX::rect rect = fActiveRect;
	rect.x += (sint8)stream->ReadByte();
	rect.y += (sint8)stream->ReadByte();
	GraphicsEngine::Get()->BlitBitmap(fCurrentFrame, &rect, fNewFrame, blitRect);
}


void
MovieDecoder::Opcode6(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	// Not used
}


void
MovieDecoder::Opcode7(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	// Most of the following encodings are "patterned" blocks, where we are
	// given a number of pixel values and then bitmapped values to specify which
	// pixel values belong to which squares. For this encoding, we are given the
	// following in the data stream:
	/*
		P0 P1

		These are pixel values (i.e. 8-bit indices into the palette).
		If P0 <= P1, we then get 8 more bytes from the data stream, one for each
		row in the block:

		B0 B1 B2 B3 B4 B5 B6 B7

		For each row, the rightmost pixel is represented by the low-order bit,
		and the leftmost by the high-order bit. Use your imagination in between.
		If a bit is set, the pixel value is P1 and if it is unset, the pixel
		value is P0.

		If, on the other hand, P0 > P1, we get two more bytes from the data
		stream:

		B0 B1

		Each of these bytes contains a 4-bit pattern. This pattern works exactly
		like the pattern above with 8 bytes, except each bit represents a 2x2
		pixel region.
	*/
	uint8 p0 = stream->ReadByte();
	uint8 p1 = stream->ReadByte();
	BitStreamAdapter bs(stream);
	if (p0 < p1) {
		for (int32 c = 0; c < 64; c++)
			*pixels++ = bs.ReadBit() ? p1 : p0;
	} else {
		for (int32 r = 0; r < 8; r+=2) {
			for (int32 c = 0; c < 8; c+=2) {
				pixels[c] = pixels[c + 1] = pixels[c + 8]
				          = pixels[c + 9] = bs.ReadBit() ? p1 : p0;
			}
			pixels += 16;
		}
	}
	GraphicsEngine::Get()->BlitBitmap(fScratchBuffer, NULL, fNewFrame, blitRect);
}


void
MovieDecoder::Opcode8(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	// Ok, this one is basically like encoding 0x7, only more complicated.
	// Again, we start out by getting two bytes on the data stream:
	/*
		P0 P1
		if P0 <= P1 then we get the following from the data stream:

					  B0 B1
				P2 P3 B2 B3
				P4 P5 B4 B5
				P6 P7 B6 B7

		P0 P1 and B0 B1 are used for the top-left corner, P2 P3 B2 B3 for the
		bottom-left corner, P4 P5 B4 B5 for the top-right, P6 P7 B6 B7 for the
		bottom-right. (So, each codes for a 4x4 pixel array.) Since we have
		16 bits in B0 B1, there is one bit for each pixel in the array.
		The convention for the bit-mapping is, again, left to right and top to
		bottom.

		So, basically, the top-left quarter of the block is an arbitrary pattern
		with 2 pixels, the bottom-left a different arbitrary pattern with 2
		different pixels, and so on.

		if P0 > P1 then we get 10 more bytes from the data stream:

		B0 B1 B2 B3 P2 P3 B4 B5 B6 B7

		Now, if P2 <= P3, then [P0 P1 B0 B1 B2 B3] represent the left half of
		the block and [P2 P3 B4 B5 B6 B7] represent the right half.

		If P2 > P3, [P0 P1 B0 B1 B2 B3] represent the top half of the block and
		[P2 P3 B4 B5 B6 B7] represent the bottom half.
	*/
	uint8 t0 = stream->ReadByte();
	uint8 t1 = stream->ReadByte();
	uint16 flags = 0;
	if (t0 <= t1) {
		stream->Seek(-2, SEEK_CUR);
		for (int y = 0; y < 16; y++) {
			if (!(y & 3)) {
				t0 = stream->ReadByte();
				t1 = stream->ReadByte();
				stream->Read(flags);
			}
			for (int x = 0; x < 4; x++, flags >>= 1)
				*pixels++ = (flags & 1) ? t0 : t1;
			pixels += 8 - 4;
			if (y == 7) {
				// switch to right half
				pixels -= 8 * 8 - 4;
			}
		}
	} else {
		stream->Seek(4, SEEK_CUR);
		uint8 p2 = stream->ReadByte();
		uint8 p3 = stream->ReadByte();
		stream->Seek(-6, SEEK_CUR);
		if (p2 <= p3) { // LEFT-RIGHT
			{
				uint8* p = pixels;
				BitStreamAdapter bs(stream);
				for (int32 r = 0; r < 8; r++) {
					for (int32 c = 0; c < 4; c++) {
						*p++ = bs.ReadBit() ? t1 : t0;
					}
					p += 4;
				}
				stream->Seek(2, SEEK_CUR);
			}
			{
				uint8* p = pixels + 4;
				BitStreamAdapter bs(stream);
				for (int32 r = 0; r < 8; r++) {
					for (int32 c = 0; c < 4; c++) {
						*p++ = bs.ReadBit() ? p3 : p2;
					}
					p += 4;
				}
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
MovieDecoder::Opcode9(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	uint8 p0 = stream->ReadByte();
	uint8 p1 = stream->ReadByte();
	uint8 p2 = stream->ReadByte();
	uint8 p3 = stream->ReadByte();
	Pattern4 op(p0, p1, p2, p3);
	TwoBitsStreamAdapter bs(stream);
	if (p0 <= p1)
		if (p2 <= p3) {
			for (int i = 0; i < 64; i++)
				*pixels++ = op.PixelValue(bs.ReadBits());
		} else {
			for (int i = 0; i < 16; i++) {
				const uint8 value = op.PixelValue(bs.ReadBits());
				fill_pixels(pixels, i, value, 2);
			}
	} else {
		if (p2 <= p3 ) {
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
	}
	GraphicsEngine::Get()->BlitBitmap(fScratchBuffer, NULL, fNewFrame, blitRect);
}


void
MovieDecoder::OpcodeA(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	// Similar to the previous, only a little more complicated. We are still
	// dealing with patterns over 4 pixel values with 2 bits assigned to each
	// pixel (or block of pixels).

	/* So, first on the data stream are our 4 pixel values:

		P0 P1 P2 P3

		Now, if P0 <= P1, the block is divided into 4 quadrants, ordered
		(as with opcode 0x8) TL, BL, TR, BR.
		In this case the next data in the data stream should be:

                               B0  B1  B2  B3
               P4  P5  P6  P7  B4  B5  B6  B7
               P8  P9  P10 P11 B8  B9  B10 B11
               P12 P13 P14 P15 B12 B13 B14 B15

		Each 2 bits represent a 1x1 pixel (00=P0, 01=P1, 10=P2, 11=P3).
		The ordering is again left to right and top to bottom.
		The most significant bits represent the left side at the top, and so on.

		If P0 > P1 then the next data on the data stream is:

                           B0 B1 B2  B3  B4  B5  B6  B7
               P4 P5 P6 P7 B8 B9 B10 B11 B12 B13 B14 B15

		Now, in this case, if P4 <= P5, [P0 P1 P2 P3 B0 B1 B2 B3 B4 B5 B6 B7]
		represent the left half of the block and the other bytes represent the
		right half.
		If P4 > P5, then [P0 P1 P2 P3 B0 B1 B2 B3 B4 B5 B6 B7] represent the top
		half of the block and the other bytes represent the bottom half.
	*/
	uint8 p0 = stream->ReadByte();
	uint8 p1 = stream->ReadByte();
	uint8 p2 = 0, p3 = 0;
	if (p0 <= p1) {
		stream->Seek(-2, SEEK_CUR);
		uint32 flags = 0;
		for (int y = 0; y < 16; y++) {
			if (!(y & 3)) {
				p0 = stream->ReadByte();
				p1 = stream->ReadByte();
				p2 = stream->ReadByte();
				p3 = stream->ReadByte();
				stream->Read(flags);
			}
			Pattern4 pattern(p0, p1, p2, p3);
			for (int x = 0; x < 4; x++, flags >>= 2)
				*pixels++ = pattern.PixelValue(flags & 3);

			pixels += 4;
			if (y == 7)
				pixels -= 60;
		}
	} else {
		stream->Seek(10, SEEK_CUR);
		uint8 p4 = stream->ReadByte();
		uint8 p5 = stream->ReadByte();
		stream->Seek(-14, SEEK_CUR);
		uint64 flags = 0;
		for (int y = 0; y < 16; y++) {
			if (!(y & 7)) {
				p0 = stream->ReadByte();
				p1 = stream->ReadByte();
				p2 = stream->ReadByte();
				p3 = stream->ReadByte();
				stream->Read(flags);
			}
			Pattern4 pattern(p0, p1, p2, p3);
			for (int x = 0; x < 4; x++, flags >>= 2)
				*pixels++ = pattern.PixelValue(flags & 3);

			if (p4 <= p5) {
				pixels += 4;
				if (y == 7) {
					pixels -= 60;
				}
			}
		}
	}
	GraphicsEngine::Get()->BlitBitmap(fScratchBuffer, NULL, fNewFrame, blitRect);
}


void
MovieDecoder::OpcodeB(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	// In this encoding we get raw pixel data in the data stream -- 64 bytes of
	// pixel data. 1 byte for each pixel, and in the standard order (l->r, t->b)
	for (int y = 0; y < 64; y++)
		*pixels++ = stream->ReadByte();
	GraphicsEngine::Get()->BlitBitmap(fScratchBuffer, NULL, fNewFrame, blitRect);
}


void
MovieDecoder::OpcodeC(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	// In this encoding we get raw pixel data in the data stream -- 16 bytes of
	// pixel data. 1 byte for each block of 2x2 pixels, and in the standard
	// order (l->r, t->b).
	for (int32 r = 0; r < 8; r+=2) {
		for (int32 c = 0; c < 8; c+=2) {
			pixels[c] =
					pixels[c + 1] =
					pixels[c + 8] =
					pixels[c + 9] = stream->ReadByte();
		}
		pixels += 16;
	}
	GraphicsEngine::Get()->BlitBitmap(fScratchBuffer, NULL, fNewFrame, blitRect);
}


void
MovieDecoder::OpcodeD(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	// In this encoding we get raw pixel data in the data stream -- 4 bytes of
	// pixel data. 1 byte for each block of 4x4 pixels, and in the standard
	// order (l->r, t->b). 
	for (int i = 0; i < 4; i++) {
		uint8 data = stream->ReadByte();
		fill_pixels(pixels, i, data, 4);
	}
	GraphicsEngine::Get()->BlitBitmap(fScratchBuffer, NULL, fNewFrame, blitRect);
}


void
MovieDecoder::OpcodeE(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	// This encoding represents a solid frame.
	// We get 1 byte of pixel data from the data stream. 
	uint8 pixel = stream->ReadByte();
	GFX::rect destRect = *blitRect;
	fNewFrame->FillRect(destRect, pixel);
#if DEBUG
	GraphicsEngine::Get()->BlitBitmap(fNewFrame, blitRect, fScratchBuffer, NULL);
#endif
}


void
MovieDecoder::OpcodeF(Stream* stream, uint8* pixels, GFX::rect* blitRect)
{
	// This encoding represents a "dithered" frame, which is checkerboarded with
	// alternate pixels of two colors.
	// We get 2 bytes of pixel data from the data stream, and these bytes are
	// alternated
	uint8 p0 = stream->ReadByte();
	uint8 p1 = stream->ReadByte();
	uint8 pattern1[8] = { p0, p1, p0, p1, p0, p1, p0, p1 };
	uint8 pattern2[8] = { p1, p0, p1, p0, p1, p0, p1, p0 };
	for (int32 r = 0; r < 8; r++) {
		if ((r & 1) == 0)
			memcpy(pixels, pattern1, 8);
		else
			memcpy(pixels, pattern2, 8);
		pixels += 8;
	}
	GraphicsEngine::Get()->BlitBitmap(fScratchBuffer, NULL, fNewFrame, blitRect);
}



// BitStreamAdapter
inline
BitStreamAdapter::BitStreamAdapter(Stream* stream)
	:
	fStream(stream),
	fByte(0),
	fBitPos(0)
{
}


inline
uint8
BitStreamAdapter::ReadBit()
{
	if (fBitPos == 0)
		fByte = fStream->ReadByte();
	uint8 bit = (fByte >> fBitPos) & 1;
	if (++fBitPos > 7)
		fBitPos = 0;
	return bit;
}


// TwoBitsStreamAdapter
inline
TwoBitsStreamAdapter::TwoBitsStreamAdapter(Stream* stream)
	:
	fStream(stream),
	fByte(0),
	fBitPos(0)
{
}


inline uint8
TwoBitsStreamAdapter::ReadBits()
{
	if (fBitPos == 0)
		fByte = fStream->ReadByte();
	uint8 bits = (fByte >> fBitPos) & 0x3;
	fBitPos += 2;
	if (fBitPos > 7)
		fBitPos = 0;
	return bits;
}
