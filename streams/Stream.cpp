#include "Stream.h"

#include <iostream>


Stream::Stream()
{
}


Stream::~Stream()
{
}

	
ssize_t
Stream::Read(void *dst, size_t size)
{
	off_t curPos = Position();
	ssize_t read = ReadAt(curPos, dst, size);
	if (read > 0)
		Seek(read, SEEK_CUR);
	return read;
}


char*
Stream::ReadLine(char *buffer, size_t maxSize, char endLine)
{
	maxSize--;

	char *ptr = buffer;
	try {
		while ((*ptr = ReadByte()) != endLine
				&& (size_t)(ptr - buffer) < maxSize) {
			ptr++;
		}
	} catch (...) {
		// eof
	}

	if (ptr > buffer) {
		*(ptr - 1) = '\0';
		return buffer;
	}

	return NULL;
}


ssize_t
Stream::ReadString(char *string, size_t size)
{
	char *ptr = string;
	char c;
	for (;;) {
		c = ReadByte();
		if (c == ' ' || c == '\n' || c == '\r')
			break;
		*ptr++ = c;
	}
	*ptr = '\0';

	return ptr - string;
}


ssize_t
Stream::Write(const void *src, size_t size)
{
	off_t curPos = Position();
	ssize_t wrote = WriteAt(curPos, src, size);
	if (wrote > 0)
		Seek(wrote, SEEK_CUR);
	return wrote;
}


uint8
Stream::ReadByte()
{
	uint8 byte;
	if (Read(&byte, sizeof(byte)) != sizeof(byte))
		throw -1;
	return byte;
}


void
Stream::Dump()
{
	off_t oldPos = Position();
	Seek(0, SEEK_SET);
	ssize_t read;
	char buffer[1024];
	while ((read = Read(buffer, 1024)) > 0) {
		if (read < 1024)
			buffer[read] = '\0';
		std::cout << buffer;
	}

	Seek(oldPos, SEEK_SET);
}


void
Stream::DumpToFile(const char *fileName)
{
	FILE *file = fopen(fileName, "wb");
	if (file) {
		off_t oldPos = Position();
		Seek(0, SEEK_SET);
		ssize_t read;
		char buffer[1024];
		while ((read = Read(buffer, 1024)) > 0)
			fwrite(buffer, read, 1, file);
		fclose(file);
		Seek(oldPos, SEEK_SET);
	}
	
}
