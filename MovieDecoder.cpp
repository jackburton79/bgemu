#include "Bitmap.h"
#include "GraphicsEngine.h"
#include "MemoryStream.h"
#include "MovieDecoder.h"
#include "Stream.h"
#include "SDL.h"

#include <assert.h>
#include <iostream>



const static int kOffsets[2][16] = {
		{0, 2, 4, 6, 16, 18, 20, 22, 32, 34, 36, 38, 48, 50, 52, 54 },
		{0, 4, 32, 36, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};


// TODO: works for 8x8x8 bitmaps only
static inline void
fill_pixels(uint8 *pixels, uint16 offset, uint8 value, uint16 blockSize)
{
	uint8* ptr = pixels + kOffsets[(blockSize / 2) - 1][offset];
//	printf("offset: %d %d %d\n", ptr - pixels, offset, kOffsets[blockSize - 1][offset]) ;
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


class Pattern4 {
public:
	Pattern4(const uint8& p0, const uint8& p1, const uint8& p2, const uint8& p3)
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

	fNewFrame->SetColors(fColors, start, count);
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
				opcode = fDecodingMap[i] >> 4;

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
					rect.y += (byte >> 4) - 8;
					gfx->BlitBitmap(fCurrentFrame, &rect, fNewFrame, &fActiveRect);
					break;
				}
				case 0x5:
				{
					GFX::rect rect = fActiveRect;
					rect.x += (sint8)stream->ReadByte();
					rect.y += (sint8)stream->ReadByte();
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
					BitStreamAdapter bs(stream);
					if (p0 < p1) {
						for (int c = 0; c < 64; c++)
							*pixels++ = bs.ReadBit() ? p1 : p0;
					} else {
						for (int i = 0; i < 16; i++)
							fill_pixels(pixels, i, bs.ReadBit() ? p0 : p1, 2);
					}
					gfx->BlitBitmap(fScratchBuffer, NULL, fNewFrame, &fActiveRect);
					break;
				}

				case 0x8:
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
								for (int c = 0; c < 16; c++) { // 4x4
									uint8 value = bs.ReadBit() ? p1 : p0;
									p[c + 4 * (c / 4)] = value;
								} // 4x4
							} // 4x8
						} // 8x8
					} else {
						stream->Seek(4, SEEK_CUR);
						uint8 p2 = stream->ReadByte();
						uint8 p3 = stream->ReadByte();
						stream->Seek(-6, SEEK_CUR);
						if (p2 <= p3) { // LEFT-RIGHT
							uint8 *p = pixels;
							{
								BitStreamAdapter bs(stream);
								for (int32 c = 0; c < 32; c++) {
									p[c + 4 * (c / 4)] = bs.ReadBit() ? t1 : t0;
								}
								stream->Seek(2, SEEK_CUR);
							}
							{
								p = pixels + 4;
								BitStreamAdapter bs(stream);
								for (int32 c = 0; c < 32; c++)
									p[c + 4 * (c / 4)] = bs.ReadBit() ? p3 : p2;
							}
						} else { // TOP-BOTTOM
							uint8 *p = pixels;
							{
								BitStreamAdapter bs(stream);
								for (int32 x = 0; x < 32; x++)
									*p++ = bs.ReadBit() ? t1 : t0;
								stream->Seek(2, SEEK_CUR);
							}
							{
								BitStreamAdapter bs(stream);
								for (int32 x = 0; x < 32; x++)
									*p++ = bs.ReadBit() ? p3 : p2;
							}
						}
					}
					break;
				}
				case 0x9:
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
							uint8 value = op.PixelValue(bs.ReadBits());
							fill_pixels(pixels, i, value, 2);
						}
					} else if (p0 > p1 && p2 <= p3 ) {
						for (int i = 0; i < 32; i++) {
							uint8 value = op.PixelValue(bs.ReadBits());
							*pixels++ = value;
							*pixels++ = value;
						}
					} else {
						for (int i = 0; i < 32; i++) {
							uint8 value = op.PixelValue(bs.ReadBits());
							uint8 ind = i + (i / 8) * 8;
							pixels[ind] = value;
							pixels[ind + 8] = value;
						}
					}
					gfx->BlitBitmap(fScratchBuffer, NULL, fNewFrame, &fActiveRect);
					break;
				}
				case 0xa:
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
								uint8 p0 = stream->ReadByte();
								uint8 p1 = stream->ReadByte();
								uint8 p2 = stream->ReadByte();
								uint8 p3 = stream->ReadByte();
								TwoBitsStreamAdapter bs(stream);
								Pattern4 op(p0, p1, p2, p3);
								for (int c = 0; c < 16; c++) { // 4x4
									p[c + 4 * (c / 4)] = op.PixelValue(bs.ReadBits());
								} // 4x4
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
								uint8* p = pixels + 4;
								TwoBitsStreamAdapter bs(stream);
								for (int32 c = 0; c < 32; c++)
									p[c + 4 * (c / 4)] = op1.PixelValue(bs.ReadBits());
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
					gfx->BlitBitmap(fScratchBuffer, NULL, fNewFrame, &fActiveRect);
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
#if DEBUG
					gfx->BlitBitmap(fNewFrame, &fActiveRect, fScratchBuffer, NULL);
#endif
					break;
				}
				case 0xF:
				{
					uint8 p0 = stream->ReadByte();
					uint8 p1 = stream->ReadByte();
					for (int32 i = 0; i < 64; i++)
						*pixels++ = i % 2 ? p1 : p0;
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
	//fNewFrame->SetColors(fColors, 0, 256);
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
#if DEBUG
	TestOpcode7A();
	TestOpcode7B();
	TestOpcode8A();
	TestOpcode8B();
	TestOpcode8C();
	TestOpcode9A();
	//TestOpcode9B();
	TestOpcodeB();
	TestOpcodeC();
	TestOpcodeD();
	TestOpcodeE();
	//throw -1;
#endif
}


#if DEBUG
void
MovieDecoder::TestInit(uint8 opcode, const uint8 data[], uint32 size)
{
	AllocateBuffer(640, 480);

	uint8 *decodingMap = new uint8[1];
	decodingMap[0] = opcode;
	SetDecodingMap(decodingMap, 1);

	MemoryStream stream(data, size);
	DecodeDataBlock(&stream, stream.Size());
}


void
MovieDecoder::TestFinish(const uint8 data[], uint32 dataSize)
{
	int result = memcmp(data, fScratchBuffer->Pixels(), dataSize);
	printf("opcode 0x%x: %s\n", fDecodingMap[0] & 0xF, result ? "FAILURE" : "OK");


	if (result) {
		DumpData((uint8*)fScratchBuffer->Pixels(), 64);
		throw "error";
	}

	GraphicsEngine::DeleteBitmap(fNewFrame);
	GraphicsEngine::DeleteBitmap(fCurrentFrame);
	fNewFrame = NULL;
	fCurrentFrame = NULL;
}



void
MovieDecoder::TestOpcode7A()
{
	// Test opcode 0x7:
	const uint8 data[] = {
			0x11, 0x22, 0xff, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xff }; // opcode7, first case

	TestInit(0x7, data, sizeof(data));

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

	TestFinish(resultData, sizeof(resultData));
}


void
MovieDecoder::TestOpcode7B()
{
	// Test opcode 0x7:
	const uint8 data[] = { 	0x22, 0x11, 0xff, 0x81 }; // opcode7, second case

	TestInit(0x7, data, sizeof(data));

	const uint8 resultData[] = { // opcode 7, second case
			0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
			0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
			0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
			0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
			0x22, 0x22, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
			0x22, 0x22, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
			0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x22, 0x22,
			0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x22, 0x22
	};

	TestFinish(resultData, sizeof(resultData));
}


void
MovieDecoder::TestOpcode8A()
{
	const uint8 data[] = {
			0x00, 0x22, 0xf9, 0x9f, 0x11, 0x33, 0xcc, 0x33,
			0x44, 0x55, 0xaa, 0x55, 0x66, 0x77, 0x01, 0xef
	};

	TestInit(0x8, data, sizeof(data));

	const uint8 result[] = {
			0x22, 0x22, 0x22, 0x22, 0x33, 0x33, 0x11, 0x11,
            0x22, 0x00, 0x00, 0x22, 0x33, 0x33, 0x11, 0x11,
            0x22, 0x00, 0x00, 0x22, 0x11, 0x11, 0x33, 0x33,
            0x22, 0x22, 0x22, 0x22, 0x11, 0x11, 0x33, 0x33,
            0x55, 0x44, 0x55, 0x44, 0x66, 0x66, 0x66, 0x66,
            0x55, 0x44, 0x55, 0x44, 0x66, 0x66, 0x66, 0x77,
            0x44, 0x55, 0x44, 0x55, 0x77, 0x77, 0x77, 0x66,
            0x44, 0x55, 0x44, 0x55, 0x77, 0x77, 0x77, 0x77
	};

	TestFinish(result, sizeof(result));
}


void
MovieDecoder::TestOpcode8B()
{
	const uint8 data[] = {
		0x22, 0x00, 0x01, 0x37, 0xf7, 0x31, 0x11, 0x66, 0x8c, 0xe6, 0x73, 0x31
	};

	TestInit(0x8, data, sizeof(data));

	const uint8 result[] = {
		0x22, 0x22, 0x22, 0x22, 0x66, 0x11, 0x11, 0x11,
		0x22, 0x22, 0x22, 0x00, 0x66, 0x66, 0x11, 0x11,
		0x22, 0x22, 0x00, 0x00, 0x66, 0x66, 0x66, 0x11,
		0x22, 0x00, 0x00, 0x00, 0x11, 0x66, 0x66, 0x11,
		0x00, 0x00, 0x00, 0x00, 0x11, 0x66, 0x66, 0x66,
		0x22, 0x00, 0x00, 0x00, 0x11, 0x11, 0x66, 0x66,
		0x22, 0x22, 0x00, 0x00, 0x11, 0x11, 0x66, 0x66,
		0x22, 0x22, 0x22, 0x00, 0x11, 0x11, 0x11, 0x66
	};

	TestFinish(result, sizeof(result));
}


void
MovieDecoder::TestOpcode8C()
{
	const uint8 data[] = {
			0x22, 0x00, 0xcc, 0x66, 0x33, 0x19, 0x66, 0x11, 0x18, 0x24, 0x42, 0x81
	};

	TestInit(0x8, data, sizeof(data));

	const uint8 result[] = {
		0x00, 0x00, 0x22, 0x22, 0x00, 0x00, 0x22, 0x22,
		0x22, 0x00, 0x00, 0x22, 0x22, 0x00, 0x00, 0x22,
		0x22, 0x22, 0x00, 0x00, 0x22, 0x22, 0x00, 0x00,
		0x22, 0x22, 0x22, 0x00, 0x00, 0x22, 0x22, 0x00,
		0x66, 0x66, 0x66, 0x11, 0x11, 0x66, 0x66, 0x66,
		0x66, 0x66, 0x11, 0x66, 0x66, 0x11, 0x66, 0x66,
		0x66, 0x11, 0x66, 0x66, 0x66, 0x66, 0x11, 0x66,
		0x11, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x11
	};

	TestFinish(result, sizeof(result));
}


void
MovieDecoder::TestOpcode9A()
{
	const uint8 data[] = {
			0x00, 0x22, 0xcc, 0xdd,
			0x00, 0x00, 0x00, 0x11,
			0x00, 0x00, 0x00, 0x08,
			0xF0, 0x0F, 0xF0, 0x0F,
			0xFF, 0xFF, 0xFF, 0xFF
	};

	TestInit(0x9, data, sizeof(data));

	const uint8 result[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x00, 0x22,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xcc, 0x00,
		0xdd, 0xdd, 0x00, 0x00, 0x00, 0x00, 0xdd, 0xdd,
		0xdd, 0xdd, 0x00, 0x00, 0x00, 0x00, 0xdd, 0xdd,
		0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
		0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd
	};

	TestFinish(result, sizeof(result));
}


void
MovieDecoder::TestOpcode9B()
{
	const uint8 data[] = {
			0x00, 0x22, 0xdd, 0xcc,
			0x01, 0x24, 0xFF, 0x11
	};

	TestInit(0x9, data, sizeof(data));

	const uint8 result[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x00, 0x22,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xcc, 0x00,
		0xdd, 0xdd, 0x00, 0x00, 0x00, 0x00, 0xdd, 0xdd,
		0xdd, 0xdd, 0x00, 0x00, 0x00, 0x00, 0xdd, 0xdd,
		0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
		0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd
	};

	TestFinish(result, sizeof(result));
}


void
MovieDecoder::TestOpcodeB()
{
	const uint8 data[] = {
		0x01, 0x00, 0x22, 0x22, 0x00, 0x00, 0x22, 0x22,
		0x22, 0x02, 0x00, 0x22, 0x22, 0x00, 0x00, 0x22,
		0x22, 0x22, 0x03, 0x00, 0x22, 0x22, 0x00, 0x00,
		0x22, 0x22, 0x22, 0x05, 0x00, 0x22, 0x22, 0x00,
		0x66, 0x66, 0x66, 0x11, 0x43, 0x66, 0x66, 0x66,
		0x66, 0x66, 0x11, 0x66, 0x66, 0x16, 0x66, 0x66,
		0x66, 0x11, 0x66, 0x66, 0x66, 0x12, 0x11, 0xA3,
		0x11, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x11
	};

	TestInit(0xB, data, sizeof(data));

	const uint8 result[] = {
		0x01, 0x00, 0x22, 0x22, 0x00, 0x00, 0x22, 0x22,
		0x22, 0x02, 0x00, 0x22, 0x22, 0x00, 0x00, 0x22,
		0x22, 0x22, 0x03, 0x00, 0x22, 0x22, 0x00, 0x00,
		0x22, 0x22, 0x22, 0x05, 0x00, 0x22, 0x22, 0x00,
		0x66, 0x66, 0x66, 0x11, 0x43, 0x66, 0x66, 0x66,
		0x66, 0x66, 0x11, 0x66, 0x66, 0x16, 0x66, 0x66,
		0x66, 0x11, 0x66, 0x66, 0x66, 0x12, 0x11, 0xA3,
		0x11, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x11
	};

	TestFinish(result, sizeof(result));
}


void
MovieDecoder::TestOpcodeC()
{
	const uint8 data[] = {
		0x01, 0x00, 0x22, 0x23,
		0x24, 0x40, 0x55, 0x60,
		0x11, 0x0B, 0xF0, 0x12,
		0x13, 0x15, 0x45, 0x70
	};

	TestInit(0xC, data, sizeof(data));

	const uint8 result[] = {
		0x01, 0x01, 0x00, 0x00, 0x22, 0x22, 0x23, 0x23,
		0x01, 0x01, 0x00, 0x00, 0x22, 0x22, 0x23, 0x23,
		0x24, 0x24, 0x40, 0x40, 0x55, 0x55, 0x60, 0x60,
		0x24, 0x24, 0x40, 0x40, 0x55, 0x55, 0x60, 0x60,
		0x11, 0x11, 0x0B, 0x0B, 0xF0, 0xF0, 0x12, 0x12,
		0x11, 0x11, 0x0B, 0x0B, 0xF0, 0xF0, 0x12, 0x12,
		0x13, 0x13, 0x15, 0x15, 0x45, 0x45, 0x70, 0x70,
		0x13, 0x13, 0x15, 0x15, 0x45, 0x45, 0x70, 0x70
	};

	TestFinish(result, sizeof(result));
}


void
MovieDecoder::TestOpcodeD()
{
	const uint8 data[] = {
		0x01, 0x22, 0x40, 0x10
	};

	TestInit(0xD, data, sizeof(data));

	const uint8 result[] = {
		0x01, 0x01, 0x01, 0x01, 0x22, 0x22, 0x22, 0x22,
		0x01, 0x01, 0x01, 0x01, 0x22, 0x22, 0x22, 0x22,
		0x01, 0x01, 0x01, 0x01, 0x22, 0x22, 0x22, 0x22,
		0x01, 0x01, 0x01, 0x01, 0x22, 0x22, 0x22, 0x22,
		0x40, 0x40, 0x40, 0x40, 0x10, 0x10, 0x10, 0x10,
		0x40, 0x40, 0x40, 0x40, 0x10, 0x10, 0x10, 0x10,
		0x40, 0x40, 0x40, 0x40, 0x10, 0x10, 0x10, 0x10,
		0x40, 0x40, 0x40, 0x40, 0x10, 0x10, 0x10, 0x10
	};

	TestFinish(result, sizeof(result));
}


void
MovieDecoder::TestOpcodeE()
{
	const uint8 data[] = {
		0x01
	};

	TestInit(0xE, data, sizeof(data));

	const uint8 result[] = {
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01
	};

	TestFinish(result, sizeof(result));
}


#endif


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
	uint8 bit = (fByte & (1 << fBitPos)) >> fBitPos;
	if (--fBitPos < 0)
		fBitPos = 7;
	return bit;
}


// TwoBitsStreamAdapter
TwoBitsStreamAdapter::TwoBitsStreamAdapter(Stream* stream)
	:
	fStream(stream),
	fByte(0),
	fBitPos(6)
{
}


uint8
TwoBitsStreamAdapter::ReadBits()
{
	if (fBitPos == 6)
		fByte = fStream->ReadByte();
	uint8 bits = (fByte & (3 << fBitPos)) >> fBitPos;
	fBitPos -= 2;
	if (fBitPos < 0)
		fBitPos = 6;
	return bits;
}
