/*
 * EncryptedStream.cpp
 *
 *  Created on: 23/ago/2013
 *      Author: stefano
 */

#include "EncryptedStream.h"

#include <iostream>

static const uint8 kEncryptionKey[64] = {
	0x88, 0xa8, 0x8f, 0xba, 0x8a, 0xd3, 0xb9, 0xf5,
	0xed, 0xb1, 0xcf, 0xea, 0xaa, 0xe4, 0xb5, 0xfb,
	0xeb, 0x82, 0xf9, 0x90, 0xca, 0xc9, 0xb5, 0xe7,
	0xdc, 0x8e, 0xb7, 0xac, 0xee, 0xf7, 0xe0, 0xca,
	0x8e, 0xea, 0xca, 0x80, 0xce, 0xc5, 0xad, 0xb7,
	0xc4, 0xd0, 0x84, 0x93, 0xd5, 0xf0, 0xeb, 0xc8,
	0xb4, 0x9d, 0xcc, 0xaf, 0xa5, 0x95, 0xba, 0x99,
	0x87, 0xd2, 0x9d, 0xe3, 0x91, 0xba, 0x90, 0xca
};


EncryptedStream::EncryptedStream(Stream *stream)
	:
	fKeySize(64)
{
	fEncryptedStream = stream->Adopt();
	fEncryptedStream->Seek(2, SEEK_SET);
}


EncryptedStream::~EncryptedStream()
{
	delete fEncryptedStream;
}


/* virtual */
ssize_t
EncryptedStream::ReadAt(off_t pos, void *dst, size_t size)
{
	uint8* pointer = static_cast<uint8*>(dst);
	ssize_t totalSizeRead = 0;
	for (size_t i = 0; i < size; i++) {
		uint8 byteRead;
		ssize_t read = fEncryptedStream->ReadAt(pos + 2, &byteRead, sizeof(byteRead));
		if (read < 0)
			return read;

		totalSizeRead += read;
		if (size_t(read) < sizeof(byteRead))
			break;
		byteRead ^= kEncryptionKey[pos % fKeySize];
		*pointer++ = byteRead;
		pos++;
	}

	return totalSizeRead;
}


off_t
EncryptedStream::Seek(off_t where, int whence)
{
	return fEncryptedStream->Seek(where, whence);
}


off_t
EncryptedStream::Position() const
{
	return fEncryptedStream->Position() - 2;
}


size_t
EncryptedStream::Size() const
{
	return fEncryptedStream->Size() - 2;
}
