/*
 * StringStream.h
 *
 *  Created on: 19/mag/2012
 *      Author: Stefano Ceccherini
 */

#ifndef STRINGSTREAM_H_
#define STRINGSTREAM_H_

#include "MemoryStream.h"

#include <string>

class StringStream: public MemoryStream {
public:
	StringStream(const char* string);
	StringStream(std::string string);
	virtual ~StringStream();
};

#endif /* STRINGSTREAM_H_ */
