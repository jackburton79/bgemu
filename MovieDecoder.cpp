#include "MovieDecoder.h"
#include "Stream.h"

#include <assert.h>
#include <iostream>


MovieDecoder::MovieDecoder()
	:
	fDecodingMap(NULL),
	fMapSize(0),
	fCurrentFrame(NULL),
	fPreviousFrame(NULL)
{
	fActiveRect.x = fActiveRect.y = 0;
	fActiveRect.w = fActiveRect.h = 8;
}


MovieDecoder::~MovieDecoder()
{
	delete[] fDecodingMap;
	SDL_FreeSurface(fCurrentFrame);
	SDL_FreeSurface(fPreviousFrame);
}


void
MovieDecoder::AllocateSurface(uint16 width, uint16 height)
{
	assert(fCurrentFrame == NULL);
	fCurrentFrame = SDL_CreateRGBSurface(SDL_SWSURFACE,
										 width, height, 8, 0, 0, 0, 0);
}


void
MovieDecoder::SetDecodingMap(uint8 *map, uint32 size)
{
	cout << __PRETTY_FUNCTION__ << endl;
	if (fDecodingMap)
		delete[] fDecodingMap;
		
	fDecodingMap = new uint8[size];
	memcpy(fDecodingMap, map, size);
	fMapSize = size;
}


void
MovieDecoder::SetPalette(uint16 start, uint16 count, uint8 palette[])
{
	cout << "Set palette:" << endl;
	cout << "start: " << start << " count: " << count << endl;
	SDL_Color *colors = new SDL_Color[count];
	for (int32 i = 0; i < count; i++) {
		colors[i].r = *palette++;
		colors[i].g = *palette++;
		colors[i].b = *palette++;
		colors[i].unused = 0;
	}
	SDL_SetColors(fCurrentFrame, colors, start, count);
	delete[] colors;
}


void
MovieDecoder::DecodeDataBlock(TStream *stream, uint32 length)
{
	cout << __PRETTY_FUNCTION__ << endl;
	uint8 opcode;
	uint32 pos = stream->Position();
	for (uint32 i = 0; i < fMapSize; i++) {
		opcode = fDecodingMap[i];
		switch (opcode) {
			case 0x0:
			case 0x1:
				break;
			
			case 0xB:
			{
				uint8 *pixels = (uint8 *)fCurrentFrame->pixels + (fActiveRect.y * fCurrentFrame->pitch + fActiveRect.x);
				uint8 data;
				for (int i = 0; i < 64; i++) {
					stream->Read(data);
					/**pixels++ = data;		
					if (i % 8 == 0) {
						pixels += fCurrentFrame->pitch;
					}*/
				}
				break;
			}
			case 0xC:
			{
				uint8 *pixels = (uint8 *)fCurrentFrame->pixels + (fActiveRect.y * fCurrentFrame->pitch + fActiveRect.x);
				uint8 data;
				for (int i = 0; i < 16; i++) {
					stream->Read(data);
					/**pixels++ = data;		
					if (i % 8 == 0) {
						pixels += fCurrentFrame->pitch;
					}*/
				}
				break;
			}
			case 0xD:
			{
				uint8 *pixels = (uint8 *)fCurrentFrame->pixels + (fActiveRect.y * fCurrentFrame->pitch + fActiveRect.x);
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
				uint8 pixel;
				cout << "FillRect(" << (int)pixel << ")" << endl;
				stream->Read(pixel);
				SDL_FillRect(fCurrentFrame, &fActiveRect, pixel);
				break;
			}
			case 0xF:
			{
				uint8 pixel;
				cout << "FillRect(" << (int)pixel << ")" << endl;
				stream->Read(pixel);
				stream->Read(pixel);
				SDL_FillRect(fCurrentFrame, &fActiveRect, pixel);
				break;
			}	
			default:
				cout << (int)opcode << endl;
				break;
		}	
		fActiveRect.x += 8;
		if (fActiveRect.x >= fCurrentFrame->w) {
			fActiveRect.x = 0;
			fActiveRect.y += 8;
		}
	}
	SDL_Flip(fCurrentFrame);
	stream->Seek(pos + length, SEEK_SET);
}


SDL_Surface *
MovieDecoder::CurrentFrame() const
{
	return fCurrentFrame;
}


