#ifndef __STREAM_H
#define __STREAM_H

#include <cstdio>

#include <SupportDefs.h>

class Stream {
public:
	Stream();
	virtual ~Stream();
	
	virtual ssize_t Read(void *dst, int size);
	virtual ssize_t ReadAt(int pos, void *dst, int size);

	char *ReadLine(char *buffer, size_t maxSize, char endLine = '\n');
	ssize_t ReadString(char *string, size_t size);

	virtual ssize_t Write(void *src, int size);
	virtual ssize_t WriteAt(int pos, void *src, int size)
	{
		throw "Stream is not writable!";
	};
	
	virtual int32 Seek(int32 where, int whence)
	{
		throw "Seek() not supported for this stream!";
	};
	
	virtual int32 Position() const 
	{
		throw "Position() not supported for this stream!";
	};
	
	virtual uint32 Size() const
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
		Read(&data, sizeof(data));
		return *this;
	};

	template<typename T>
	ssize_t Read(T& dest) {
		return Read(&dest, sizeof(dest));
	}
	
	template<typename T>
	ssize_t ReadAt(int pos, T &dst) {
		return ReadAt(pos, &dst, sizeof(dst));
	};

	void DumpToFile(const char *fileName);
};


#endif
