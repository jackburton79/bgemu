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

	virtual ssize_t ReadAt(off_t pos, void *dst, size_t size);

	virtual off_t Seek(off_t where, int whence);
	virtual off_t Position() const;

	virtual size_t Size() const;

private:
	Stream* fEncryptedStream;
	int fKeySize;
};


#endif /* ENCRYPTEDSTREAM_H_ */
