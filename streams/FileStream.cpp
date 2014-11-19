#include "FileStream.h"
#include "Utils.h"


FileStream::FileStream(const char *filename, openmode mode,
		casemode caseMode)
{		
	if (!SetTo(filename, mode, caseMode))
		throw -1;
}


FileStream::FileStream()
	:
	fFileHandle(NULL)
{
}


FileStream::~FileStream()
{
	if (fFileHandle != NULL)
		fclose(fFileHandle);
}


bool
FileStream::SetTo(const char *filename, openmode mode, casemode caseMode)
{
	const char *flags = NULL;
	
	switch (mode) {
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
	
	if (caseMode == CASE_SENSITIVE)
		fFileHandle = fopen(filename, flags);
	else
		fFileHandle = fopen_case(filename, flags);

	return fFileHandle != NULL;
}


bool
FileStream::IsValid() const
{
	return fFileHandle != NULL;
}


off_t
FileStream::Seek(off_t where, int whence)
{
	fseek(fFileHandle, where, whence);
	return ftell(fFileHandle);
}


off_t
FileStream::Position() const
{
	return ftell(fFileHandle);
}


bool
FileStream::EoF()
{
	return feof(fFileHandle) != 0;
}


ssize_t
FileStream::ReadAt(off_t pos, void *dst, size_t size)
{
	off_t oldPos = ftell(fFileHandle);
	fseek(fFileHandle, pos, SEEK_SET);
	ssize_t read = fread(dst, 1, size, fFileHandle);
	fseek(fFileHandle, oldPos, SEEK_SET);

	return read;
}


ssize_t
FileStream::Read(void *dst, size_t count)
{
	return fread(dst, 1, count, fFileHandle);
}


ssize_t
FileStream::Write(const void *src, size_t count)
{
	return fwrite(src, 1, count, fFileHandle);
}
	

uint8
FileStream::ReadByte()
{
	uint8 result;
	
	if (fread(&result, 1, 1, fFileHandle) != 1)
		throw -1;
		
	return result;
}


uint16
FileStream::ReadWordBE(void)
{
	uint8 result1 = ReadByte();
	uint8 result2 = ReadByte();
	
	return (uint16)((result1) | (result2 << 8));
}


uint16
FileStream::ReadWordLE(void)
{
	uint16 result;
	if (fread(&result, sizeof(result), 1, fFileHandle) != sizeof(result))
		throw -1;
	return result;
}


uint32
FileStream::ReadDWordBE(void)
{	
	uint16 result1 = ReadWordBE();
	uint16 result2 = ReadWordBE();
	
	return (uint32)((result1) | (result2 << 16));
}


uint32
FileStream::ReadDWordLE(void)
{
	uint32 result;
	if (fread(&result, sizeof(result), 1, fFileHandle) != sizeof(result))
		throw -1;
	return result;
}


size_t
FileStream::Size() const
{
	const off_t oldpos = Position();
	size_t fileSize = const_cast<FileStream*>(this)->Seek(0L, SEEK_END);
	
	const_cast<FileStream*>(this)->Seek(oldpos, SEEK_SET);

	return fileSize;
}
