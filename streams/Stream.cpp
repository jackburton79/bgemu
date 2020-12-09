#include "Stream.h"

#include <stdexcept>
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
	} catch (std::exception& e) {
		// eof
	}

	if (ptr > buffer) {
		*(ptr - 1) = '\0';
		return buffer;
	}

	return NULL;
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
		throw std::runtime_error("Stream::ReadByte(): tried to read uint8 but failed!");
	return byte;
}


/* virtual */
Stream*
Stream::Clone() const
{
	std::cerr << "Stream::Clone() not implemented!";
	throw std::runtime_error("Stream::Clone() not implemented!");
	return NULL;
}


/* virtual */
Stream*
Stream::Adopt()
{
	std::cerr << "Stream::Adopt() not implemented!";
	throw std::runtime_error("Stream::Adopt() not implemented!");
	return NULL;
}


void
Stream::Dump()
{
	off_t oldPos = Position();
	Seek(0, SEEK_SET);
	uint8 byte;
	while (true) {
		byte = ReadByte();
		std::cout << byte;
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
