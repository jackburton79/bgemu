#include "FileStream.h"
#include "Utils.h"

#include <string.h>

FileStream::FileStream(const char *filename, int mode)
{		
	int status = SetTo(filename, mode);
	if (status != 0) {
		std::string error;
		error += ::strerror(status);
		error += " (\"";
		error += filename;
		error += "\")";
		throw std::runtime_error(error);
	}
}


FileStream::FileStream()
	:
	fFileHandle(NULL)
{
}


FileStream::~FileStream()
{
	if (fFileHandle != NULL)
		::fclose(fFileHandle);
}


int
FileStream::SetTo(const char *filename, int mode)
{
	const char *flags = NULL;
	
	int accessMode = mode & ACCESS_MODE_MASK;
	int openFlags = mode & FLAGS_MASK;
	
	switch (accessMode) {
		case READ_WRITE:
			flags = "wrb";	
			break;
		case WRITE_ONLY:
			flags = "wb";
			break;
		case READ_ONLY:
		default:
			flags = "rb";
			break;
	}
	
	if (openFlags & FileStream::IGNORE_CASE)
		fFileHandle = ::fopen_case(filename, flags);
	else
		fFileHandle = ::fopen(filename, flags);

	if (fFileHandle == NULL)
		return errno;

	return 0;
}


bool
FileStream::IsValid() const
{
	return fFileHandle != NULL;
}


off_t
FileStream::Seek(off_t where, int whence)
{
	::fseek(fFileHandle, where, whence);
	return ::ftell(fFileHandle);
}


off_t
FileStream::Position() const
{
	return ::ftell(fFileHandle);
}


size_t
FileStream::Size() const
{
	const off_t oldpos = Position();
	size_t fileSize = const_cast<FileStream*>(this)->Seek(0L, SEEK_END);

	const_cast<FileStream*>(this)->Seek(oldpos, SEEK_SET);

	return fileSize;
}


bool
FileStream::EoF()
{
	return ::feof(fFileHandle) != 0;
}


ssize_t
FileStream::ReadAt(off_t pos, void *dst, size_t size)
{
	off_t oldPos = ::ftell(fFileHandle);
	::fseek(fFileHandle, pos, SEEK_SET);
	ssize_t read = ::fread(dst, 1, size, fFileHandle);
	::fseek(fFileHandle, oldPos, SEEK_SET);

	return read;
}


ssize_t
FileStream::Read(void *dst, size_t count)
{
	return ::fread(dst, 1, count, fFileHandle);
}


ssize_t
FileStream::Write(const void *src, size_t count)
{
	return ::fwrite(src, 1, count, fFileHandle);
}
	

uint8
FileStream::ReadByte()
{
	uint8 result;
	if (::fread(&result, 1, 1, fFileHandle) != 1)
		throw std::runtime_error("ReadByte() error");
		
	return result;
}


uint16
FileStream::ReadWordBE()
{
	uint8 result1 = ReadByte();
	uint8 result2 = ReadByte();
	
	return (uint16)((result1 << 8) | (result2));
}


uint16
FileStream::ReadWordLE()
{
	uint16 result;
	if (::fread(&result, 1, sizeof(result), fFileHandle) != sizeof(result))
		throw std::runtime_error("ReadWordLE() read error");
	return result;
}


uint32
FileStream::ReadDWordBE()
{	
	uint16 result1 = ReadWordBE();
	uint16 result2 = ReadWordBE();
	
	return (uint32)((result1 << 16) | (result2));
}


uint32
FileStream::ReadDWordLE()
{
	uint32 result;
	if (::fread(&result, 1, sizeof(result), fFileHandle) != sizeof(result))
		throw std::runtime_error("ReadDWordLE() read error");
	return result;
}
