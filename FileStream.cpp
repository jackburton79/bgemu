#include "FileStream.h"
#include "Utils.h"


TFileStream::TFileStream(const char *filename, openmode mode,
		casemode caseMode)
{		
	if (!SetTo(filename, mode, caseMode))
		throw -1;
}


TFileStream::TFileStream()
	:
	fFileHandle(NULL)
{
}


TFileStream::~TFileStream()
{
	if (fFileHandle != NULL)
		fclose(fFileHandle);
}


bool
TFileStream::SetTo(const char *filename, openmode mode, casemode caseMode)
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
TFileStream::IsValid() const
{
	return fFileHandle != NULL;
}


int32
TFileStream::Seek(int32 where, int whence)
{
	fseek(fFileHandle, where, whence);
	return ftell(fFileHandle);
}


int32
TFileStream::Position() const
{
	return ftell(fFileHandle);
}


bool
TFileStream::EoF()
{
	return feof(fFileHandle) != 0;
}


ssize_t
TFileStream::ReadAt(int pos, void *dst, int size)
{
	int oldPos = ftell(fFileHandle);
	fseek(fFileHandle, pos, SEEK_SET);
	ssize_t read = fread(dst, 1, size, fFileHandle);
	fseek(fFileHandle, oldPos, SEEK_SET);

	return read;
}


ssize_t
TFileStream::Read(void *dst, int count)
{
	return fread(dst, 1, count, fFileHandle);
}


ssize_t
TFileStream::Write(void *src, int count)
{
	return fwrite(src, 1, count, fFileHandle);
}
	

uint8
TFileStream::ReadByte()
{
	uint8 result;
	
	if (fread(&result, 1, 1, fFileHandle) != 1)
		throw -1;
		
	return result;
}


uint16
TFileStream::ReadWordBE(void)
{
	uint8 result1 = ReadByte();
	uint8 result2 = ReadByte();
	
	return (uint16)((result1) | (result2 << 8));
}


uint16
TFileStream::ReadWordLE(void)
{
	uint16 result;
	if (fread(&result, sizeof(result), 1, fFileHandle) != sizeof(result))
		throw -1;
	return result;
}


uint32
TFileStream::ReadDWordBE(void)
{	
	uint16 result1 = ReadWordBE();
	uint16 result2 = ReadWordBE();
	
	return (uint32)((result1) | (result2 << 16));
}


uint32
TFileStream::ReadDWordLE(void)
{
	uint32 result;
	if (fread(&result, sizeof(result), 1, fFileHandle) != sizeof(result))
		throw -1;
	return result;
}


uint32
TFileStream::Size()
{
	int32 oldpos = Position();
	int32 fileSize;
	
	Seek(0L, SEEK_END);
	fileSize = Position();
	Seek(oldpos, SEEK_SET);

	return fileSize;
}
