/*
 * ZLibDecompressor.cpp
 *
 *  Created on: 19/nov/2014
 *      Author: Stefano Ceccherini
 */

#include "ZLibDecompressor.h"

#include <iostream>

#include <zlib.h>


ZLibDecompressor::ZLibDecompressor()
{
}


ZLibDecompressor::~ZLibDecompressor()
{
}


/* static */
status_t
ZLibDecompressor::DecompressBuffer(const void* inputBuffer,
		const uint32& inputSize, void* outputBuffer,
		uint32& outputSize)
{
	int status = uncompress((Bytef*)outputBuffer, (uLongf*)&outputSize,
					(const Bytef*)inputBuffer, (uLong)inputSize);

	if (status != Z_OK) {
		std::cerr << "ZLibDecompressor::DecompressBuffer(): ";
		std::cerr << zError(status);
		return -1;
	}

	return 0;
}
