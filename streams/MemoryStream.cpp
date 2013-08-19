#include "MemoryStream.h"


#include <iostream>
#include <string.h>

MemoryStream::MemoryStream(const uint8 *data, int size, bool owns)
	:
	fData(const_cast<uint8*>(data)),
	fSize((uint32)size),
	fPosition(0),
	fOwnsBuffer(owns)
{
}


MemoryStream::MemoryStream(int size)
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


MemoryStream::~MemoryStream()
{
	if (fOwnsBuffer)
		delete[] fData;
}



ssize_t
MemoryStream::Read(void *dst, int size)
{
	if (fPosition >= fSize)
		return -1;

	ssize_t readable = fSize - fPosition;
	if (size > readable)
		size = readable;

	memcpy(dst, fData + fPosition, size);

	fPosition += size;

	return size;
}


ssize_t
MemoryStream::ReadAt(int pos, void *dst, int size)
{
	if (pos >= fSize)
		return -1;

	ssize_t readable = fSize - pos;
	if (size > readable)
		size = readable;

	memcpy(dst, fData + pos, size);

	return size;
}


ssize_t
MemoryStream::WriteAt(int pos, const void *src, int size)
{
	if (pos >= fSize)
		return -1;

	ssize_t writable = fSize - pos;
	if (size > writable)
		size = writable;
	memcpy(fData + pos, src, size);

	return size;
}


int32
MemoryStream::Seek(int32 where, int whence)
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
MemoryStream::Position() const
{
	return fPosition;
}


uint32
MemoryStream::Size() const
{
	return fSize;
}


void *
MemoryStream::Data() const
{
	return fData;
}

