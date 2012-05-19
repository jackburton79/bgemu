/*
 * StringStream.h
 *
 *  Created on: 19/mag/2012
 *      Author: stefano
 */

#ifndef STRINGSTREAM_H_
#define STRINGSTREAM_H_

#include "MemoryStream.h"

class StringStream: public MemoryStream {
public:
	StringStream(const char* string);
	virtual ~StringStream();
};

#endif /* STRINGSTREAM_H_ */
