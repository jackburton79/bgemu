#ifndef __STREAM_H
#define __STREAM_H

#include <cstdio>

#include <SupportDefs.h>

class Stream {
public:
	Stream();
	virtual ~Stream();
	
	virtual ssize_t Read(void *dst, size_t size);
	virtual ssize_t ReadAt(off_t pos, void *dst, size_t size) = 0;

	char *ReadLine(char *buffer, size_t maxSize, char endLine = '\n');
	ssize_t ReadString(char *string, size_t size);

	virtual ssize_t Write(const void *src, size_t size);
	virtual ssize_t WriteAt(off_t pos, const void *src, size_t size)
	{
		throw "Stream is not writable!";
	};
	
	virtual off_t Seek(off_t where, int whence)
	{
		throw "Seek() not supported for this stream!";
	};
	
	virtual off_t Position() const 
	{
		throw "Position() not supported for this stream!";
	};
	
	virtual size_t Size() const
	{
		throw "Size() not supported for this stream!";
	}
	
	virtual void *Data() const 
	{
		throw "Data() not supported for this stream!";
	};
	
	virtual uint8 ReadByte();
		
	template<typename T>
	Stream& operator>>(T& data) {
		ssize_t read = Read(&data, sizeof(data));
		if (read < 0 || read != sizeof(data))
			throw "Read() exception";
		return *this;
	};

	template<typename T>
	void Read(T& dest) {
		ssize_t read = Read(&dest, sizeof(dest));
		if (read < 0 || read != sizeof(dest))
			throw "Read() exception";
	}
	
	template<typename T>
	void ReadAt(off_t pos, T &dst) {
		ssize_t read = ReadAt(pos, &dst, sizeof(dst));
		if (read < 0 || read != sizeof(dst))
			throw "ReadAt() exception";
	};

	void Dump();
	void DumpToFile(const char *fileName);
};


#endif
