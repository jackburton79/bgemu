#ifndef __FILE_H
#define __FILE_H

using namespace std;

#include <cstdio>
#include <cstdlib>

#include <string>

#include "Stream.h"
#include "SupportDefs.h"


#define ACCESS_MODE_MASK	0x03
#define FLAGS_MASK			0xFFF8

class FileStream : public Stream {
public:
	enum openmode {
		READ_ONLY	=	1 << 0,
		WRITE_ONLY	=	1 << 1,
		READ_WRITE	=	READ_ONLY | WRITE_ONLY,
		
		CREATE		=	1 << 3,
		IGNORE_CASE =	1 << 5
	};


	FileStream(const char *filename,
				int mode = READ_ONLY);
	FileStream();

	virtual ~FileStream();
	
	int SetTo(const char *filename,
				int mode = READ_ONLY);
	bool IsValid() const;

	virtual off_t Seek(off_t where, int whence);
	virtual off_t Position() const;
	
	size_t Size() const;
	bool EoF();
	
	virtual ssize_t ReadAt(off_t pos, void *dst, size_t count);
	virtual ssize_t Read(void *dst, size_t count);
	
	virtual ssize_t Write(const void *src, size_t count);
	
	// Use these only if you need to swap endianess, because they are
	// not efficient
	// TODO: Move to Stream
	uint8 ReadByte();
	uint16 ReadWordLE();
	uint16 ReadWordBE();
	uint32 ReadDWordLE();
	uint32 ReadDWordBE();
	
private:
	FILE *fFileHandle;
};


#endif
