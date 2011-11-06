#include "Stream.h"

TStream::TStream()
{
}


TStream::~TStream()
{
}

	
ssize_t
TStream::Read(void *dst, int size)
{
	int32 curPos = Position();
	ssize_t read = ReadAt(curPos, dst, size);
	if (read > 0)
		Seek(read, SEEK_CUR);
	return read;
}


ssize_t
TStream::Write(void *src, int size)
{
	int32 curPos = Position();
	ssize_t wrote = WriteAt(curPos, src, size);
	if (wrote > 0)
		Seek(wrote, SEEK_CUR);
	return wrote;
}


uint8
TStream::ReadByte()
{
	uint8 byte;
	Read(&byte, sizeof(byte));
	return byte;
}

	
void
TStream::DumpToFile(const char *fileName)
{
	FILE *file = fopen(fileName, "wb");
	if (file) {
		int32 oldPos = Position();
		Seek(0, SEEK_SET);
		ssize_t read;
		char buffer[1024];
		while ((read = Read(buffer, 1024)) > 0)
			fwrite(buffer, 1024, 1, file);
		fclose(file);
		Seek(oldPos, SEEK_SET);
	}
	
}
