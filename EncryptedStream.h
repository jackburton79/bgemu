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
	virtual uint8 ReadByte();

private:
	int fKeySize;
};


#endif /* ENCRYPTEDSTREAM_H_ */
