#include "MemoryStream.h"

#include <iostream>
#include <string.h>

TMemoryStream::TMemoryStream(const uint8 *data, int size, bool owns)
	:
	fData(const_cast<uint8*>(data)),
	fSize((uint32)size),
	fPosition(0),
	fOwnsBuffer(owns)
{
}


TMemoryStream::TMemoryStream(int size)
	:
	fData(NULL),
	fSize((uint32)size),
	fPosition(0),
	fOwnsBuffer(true)
{
	try {
		fData = new uint8[size];
	} catch (...) {
		std::cout << __PRETTY_FUNCTION__ << ": Can't allocate memory!" << std::endl;
		throw "Cannot allocate memory!";
	}
}


TMemoryStream::~TMemoryStream()
{
	if (fOwnsBuffer)
		delete[] fData;
}


ssize_t
TMemoryStream::ReadAt(int pos, void *dst, int size)
{
	ssize_t readable = fSize - pos;
	if (size > readable)
		size = readable;
	memcpy(dst, fData + pos, size);

	return size;
}


ssize_t
TMemoryStream::WriteAt(int pos, void *src, int size)
{
	ssize_t writable = fSize - pos;
	if (size > writable)
		size = writable;
	memcpy(fData + pos, src, size);

	return size;
}


int32
TMemoryStream::Seek(int32 where, int whence)
{
	switch (whence) {
		case SEEK_SET:
			fPosition = where;
			break;
		case SEEK_CUR:
			fPosition += where;
			break;
		case SEEK_END:
			fPosition = fSize + where;
			break;
		default:
			break;
	}
	
	return fPosition;
}


int32
TMemoryStream::Position() const
{
	return fPosition;
}


uint32
TMemoryStream::Size() const
{
	return fSize;
}


void *
TMemoryStream::Data() const
{
	return fData;
}

