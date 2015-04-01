#ifndef __MEMORY_H
#define __MEMORY_H

#include "Stream.h"
#include "SupportDefs.h"

class MemoryStream : public Stream {
public:
	MemoryStream(const uint8 *data, size_t size, bool owns = false);
	MemoryStream(size_t size); // Allocates a block of memory
	virtual ~MemoryStream();
	
	virtual ssize_t Read(void *dst, size_t size);
	virtual ssize_t ReadAt(off_t pos, void *dst, size_t size);
	virtual ssize_t WriteAt(off_t pos, const void *src, size_t size);
	
	virtual off_t Seek(off_t where, int whence);
	virtual off_t Position() const;
	
	virtual size_t Size() const;
	
	virtual void *Data() const;
	
	virtual MemoryStream* Clone() const;

public:
	uint8 *fData;
	size_t fSize;
	off_t fPosition;
	bool fOwnsBuffer; 
};

#endif
