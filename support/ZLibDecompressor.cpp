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
		const size_t& inputSize, void* outputBuffer,
		size_t& outputSize)
{
	int status = uncompress(static_cast<Bytef*>(outputBuffer), static_cast<uLongf*>(&outputSize),
							static_cast<const Bytef*>(inputBuffer), static_cast<uLong>(inputSize));

	if (status != Z_OK) {
		std::cerr << "ZLibDecompressor::DecompressBuffer(): ";
		std::cerr << zError(status);
		return -1;
	}

	return 0;
}
