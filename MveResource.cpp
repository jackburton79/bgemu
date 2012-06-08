#include "GraphicsEngine.h"
#include "MemoryStream.h"
#include "MovieDecoder.h"
#include "MveResource.h"
#include "SDL.h"
#include "SoundEngine.h"
#include "Stream.h"

#include <iostream>

enum chunk_type {
	CHUNK_AUDIO_INIT 	= 0,
	CHUNK_AUDIO			= 1,
	CHUNK_VIDEO_INIT 	= 2,
	CHUNK_VIDEO			= 3,
	CHUNK_SHUTDOWN		= 4,
	CHUNK_END			= 5
} ;


enum movie_opcodes {
	OP_END_OF_STREAM		= 0x00,
	OP_END_OF_CHUNK			= 0x01,
	OP_CREATE_TIMER			= 0x02,
	OP_INIT_AUDIO_BUFFERS	= 0x03,
	OP_START_STOP_AUDIO		= 0x04,
	OP_INIT_VIDEO_BUFFERS	= 0x05,
	OP_BLIT_BACKBUFFER		= 0x07,
	OP_AUDIO_FRAME_DATA		= 0x08,
	OP_AUDIO_FRAME_SILENCE	= 0x09,
	OP_INIT_VIDEO_MODE		= 0x0A,
	OP_SET_PALETTE			= 0x0C,
	OP_SET_DECODING_MAP		= 0x0F,
	OP_VIDEO_DATA			= 0x11,
	OP_UNKNOWN_13			= 0x13,
	OP_UNKNOWN_15			= 0x15,
};


enum audio_flags {
	AUDIO_STEREO =		0x1 << 0,
	AUDIO_16BIT =		0x1 << 1,
	AUDIO_COMPRESSED = 	0x1 << 2
};


const int kDpcmDeltaTable[] = {
   0, 1, 2, 3, 4, 5, 6, 7,
   8, 9, 10, 11, 12, 13, 14, 15,
   16, 17, 18, 19, 20, 21, 22, 23,
   24, 25, 26, 27, 28, 29, 30, 31,
   32, 33, 34, 35, 36, 37, 38, 39,
   40, 41, 42, 43, 47, 51, 56, 61,
   66, 72, 79, 86, 94, 102, 112, 122,
   133, 145, 158, 173, 189, 206, 225, 245,
   267, 292, 318, 348, 379, 414, 452, 493,
   538, 587, 640, 699, 763, 832, 908, 991,
   1081, 1180, 1288, 1405, 1534, 1673, 1826, 1993,
   2175, 2373, 2590, 2826, 3084, 3365, 3672, 4008,
   4373, 4772, 5208, 5683, 6202, 6767, 7385, 8059,
   8794, 9597, 10472, 11428, 12471, 13609, 14851, 16206,
   17685, 19298, 21060, 22981, 25078, 27367, 29864, 32589,
   -29973, -26728, -23186, -19322, -15105, -10503, -5481, -1,
   1, 1, 5481, 10503, 15105, 19322, 23186, 26728,
   29973, -32589, -29864, -27367, -25078, -22981, -21060, -19298,
   -17685, -16206, -14851, -13609, -12471, -11428, -10472, -9597,
   -8794, -8059, -7385, -6767, -6202, -5683, -5208, -4772,
   -4373, -4008, -3672, -3365, -3084, -2826, -2590, -2373,
   -2175, -1993, -1826, -1673, -1534, -1405, -1288, -1180,
   -1081, -991, -908, -832, -763, -699, -640, -587,
   -538, -493, -452, -414, -379, -348, -318, -292,
   -267, -245, -225, -206, -189, -173, -158, -145,
   -133, -122, -112, -102, -94, -86, -79, -72,
   -66, -61, -56, -51, -47, -43, -42, -41,
   -40, -39, -38, -37, -36, -35, -34, -33,
   -32, -31, -30, -29, -28, -27, -26, -25,
   -24, -23, -22, -21, -20, -19, -18, -17,
   -16, -15, -14, -13, -12, -11, -10, -9,
   -8, -7, -6, -5, -4, -3, -2, -1
 };


MVEResource::MVEResource(const res_ref &name)
	:
	Resource(name, RES_MVE),
	fTimer(0),
	fLastFrameTime(0)
{
	fDecoder = new MovieDecoder();
}


MVEResource::~MVEResource()
{
	delete fDecoder;
}


void
MVEResource::Play()
{
	char signature[19];
	fData->Read(signature, 20);
	std::cout << signature << std::endl;
	int16 magic[3];

	fData->Read(magic);
	//std::cout << magic[0] << " " << magic[1] << " " << magic[2] << std::endl;

	fLastFrameTime = SDL_GetTicks();
	bool quitting = false;
	bool paused = false;
	SDL_Event event;
	while (!quitting) {
		if (!paused) {
			if (!GetNextChunk())
				break;
		}
		while (SDL_PollEvent(&event) != 0) {
			switch (event.type) {
				case SDL_KEYDOWN: {
					switch (event.key.keysym.sym) {
						case SDLK_q:
							quitting = true;
							break;
						case SDLK_p:
							paused = !paused;
							break;
						default:
							break;
					}
				}
				break;

				case SDL_QUIT:
					quitting = true;
					break;
				default:
					break;
			}
		}
		uint32 currentTime = SDL_GetTicks();
		if (fTimer != 0) {
			if (currentTime < fLastFrameTime + fTimer)
				SDL_Delay((fLastFrameTime + fTimer) - currentTime);
		}
	}
}


bool
MVEResource::GetNextChunk()
{
	try {
		if ((uint32)fData->Position() >= fData->Size())
			return false;
	
		chunk_header header;
		fData->Read(header);
		if (header.type == CHUNK_SHUTDOWN || header.type == CHUNK_END)
			return false;
		else
			DecodeChunk(header);
	} catch (const char *str) {
		std::cout << str << std::endl;
		return false;
	}
	return true;
}


void
MVEResource::DecodeChunk(chunk_header header)
{
	//std::cout << "CHUNK: " << chunktostr(header) << std::endl;
	op_stream_header opHeader;
	do {
		fData->Read(opHeader);
	} while (ExecuteOpcode(opHeader));
}


bool
MVEResource::ExecuteOpcode(op_stream_header opcode)
{	
	//std::cout << opcodetostr(opcode.type) << " (" << std::hex << (int)opcode.type << ") ";
	//std::cout << " length: " << opcode.length << std::endl;
	
	switch (opcode.type) {
		case OP_END_OF_STREAM:
		case OP_END_OF_CHUNK:
			return false;
		case OP_INIT_AUDIO_BUFFERS:
		{
			uint16 unknown;
			uint16 flags;
			uint16 sampleRate;
			fData->Read(unknown);
			fData->Read(flags);
			fData->Read(sampleRate);
			if (opcode.version == 0) {
				uint16 minBufferLength;
				fData->Read(minBufferLength);
				SoundEngine::Get()->InitBuffers(flags & AUDIO_STEREO,
						flags & AUDIO_16BIT,
						sampleRate, minBufferLength);
			} else if (opcode.version == 1) {
				uint32 minBufferLength;
				fData->Read(minBufferLength);
				SoundEngine::Get()->InitBuffers(flags & AUDIO_STEREO,
						flags & AUDIO_16BIT,
						sampleRate, minBufferLength);
			}
			break;
		}
		/*case OP_START_STOP_AUDIO:
			if (SoundEngine::Get()->IsPlaying()) {
				SoundEngine::Get()->StopAudio();
			} else {
				SoundEngine::Get()->StartAudio();
			}
			break;*/
		case OP_INIT_VIDEO_MODE:
		{
			uint16 xRes, yRes, flags;
			fData->Read(xRes);
			fData->Read(yRes);
			fData->Read(flags);
			fDecoder->InitVideoMode(xRes, yRes, flags);
			break;
		}
		case OP_INIT_VIDEO_BUFFERS:
		{
			uint16 width, height, count = 0, trueColor = 0;
			fData->Read(width);
			fData->Read(height);
			if (opcode.version > 0) {
				fData->Read(count);
				if (opcode.version > 1)
					fData->Read(trueColor);
			}
			fDecoder->AllocateBuffer(width * 8, height * 8);
			break;
		}
		case OP_SET_PALETTE:
		{
			uint16 start, count;
			fData->Read(start);
			fData->Read(count);

			fDecoder->SetPalette(start, count, (uint8 *)(fData->Data()) + fData->Position());
			fData->Seek(count * 3, SEEK_CUR);
			break;
		}
		case OP_BLIT_BACKBUFFER:
			// TODO: Install palette first. ??
			fDecoder->BlitBackBuffer();
			fLastFrameTime = SDL_GetTicks();
			fData->Seek(opcode.length, SEEK_CUR);
			break;
		case OP_SET_DECODING_MAP:
		{	
			uint8 *map = new uint8[opcode.length];
			fData->Read(map, opcode.length);
			fDecoder->SetDecodingMap(map, opcode.length);
			break;
		}
		case OP_VIDEO_DATA:
			fDecoder->DecodeDataBlock(fData, opcode.length);
			break;
		case OP_AUDIO_FRAME_DATA:
		case OP_AUDIO_FRAME_SILENCE:
		{
			uint16 seqIndex;
			fData->Read(seqIndex);
			uint16 streamMask;
			fData->Read(streamMask);
			uint16 streamLen;
			fData->Read(streamLen);
			//printf("seq: %d, mask: 0x%x\n", seqIndex, streamMask);
			if (opcode.type == OP_AUDIO_FRAME_DATA) {
				ReadAudioData(fData, streamLen);
			}
			fData->Seek(opcode.length - (3 * sizeof(uint16)), SEEK_CUR);
			break;
		}
		case OP_CREATE_TIMER:
		{
			uint32 rate;
			uint16 subDivision;
			fData->Read(rate);
			fData->Read(subDivision);
			fTimer = (rate * subDivision) / 1000;
			break;
		}
		default:
			fData->Seek(opcode.length, SEEK_CUR);
			break;
	}

	return true;
}


void
MVEResource::ReadAudioData(Stream* stream, uint16 numSamples)
{
	int numChannels = SoundEngine::Get()->Buffer()->IsStereo() ? 2 : 1;
	sint16 predictors[numChannels];
	//for (int c = 0; c < numChannels; c++)
		//stream->Read(predictors[c]);
}


const char *
chunktostr(chunk_header header)
{
	switch (header.type) {
		case CHUNK_VIDEO_INIT:
			return "init video";
		case CHUNK_VIDEO:
			return "video";
		case CHUNK_END:
			return "end";
		case CHUNK_SHUTDOWN:
			return "shutdown";
		case CHUNK_AUDIO_INIT:
			return "init audio";
		case CHUNK_AUDIO:
			return "audio data";
		default:
			return "unknown";
	}
}


const char *
opcodetostr(uint8 opcode)
{
	switch (opcode) {
		case OP_END_OF_STREAM:
			return "end of stream";
		case OP_END_OF_CHUNK:
			return "get next chunk";
		case OP_INIT_VIDEO_MODE:
			return "init_video_mode";
		case OP_INIT_VIDEO_BUFFERS:
			return "init video buffers";
		case OP_SET_PALETTE:
			return "set palette";
		case OP_AUDIO_FRAME_DATA:
			return "audio frame (data)";
		case OP_AUDIO_FRAME_SILENCE:
			return "audio frame (silence)";
		case OP_CREATE_TIMER:
			return "create timer";
		case OP_START_STOP_AUDIO:
			return "start/stop audio";
		case OP_BLIT_BACKBUFFER:
			return "blit buffer to screen";
		case OP_SET_DECODING_MAP:
			return "set decoding map";
		case OP_VIDEO_DATA:
			return "video data";
		case OP_UNKNOWN_13:
		case OP_UNKNOWN_15:
			return "unknown";
		default:
			return "unsupported";
	}
}

