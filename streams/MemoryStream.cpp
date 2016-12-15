#include "MemoryStream.h"


#include <iostream>
#include <string.h>

MemoryStream::MemoryStream(const uint8 *data, size_t size, bool owns)
	:
	fData(const_cast<uint8*>(data)),
	fSize(size),
	fPosition(0),
	fOwnsBuffer(owns)
{
}


MemoryStream::MemoryStream(size_t size)
	:
	fData(NULL),
	fSize(size),
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
MemoryStream::Read(void *dst, size_t size)
{
	if (fPosition < 0 || (size_t)fPosition >= fSize)
		return -1;

	size_t readable = fSize - fPosition;
	if (size > readable)
		size = readable;

	memcpy(dst, fData + fPosition, size);

	fPosition += size;

	return size;
}


ssize_t
MemoryStream::ReadAt(off_t pos, void *dst, size_t size)
{
	if (pos < 0 || (size_t)pos >= fSize)
		return -1;

	size_t readable = fSize - pos;
	if (size > readable)
		size = readable;

	memcpy(dst, fData + pos, size);

	return size;
}


ssize_t
MemoryStream::WriteAt(off_t pos, const void *src, size_t size)
{
	if (pos < 0 || (size_t)pos >= fSize)
		return -1;

	size_t writable = fSize - pos;
	if (size > writable)
		size = writable;

	memcpy(fData + pos, src, size);

	return size;
}


off_t
MemoryStream::Seek(off_t where, int whence)
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


off_t
MemoryStream::Position() const
{
	return fPosition;
}


size_t
MemoryStream::Size() const
{
	return fSize;
}


void*
MemoryStream::Data() const
{
	return fData;
}


/* virtual */
MemoryStream*
MemoryStream::Clone() const
{
	MemoryStream* newStream = new MemoryStream(Size());
	newStream->fPosition = fPosition;
	memcpy(newStream->fData, Data(), Size());
	return newStream;
}


/* virtual */
MemoryStream*
MemoryStream::Adopt()
{
	MemoryStream* newStream = new MemoryStream(fData, Size(), true);
	
	fData = NULL;
	fSize = 0;
	
	return newStream;
}
