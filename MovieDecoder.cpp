#include "Bitmap.h"
#include "GraphicsEngine.h"
#include "MovieDecoder.h"
#include "Stream.h"
#include "SDL.h"

#include <assert.h>
#include <iostream>


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
}


bool
MovieDecoder::AllocateBuffer(uint16 width, uint16 height)
{
	//height = 480;
	fNewFrame = GraphicsEngine::CreateBitmap(width, height, 8);
	fCurrentFrame = GraphicsEngine::CreateBitmap(width, height, 8);
	fPreviousFrame = GraphicsEngine::CreateBitmap(width, height, 8);

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
	std::cout << __PRETTY_FUNCTION__ << std::endl;
	delete[] fDecodingMap;
		
	fDecodingMap = map;
	fMapSize = size;
}


void
MovieDecoder::SetPalette(uint16 start, uint16 count, uint8 palette[])
{
	std::cout << "Set palette:" << std::endl;
	std::cout << "start: " << start << " count: " << count << std::endl;
	Color *colors = new Color[count];
	for (int32 i = 0; i < count; i++) {
		colors[i].r = *palette++ >> 2;
		colors[i].g = *palette++ >> 2;
		colors[i].b = *palette++ >> 2;
		colors[i].a = 0;
	}
	fNewFrame->SetColors(colors, start, count);
	delete[] colors;
}


void
MovieDecoder::DecodeDataBlock(Stream *stream, uint32 length)
{
	std::cout << __PRETTY_FUNCTION__ << std::endl;

	fActiveRect.x = fActiveRect.y = 0;

	uint32 pos = stream->Position();
	for (uint32 i = 0; i < fMapSize; i++) {
		for (int b = 0; b < 2; b++) {
			uint8 opcode;
			if (b == 0)
				opcode = fDecodingMap[i] & 0x0F;
			else
				opcode = (fDecodingMap[i] & 0xF0) >> 4;
			std::cout << "opcode " << (int)opcode << std::endl;

			uint8 *pixels = (uint8 *)fNewFrame->Pixels()
					+ (fActiveRect.y * fNewFrame->Pitch() + fActiveRect.x);
			switch (opcode) {
				case 0x0:
					GraphicsEngine::Get()->BlitBitmap(
							fCurrentFrame, &fActiveRect, fNewFrame, &fActiveRect);
					break;

				case 0x1:
					break;
				case 0x2:
				{
					uint8 byte = stream->ReadByte();
					GFX::rect rect = fActiveRect;
					if (b < 56) {
					   rect.x += 8 + (byte % 7);
					   rect.y += byte / 7;
					} else {
					   rect.x += -14 + ((byte - 56) % 29);
					   rect.y +=  8 + ((byte - 56) / 29);
					}
					GraphicsEngine::Get()->BlitBitmap(
							fNewFrame, &rect, fNewFrame, &fActiveRect);
					break;
				}
				case 0x3:
				{
					// Same as above but negated
					// TODO: What about less code duplication ?
					uint8 byte = stream->ReadByte();
					GFX::rect rect = fActiveRect;
					if (b < 56) {
					   rect.x -= 8 + (byte % 7);
					   rect.y -= byte / 7;
					} else {
					   rect.x -= -14 + ((byte - 56) % 29);
					   rect.y -=  8 + ((byte - 56) / 29);
					}
					GraphicsEngine::Get()->BlitBitmap(
							fNewFrame, &rect, fNewFrame, &fActiveRect);
					break;
				}
				case 0x4:
				{
					uint8 byte = stream->ReadByte();
					GFX::rect rect = fActiveRect;
					rect.x += (byte & 0x0F) - 8;
					rect.y += ((byte & 0xF0) >> 4) - 8;
					GraphicsEngine::Get()->BlitBitmap(
							fCurrentFrame, &rect, fNewFrame, &fActiveRect);
					break;
				}
				case 0x5:
				{
					GFX::rect rect = fActiveRect;
					rect.x += (stream->ReadByte() & 0x0F) - 8;
					rect.y += ((stream->ReadByte() & 0xF0) >> 4) - 8;
					GraphicsEngine::Get()->BlitBitmap(
							fCurrentFrame, &rect, fNewFrame, &fActiveRect);
					break;
				}
				case 0x6:
					break;
				case 0x7:
				{
					// TODO: Implement!
					uint8 p0 = stream->ReadByte();
					uint8 p1 = stream->ReadByte();

					printf("activeRect: %d %d\n", fActiveRect.x, fActiveRect.y);
					if (p0 < p1) {
						uint8 rowPixels = stream->ReadByte();
						for (int i = 0; i < 8; i++) {
							for (int bit = 7; bit >= 0; bit--) {
								if (rowPixels & (1 << bit))
									*pixels++ = p1;
								else
									*pixels++ = p0;
							}
							pixels += fNewFrame->Pitch() - 8;
						}
					} else {
						for (int i = 0; i < 2; i++)
							stream->ReadByte();
					}
					break;
				}
				case 0x8:
				{
					// TODO: Implement!
					uint8 p0 = stream->ReadByte();
					uint8 p1 = stream->ReadByte();
					if (p0 < p1) {
						stream->Seek(14, SEEK_CUR);
					} else {
						stream->Seek(10, SEEK_CUR);
					}
					break;
				}
				case 0x9:
				{
					// TODO: Implement!
					uint8 p0 = stream->ReadByte();
					uint8 p1 = stream->ReadByte();
					uint8 p2 = stream->ReadByte();
					uint8 p3 = stream->ReadByte();
					if (p0 <= p1 and p2 <= p3) {
						for (int i = 0; i < 16; i++) {
							uint8 value = stream->ReadByte();
							for (int c = 6; c >= 0; c -= 2) {
								if (value & (3 << c))
									*pixels++ = p1;
								else
									*pixels++ = p0;
							}
							if (i % 2 == 0)
								pixels += fNewFrame->Pitch() - 8;
						}
					} else if (p0 <= p1 and p2 > p3) {
						stream->Seek(4, SEEK_CUR);
					} else if (p0 > p1 and p2 <= p3 ) {
						stream->Seek(8, SEEK_CUR);
					} else {
						stream->Seek(8, SEEK_CUR);
					}
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
						if (y % 8 == 0)
							pixels += fNewFrame->Pitch() - 8;
					}
					break;
				}
				case 0xC:
				{
					uint8 data;
					for (int i = 0; i < 16; i++) {
						stream->Read(data);
						/**pixels++ = data;
						if (i % 8 == 0) {
							pixels += fNewFrame->Pitch();
						}*/
					}
					break;
				}
				case 0xD:
				{
					uint8 data;
					for (int i = 0; i < 4; i++) {
						stream->Read(data);
						/**pixels++ = data;
						if (i % 8 == 0) {
							pixels += fCurrentFrame->pitch;
						}*/
					}
					break;
				}
				case 0xE:
				{
					uint8 pixel = stream->Read(pixel);
					std::cout << "FillRect(" << (int)pixel << ")" << std::endl;
					SDL_FillRect(fNewFrame->Surface(), (SDL_Rect*)&fActiveRect, pixel);
					break;
				}
				case 0xF:
				{
					// TODO: checker pattern
					uint8 pixel;
					stream->Read(pixel);
					stream->Read(pixel);
					std::cout << "FillRect(" << (int)pixel << ")" << std::endl;
					SDL_FillRect(fNewFrame->Surface(), (SDL_Rect*)&fActiveRect, pixel);
					break;
				}
				default:
					printf("BAD OPCODE\n");
					throw "BAD OPCODE!!!";
					break;
			}
			fActiveRect.x += 8;
			if (fActiveRect.x >= fNewFrame->Width()) {
				fActiveRect.x = 0;
				fActiveRect.y += 8;
			}
		}
	}
	//GraphicsEngine::Get()->BlitToScreen(fCurrentFrame, NULL, NULL);

	stream->Seek(pos + length, SEEK_SET);
}


void
MovieDecoder::BlitBackBuffer()
{
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
