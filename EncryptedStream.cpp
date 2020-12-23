/*
 * EncryptedStream.cpp
 *
 *  Created on: 23/ago/2013
 *      Author: stefano
 */

#include "EncryptedStream.h"

#include <iostream>

EncryptedStream::EncryptedStream(Stream* stream, const uint8* key, size_t keyLength)
	:
	fKey(key),
	fKeySize(keyLength)
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
		byteRead ^= fKey[pos % fKeySize];
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
