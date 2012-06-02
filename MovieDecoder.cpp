#include "Bitmap.h"
#include "GraphicsEngine.h"
#include "MemoryStream.h"
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
fill_pixels(uint8 *pixels, uint16 offset, uint8 value, uint16 blockSize)
{
	uint8* ptr = pixels + offset * blockSize + ((offset * blockSize) / 8) * 8 ;
	//printf("offset: %d %d\n", ptr - pixels, ((offset * blockSize) / 8) * 8) ;
	for (int16 r = 0; r < blockSize; r++) {
		for (int16 c = 0; c < blockSize; c++) {
			//printf("pixel number %d\n", (ptr + c + (r * 8)) - pixels);
			ptr[c + r * 8] = value;
		}
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
}


MovieDecoder::~MovieDecoder()
{
	delete[] fDecodingMap;
	GraphicsEngine::DeleteBitmap(fNewFrame);
	GraphicsEngine::DeleteBitmap(fCurrentFrame);
	GraphicsEngine::DeleteBitmap(fScratchBuffer);
}


bool
MovieDecoder::AllocateBuffer(uint16 width, uint16 height)
{
	fNewFrame = GraphicsEngine::CreateBitmap(width, height, 8);
	fCurrentFrame = GraphicsEngine::CreateBitmap(width, height, 8);

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
					throw "opcode 0x6 unimplemented!";
					break;
				case 0x7:
				{
					uint8 p0 = stream->ReadByte();
					uint8 p1 = stream->ReadByte();
					if (p0 < p1) {
						BitStreamAdapter bs(stream);
						for (int row = 0; row < 8; row++) {
							for (int c = 0; c < 8; c++)
								*pixels++ = bs.ReadBit() ? p1 : p0;
						}
					} else {
						BitStreamAdapter bs(stream);
						for (int i = 0; i < 16; i++)
							fill_pixels(pixels, i, bs.ReadBit() ? p0 : p1, 2);
					}
					gfx->BlitBitmap(fScratchBuffer, NULL, fNewFrame, &fActiveRect);
					break;
				}
				case 0x8:
				{
					//break;
					// TODO: Implement!
					uint8 p0 = stream->ReadByte();
					uint8 p1 = stream->ReadByte();
					if (p0 < p1) {
						stream->Seek(-2, SEEK_CUR);
						for (int qh = 0; qh < 2; qh++) { // 8x8
							for (int q = 0; q < 2; q++) { // 4x8
								uint8 *p = pixels + (q * 4) * 8 + qh * 4;
								uint8 p0 = stream->ReadByte();
								uint8 p1 = stream->ReadByte();
								for (int c = 0; c < 2; c++) { // 4x4
									uint8 index0 = stream->ReadByte();
									for (int bit = 7; bit >= 0; bit--) { // 4x2
										uint8 value = (index0 & (1 << bit)) ? p1 : p0;
										p[(7 - bit) % 4] = value;
									} // 4x2
									p += 4;
								} // 4x4
							} // 4x8
						} // 8x8
					} else {
						stream->Seek(4, SEEK_CUR);
						uint8 p2 = stream->ReadByte();
						uint8 p3 = stream->ReadByte();
						stream->Seek(-4, SEEK_CUR);
						if (p2 <= p3) { // LEFT-RIGHT
							// TODO: Implement

						} else { // TOP-BOTTOM
							uint8 *p = pixels;
							{
								BitStreamAdapter bs(stream);
								for (int32 x = 0; x < 32; x++)
									*p++ = bs.ReadBit() ? p1 : p0;
								stream->Seek(2, SEEK_CUR);
							}
							{
								BitStreamAdapter bs(stream);
								for (int32 x = 0; x < 32; x++)
									*p++ = bs.ReadBit() ? p3 : p2;
							}
						}
						//stream->Seek(10, SEEK_CUR);
					}
					break;
				}
				case 0x9:
				{
					break;
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
					break;
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
			if (0) {
				DumpData((uint8*)fScratchBuffer->Pixels(), 64);
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
		fScratchBuffer->SetColors(colors, start, count);
		*/
	GraphicsEngine::Get()->BlitToScreen(fNewFrame, NULL, NULL);
	GraphicsEngine::Get()->Flip();

	std::swap(fNewFrame, fCurrentFrame);
}


Bitmap*
MovieDecoder::CurrentFrame()
{
	return fNewFrame;
}


void
MovieDecoder::Test()
{
	AllocateBuffer(640, 480);

	uint8 *decodingMap = new uint8[1];
	decodingMap[0] = (0x7 << 4) | 0x7;
	SetDecodingMap(decodingMap, 1);

	// Test opcode 0x7:
	const uint8 data[] = {
			0x11, 0x22, 0xff, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xff, // opcode7, first case
			0x22, 0x11, 0xff, 0x81 }; // opcode7, second case
	MemoryStream stream(data, sizeof(data));
	DecodeDataBlock(&stream, stream.Size());

	const uint8 resultData[] = { // opcode 7, first case
			0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
			0x22, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x22,
			0x22, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x22,
			0x22, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x22,
			0x22, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x22,
			0x22, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x22,
			0x22, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x22,
			0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22
	};

	const uint8 resultData1[] = { // opcode 7, second case
			0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
			0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
			0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
			0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
			0x22, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
			0x22, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
			0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x22,
			0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x22
	};

	int result = memcmp(&resultData, fScratchBuffer->Pixels(), sizeof(resultData));
	printf("opcode 0x%x: %s\n", decodingMap[0] & 0xF, result ? "FAILURE" : "OK");

	result = memcmp(&resultData1, fScratchBuffer->Pixels(), sizeof(resultData1));
	printf("opcode 0x%x: %s\n", decodingMap[0] >> 4, result ? "FAILURE" : "OK");


}


// BitStreamAdapter
BitStreamAdapter::BitStreamAdapter(Stream* stream)
	:
	fStream(stream),
	fByte(0),
	fBitPos(7)
{
}


uint8
BitStreamAdapter::ReadBit()
{
	//printf("bitpos: %d\n", fBitPos);
	if (fBitPos == 7) {
		fByte = fStream->ReadByte();
		//printf("byte: 0x%x\n", fByte);
	}
	uint8 bit = fByte & (1 << fBitPos);
	if (--fBitPos < 0)
		fBitPos = 7;
	return bit;
}
