#include "GraphicsEngine.h"
#include "MemoryStream.h"
#include "MovieDecoder.h"
#include "MveResource.h"
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


MVEResource::MVEResource(const res_ref &name)
	:
	Resource(name, RES_MVE)
{
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

	fDecoder = new MovieDecoder();

	bool quitting = false;
	SDL_Event event;
	while (GetNextChunk() && !quitting) {
		while (SDL_PollEvent(&event) != 0) {
			switch (event.type) {
				case SDL_KEYDOWN: {
					switch (event.key.keysym.sym) {
						case SDLK_q:
							quitting = true;
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
		//cout << "length: " << header.length << " type: " << header.type;
		//cout << endl;	
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
	std::cout << "CHUNK: " << chunktostr(header) << std::endl;
	op_stream_header opHeader;
	do {
		fData->Read(opHeader);
	} while (ExecuteOpcode(opHeader));
}


bool
MVEResource::ExecuteOpcode(op_stream_header opcode)
{	
	std::cout << opcodetostr(opcode.type) << " (" << std::hex << (int)opcode.type << ") ";
	std::cout << " length: " << opcode.length << std::endl;
	
	//cout << "version: " << (int)opcode.version << endl;

	switch (opcode.type) {
		case OP_END_OF_STREAM:
		case OP_END_OF_CHUNK:
			return false;
		case OP_INIT_VIDEO_MODE:
		{
			uint16 xRes, yRes, flags;
			fData->Read(xRes);
			fData->Read(yRes);
			fData->Read(flags);
			printf("OP_INIT_VIDEO_MODE: w: %d, h: %d, flags: 0x%x\n",
					xRes, yRes, flags);
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
			width *= 8;
			height *= 8;
			printf("OP_INIT_VIDEO_BUFFERS: w: %d, h: %d, count: %d\n",
					width, height, count);
			fDecoder->AllocateBuffer(width, height);
			break;
		}
		case OP_SET_PALETTE:
		{
			printf("OP_SET_PALETTE\n");
			uint16 start, count;
			fData->Read(start);
			fData->Read(count);

			fDecoder->SetPalette(start, count, (uint8 *)(fData->Data()) + fData->Position());
			fData->Seek(count * 3, SEEK_CUR);
			break;
		}
		case OP_BLIT_BACKBUFFER:
			printf("OP_BLIT_BACKBUFFER\n");
			fDecoder->BlitBackBuffer();
			fData->Seek(opcode.length, SEEK_CUR);
			break;
		case OP_SET_DECODING_MAP:
		{	
			printf("OP_SET_DECODING_MAP\n");
			uint8 *map = new uint8[opcode.length];
			fData->Read(map, opcode.length);
			fDecoder->SetDecodingMap(map, opcode.length);
			break;
		}
		case OP_VIDEO_DATA:
			printf("OP_VIDEO_DATA\n");
			fDecoder->DecodeDataBlock(fData, opcode.length);
			break;
		case OP_AUDIO_FRAME_DATA:
		case OP_AUDIO_FRAME_SILENCE:
		case OP_CREATE_TIMER:
		default:
			//cout << "\tunimplemented" << endl;
			fData->Seek(opcode.length, SEEK_CUR);
			break;
	}
	SDL_Delay(100);
	return true;
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

