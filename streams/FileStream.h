#ifndef __FILE_H
#define __FILE_H

using namespace std;

#include <cstdio>
#include <cstdlib>

#include <string>

#include "Stream.h"
#include "SupportDefs.h"


class FileStream : public Stream {
public:
	enum openmode {
		READ_ONLY = 0,
		WRITE_ONLY = 1,
		READ_WRITE = 2,
		CREATE = 4
	};

	enum casemode {
		CASE_SENSITIVE = 0,
		CASE_INSENSITIVE = 1
	};

	FileStream(const char *filename,
				openmode mode = READ_ONLY,
				casemode caseMode = CASE_SENSITIVE);
	FileStream();

	virtual ~FileStream();
	
	bool SetTo(const char *filename,
				openmode mode = READ_ONLY,
				casemode caseMode = CASE_SENSITIVE);
	bool IsValid() const;

	virtual int32 Seek(int32 where, int whence);
	virtual int32 Position() const;
	
	uint32 Size() const;
	bool EoF();
	
	virtual ssize_t ReadAt(int pos, void *dst, int count);
	virtual ssize_t Read(void *dst, int count);
	
	virtual ssize_t Write(const void *src, int count);
	
	// Use these only if you need to swap endianess
	uint8 ReadByte();
	uint16 ReadWordLE();
	uint16 ReadWordBE();
	uint32 ReadDWordLE();
	uint32 ReadDWordBE();
	
private:
	FILE *fFileHandle;
};


#endif
