/*
 * ZLibDecompressor.cpp
 *
 *  Created on: 19/nov/2014
 *      Author: stefano
 */

#include "ZLibDecompressor.h"

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
		const size_t& inputSize, void* outputBuffer,
		size_t& outputSize)
{
	int status = uncompress((Bytef*)outputBuffer, (uLongf*)&outputSize,
					(const Bytef*)inputBuffer, (uLong)inputSize);

	if (status != Z_OK)
		return -1;

	return 0;
}
