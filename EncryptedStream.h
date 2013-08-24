/*
 * EncryptedStream.h
 *
 *  Created on: 23/ago/2013
 *      Author: stefano
 */

#ifndef ENCRYPTEDSTREAM_H_
#define ENCRYPTEDSTREAM_H_

#include "MemoryStream.h"

class EncryptedStream : public MemoryStream {
public:
	EncryptedStream(Stream *stream);
	virtual ssize_t Read(void* dst, int size);

private:
	int fKeySize;
};


#endif /* ENCRYPTEDSTREAM_H_ */
