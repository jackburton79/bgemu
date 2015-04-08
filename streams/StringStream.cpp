/*
 * StringStream.cpp
 *
 *  Created on: 19/mag/2012
 *      Author: stefano
 */

#include "StringStream.h"

#include <string.h>

StringStream::StringStream(const char* string)
	:
	// TODO: Added +1 because Tokenizer::_ReadFullToken() is happy this way,
	// not sure it's a bug there instead
	MemoryStream((const uint8*)string, strlen(string) + 1)
{
}


StringStream::~StringStream()
{
}

