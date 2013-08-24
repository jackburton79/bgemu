/*
 * EncryptedStream.h
 *
 *  Created on: 23/ago/2013
 *      Author: stefano
 */

#ifndef ENCRYPTEDSTREAM_H_
#define ENCRYPTEDSTREAM_H_

#include "Stream.h"

class EncryptedStream : public Stream {
public:
	EncryptedStream(Stream *stream);
	virtual ~EncryptedStream();

	virtual ssize_t Read(void* dst, int size);
	virtual ssize_t ReadAt(int pos, void *dst, int size);

	virtual int32 Seek(int32 where, int whence);
	virtual int32 Position() const;

	virtual uint32 Size() const;

private:
	Stream* fEncryptedStream;
	int fKeySize;
};


#endif /* ENCRYPTEDSTREAM_H_ */
