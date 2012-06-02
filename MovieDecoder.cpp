#include "Bitmap.h"
#include "GraphicsEngine.h"
#include "MovieDecoder.h"
#include "Stream.h"
#include "SDL.h"

#include <assert.h>
#include <iostream>


// TODO: works for 8x8x8 bitmaps only
static inline uint8*
pixelat(uint8 *pixels, int x, int y)
{
	return pixels + (y * 8) + x;
}


// TODO: works for 8x8x8 bitmaps only
static inline void
fill_pixels(uint8 *pixels, uint16 offset, int16 value, uint16 blockSize)
{
	uint8* ptr = pixels + offset * blockSize;
	for (int16 r = 0; r < blockSize; r++) {
		for (int16 c = 0; c < blockSize; c++) {
			ptr[c + r * 8] = value;
		}
	}
}


class Opcode9 {
public:
	Opcode9(const uint8& p0, const uint8& p1, const uint8& p2, const uint8& p3)
		:
		fP0(p0),
		fP1(p1),
		fP2(p2),
		fP3(p3)
	{
	}
	inline uint8 PixelValue(const uint8 &bits)
	{
		switch (bits) {
			default:
			case 0: return fP0;
			case 1: return fP1;
			case 2: return fP2;
			case 3: return fP3;
		}
	}
private:
	const uint8& fP0;
	const uint8& fP1;
	const uint8& fP2;
	const uint8& fP3;
};


class Opcode2 {
public:
	Opcode2(const GFX::rect& rect, uint8 byte)
		:
		fRect(rect),
		fByte(byte)
	{
	}
	inline GFX::rect Rect(bool opcode3) const
	{
		GFX::rect rect = fRect;
		int16 xValue;
		int16 yValue;
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
		return rect;
	}
private:
	const GFX::rect& fRect;
	const uint8& fByte;
};

class BitStreamAdapter : public Stream {
public:
	BitStreamAdapter(Stream* stream);
	uint8 ReadBit();

private:
	Stream* fStream;
	uint8 fByte;
	uint8 fBitPos;
};


MovieDecoder::MovieDecoder()
	:
	fDecodingMap(NULL),
	fMapSize(0),
	fNewFrame(NULL),
	fCurrentFrame(NULL),
	fPreviousFrame(NULL)
{
	fActiveRect.x = fActiveRect.y = 0;
	fActiveRect.w = fActiveRect.h = 8;
}


MovieDecoder::~MovieDecoder()
{
	delete[] fDecodingMap;
	GraphicsEngine::DeleteBitmap(fNewFrame);
	GraphicsEngine::DeleteBitmap(fCurrentFrame);
	GraphicsEngine::DeleteBitmap(fPreviousFrame);
	GraphicsEngine::DeleteBitmap(fScratchBuffer);
}


bool
MovieDecoder::AllocateBuffer(uint16 width, uint16 height)
{
	fNewFrame = GraphicsEngine::CreateBitmap(width, height, 8);
	fCurrentFrame = GraphicsEngine::CreateBitmap(width, height, 8);
	fPreviousFrame = GraphicsEngine::CreateBitmap(width, height, 8);
	fScratchBuffer = GraphicsEngine::CreateBitmap(8, 8, 8);

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
	for (int32 i = 0; i < count; i++) {
		fColors[i].r = *palette++ << 2;
		fColors[i].g = *palette++ << 2;
		fColors[i].b = *palette++ << 2;
		fColors[i].a = 0;
	}

	fNewFrame->SetColors(fColors, 0, 256);
	fCurrentFrame->SetColors(fColors, start, count);
	fPreviousFrame->SetColors(fColors, start, count);
	fScratchBuffer->SetColors(fColors, start, count);
}


void
MovieDecoder::DecodeDataBlock(Stream *stream, uint32 length)
{
	fActiveRect.x = fActiveRect.y = 0;

	GraphicsEngine *gfx = GraphicsEngine::Get();
	uint32 pos = stream->Position();
	for (uint32 i = 0; i < fMapSize; i++) {
		for (int b = 0; b < 2; b++) {
			uint8 opcode;
			if (b == 0)
				opcode = fDecodingMap[i] & 0x0F;
			else
				opcode = (fDecodingMap[i] & 0xF0) >> 4;

			//std::cout << "opcode " << (int)opcode << std::endl;

			uint8 *pixels = (uint8 *)fScratchBuffer->Pixels();
			switch (opcode) {
				case 0x0:
					gfx->BlitBitmap(fCurrentFrame, &fActiveRect,
							fNewFrame, &fActiveRect);
					break;

				case 0x1:
					break;
				case 0x2:
				{
					uint8 byte = stream->ReadByte();
					GFX::rect rect = Opcode2(fActiveRect, byte).Rect(false);
					gfx->BlitBitmap(fNewFrame, &rect, fNewFrame, &fActiveRect);
					break;
				}
				case 0x3:
				{
					// Same as above but negated
					uint8 byte = stream->ReadByte();
					GFX::rect rect = Opcode2(fActiveRect, byte).Rect(true);
					gfx->BlitBitmap(fNewFrame, &rect, fNewFrame, &fActiveRect);
					break;
				}
				case 0x4:
				{
					uint8 byte = stream->ReadByte();
					GFX::rect rect = fActiveRect;
					rect.x += (byte & 0x0F) - 8;
					rect.y += ((byte & 0xF0) >> 4) - 8;
					gfx->BlitBitmap(fCurrentFrame, &rect, fNewFrame, &fActiveRect);
					break;
				}
				case 0x5:
				{
					GFX::rect rect = fActiveRect;
					rect.x += (stream->ReadByte() & 0x0F) - 8;
					rect.y += ((stream->ReadByte() & 0xF0) >> 4) - 8;
					gfx->BlitBitmap(fCurrentFrame, &rect, fNewFrame, &fActiveRect);
					break;
				}
				case 0x6:
					break;
				case 0x7:
				{
					uint8 p0 = stream->ReadByte();
					uint8 p1 = stream->ReadByte();
					if (p0 < p1) {
						uint8 rowPixels = stream->ReadByte();
						for (int row = 0; row < 8; row++) {
							for (int bit = 7; bit >= 0; bit--)
								*pixels++ = rowPixels & (1 << bit) ? p1 : p0;
						}
					} else {
						for (int i = 0; i < 2; i++) {
							uint8 value = stream->ReadByte();
							for (int bit = 7; bit >= 0; bit--) {
								uint8 pixelValue = value & (1 << bit) ? p1 : p0;
								fill_pixels(pixels, (7 - bit) * i + (7 - bit), pixelValue, 2);
							}
						}
					}
					gfx->BlitBitmap(fScratchBuffer, NULL, fNewFrame, &fActiveRect);
					break;
				}
				case 0x8:
				{
					// TODO: Implement!
					uint8 p0 = stream->ReadByte();
					uint8 p1 = stream->ReadByte();
					if (p0 < p1) {
						uint8 *p = pixels;
						for (int c = 0; c < 2; c++) {
							uint8 index0 = stream->ReadByte();
							for (int bit = 7; bit >= 0; bit--) {
								uint8 value = (index0 & (1 << bit)) ? p1 : p0;
								p[(7 - bit) % 4] = value;
								p +=4;
							}
						}
						stream->Seek(12, SEEK_CUR);
												/*
						index0 = stream->ReadByte();
						for (int bit = 7; bit >= 0; bit--) {
							uint8 value = (index0 & (1 << bit)) ? p1 : p0;
							pixels[32 + ((7 - bit) % 4)] = value;
						}
						index0 = stream->ReadByte();
						for (int bit = 7; bit >= 0; bit--) {
							uint8 value = (index0 & (1 << bit)) ? p1 : p0;
							pixels[(7 - bit) % 4] = value;
						}
						index0 = stream->ReadByte();
						for (int bit = 7; bit >= 0; bit--) {
							uint8 value = (index0 & (1 << bit)) ? p1 : p0;
							pixels[32 + ((7 - bit) % 4)] = value;
						}*/
					} else {
						stream->Seek(10, SEEK_CUR);
					}
					break;
				}
				case 0x9:
				{
					uint8 p0 = stream->ReadByte();
					uint8 p1 = stream->ReadByte();
					uint8 p2 = stream->ReadByte();
					uint8 p3 = stream->ReadByte();
					Opcode9 op(p0, p1, p2, p3);
					if (p0 <= p1 && p2 <= p3) {
						for (int i = 0; i < 16; i++) {
							uint8 byte = stream->ReadByte();
							for (int c = 6; c >= 0; c -= 2)
								*pixels++ = op.PixelValue(byte & (3 << c));
						}
					} else if (p0 <= p1 && p2 > p3) {
						for (int i = 0; i < 4; i++) {
							uint8 byte = stream->ReadByte();
							for (int c = 6; c >= 0; c -= 2) {
								uint8 pValue = op.PixelValue(byte & (3 << c));
								fill_pixels(pixels, (6 - c) * i + (6 - c), pValue, 2);
							}
						}
					} else if (p0 > p1 && p2 <= p3 ) {
						for (int i = 0; i < 8; i++) {
							uint8 byte = stream->ReadByte();
							for (int c = 6; c >= 0; c -= 2) {
								uint8 value = op.PixelValue(byte & (3 << c));
								*pixels++ = value;
								*pixels++ = value;
							}
						}
					} else {
						for (int i = 0; i < 8; i++) {
							uint8 byte = stream->ReadByte();
							for (int c = 6; c >= 0; c -= 2) {
								uint8 value = op.PixelValue(byte & (3 << c));
								pixels[(6 - c) * 8 + (6 - c)] = value;
								pixels[(6 - c) * 8 + (6 - c) + 8] = value;
							}
						}
					}
					gfx->BlitBitmap(fScratchBuffer, NULL, fNewFrame, &fActiveRect);
					break;
				}
				case 0xa:
				{
					// TODO: Implement!
					uint8 p0 = stream->ReadByte();
					uint8 p1 = stream->ReadByte();
					uint8 p2 = stream->ReadByte();
					uint8 p3 = stream->ReadByte();
					if (p0 <= p1) {
						stream->Seek(28, SEEK_CUR);
					} else {
						stream->Seek(20, SEEK_CUR);
					}
					break;
				}
				case 0xB:
				{
					for (int y = 0; y < 64; y++) {
						uint8 data = stream->ReadByte();
						*pixels++ = data;
					}
					gfx->BlitBitmap(fScratchBuffer, NULL, fNewFrame, &fActiveRect);
					break;
				}
				case 0xC:
				{
					for (int i = 0; i < 16; i++) {
						uint8 data = stream->ReadByte();
						fill_pixels(pixels, i, data, 2);
					}
					gfx->BlitBitmap(fScratchBuffer, NULL, fNewFrame, &fActiveRect);
					break;
				}
				case 0xD:
				{
					for (int i = 0; i < 4; i++) {
						uint8 data = stream->ReadByte();
						fill_pixels(pixels, i, data, 4);
					}
					gfx->BlitBitmap(fScratchBuffer, NULL, fNewFrame, &fActiveRect);
					break;
				}
				case 0xE:
				{
					uint8 pixel = stream->Read(pixel);
					SDL_FillRect(fNewFrame->Surface(), (SDL_Rect*)&fActiveRect, pixel);
					break;
				}
				case 0xF:
				{
					uint8 p0 = stream->ReadByte();
					uint8 p1 = stream->ReadByte();
					for (int32 i = 0; i < 64; i++)
						*pixels++ = i % 2 ? p0 : p1;
					gfx->BlitBitmap(fScratchBuffer, NULL, fNewFrame, &fActiveRect);
					break;
				}
				default:
					break;
			}
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
	fNewFrame->SetColors(fColors, 0, 256);
	/*	fCurrentFrame->SetColors(colors, start, count);
		fPreviousFrame->SetColors(colors, start, count);
		fScratchBuffer->SetColors(colors, start, count);
		*/
	GraphicsEngine::Get()->BlitToScreen(fNewFrame, NULL, NULL);
	GraphicsEngine::Get()->Flip();

	// Previous become the base for the new
	// Current become previous
	Bitmap* temp = fPreviousFrame;
	fPreviousFrame = fCurrentFrame;
	fCurrentFrame = fNewFrame;
	fNewFrame = temp;
	//std::swap(fNewFrame, fPreviousFrame);
	//std::swap(fPreviousFrame, fCurrentFrame);
}


Bitmap*
MovieDecoder::CurrentFrame()
{
	return fNewFrame;
}


// BitStreamAdapter
BitStreamAdapter::BitStreamAdapter(Stream* stream)
	:
	fStream(stream),
	fByte(0),
	fBitPos(0)
{
}


uint8
BitStreamAdapter::ReadBit()
{
	if (fBitPos == 0)
		fByte = fStream->ReadByte();
	uint8 bit = fByte & (1 << fBitPos);
	if (++fBitPos == 8)
		fBitPos = 0;
	return bit;
}
