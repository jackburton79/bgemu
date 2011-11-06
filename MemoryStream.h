#ifndef __MEMORY_H
#define __MEMORY_H

#include "Stream.h"
#include "SupportDefs.h"

class TMemoryStream : public TStream {
public:
	TMemoryStream(const uint8 *data, int size, bool owns = false);
	TMemoryStream(int size); // Allocates a block of memory
	
	virtual ~TMemoryStream();
	
	virtual ssize_t ReadAt(int pos, void *dst, int size);
	virtual ssize_t WriteAt(int pos, void *src, int size);
	
	virtual int32 Seek(int32 where, int whence = SEEK_CUR);
	virtual int32 Position() const;
	
	virtual uint32 Size() const;
	
	virtual void *Data() const;
	
public:
	uint8 *fData;
	int fSize;
	int32 fPosition;
	bool fOwnsBuffer; 
};

#endif
