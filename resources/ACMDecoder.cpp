/*
 * ACMDecoder.cpp - see ACMDecoder.h
 */

#include "ACMDecoder.h"

#include <cstring>

// Interplay ACM signature.
static const uint32 kACMSignature = 0x01032897;
static const size_t kWAVCHeaderSize = 28;

// Base-4, base-8 and base-11 digit tables of the triplet/pair fillers.
static const char kTable1[27] = {
	0, 1, 2, 4, 5, 6, 8, 9, 10, 16, 17, 18, 20, 21, 22, 24, 25, 26, 32, 33,
	34, 36, 37, 38, 40, 41, 42
};

static const short kTable2[125] = {
	0, 1, 2, 3, 4, 8, 9, 10, 11, 12, 16, 17, 18, 19, 20, 24, 25, 26, 27, 28,
	32, 33, 34, 35, 36, 64, 65, 66, 67, 68, 72, 73, 74, 75, 76, 80, 81, 82,
	83, 84, 88, 89, 90, 91, 92, 96, 97, 98, 99, 100, 128, 129, 130, 131, 132,
	136, 137, 138, 139, 140, 144, 145, 146, 147, 148, 152, 153, 154, 155, 156,
	160, 161, 162, 163, 164, 192, 193, 194, 195, 196, 200, 201, 202, 203, 204,
	208, 209, 210, 211, 212, 216, 217, 218, 219, 220, 224, 225, 226, 227, 228,
	256, 257, 258, 259, 260, 264, 265, 266, 267, 268, 272, 273, 274, 275, 276,
	280, 281, 282, 283, 284, 288, 289, 290, 291, 292
};

static const uint8 kTable3[121] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x10,
	0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x20, 0x21,
	0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x30, 0x31, 0x32,
	0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x40, 0x41, 0x42, 0x43,
	0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x50, 0x51, 0x52, 0x53, 0x54,
	0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65,
	0x66, 0x67, 0x68, 0x69, 0x6A, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
	0x77, 0x78, 0x79, 0x7A, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8A, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
	0x99, 0x9A, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9,
	0xAA
};

// Which filler a column's 5 bit selector picks; 0 = unusable (a damaged
// stream), otherwise the kind passed to ACMDecoder::_Fill().
enum FillKind {
	FILL_INVALID = 0,
	FILL_ZERO,
	FILL_LINEAR,
	FILL_K1_3, FILL_K1_2, FILL_T1_5,
	FILL_K2_4, FILL_K2_3, FILL_T2_7,
	FILL_K3_5, FILL_K3_4,
	FILL_K4_5, FILL_K4_4,
	FILL_T3_7
};

static const uint8 kFillers[32] = {
	FILL_ZERO, FILL_INVALID, FILL_INVALID,
	FILL_LINEAR, FILL_LINEAR, FILL_LINEAR, FILL_LINEAR, FILL_LINEAR, FILL_LINEAR,
	FILL_LINEAR, FILL_LINEAR, FILL_LINEAR, FILL_LINEAR, FILL_LINEAR, FILL_LINEAR,
	FILL_LINEAR, FILL_LINEAR,
	FILL_K1_3, FILL_K1_2, FILL_T1_5, FILL_K2_4, FILL_K2_3, FILL_T2_7,
	FILL_K3_5, FILL_K3_4, FILL_INVALID, FILL_K4_5, FILL_K4_4, FILL_INVALID,
	FILL_T3_7, FILL_INVALID, FILL_INVALID
};


ACMDecoder::ACMDecoder()
	:
	fData(nullptr),
	fSize(0),
	fPos(0),
	fChannels(0),
	fSampleRate(0),
	fTotalSamples(0),
	fSamplesLeft(0),
	fLevels(0),
	fSubblocks(0),
	fBlockSize(0),
	fSubbandSize(0),
	fNextBits(0),
	fAvailBits(0),
	fBlockPos(0),
	fSamplesReady(0)
{
}


static uint32
_ReadLE(const uint8* p, int bytes)
{
	uint32 value = 0;
	for (int i = bytes - 1; i >= 0; i--)
		value = (value << 8) | p[i];
	return value;
}


bool
ACMDecoder::Open(const uint8* data, size_t size)
{
	fData = data;
	fSize = size;
	fPos = 0;

	if (size >= 4 && ::memcmp(data, "WAVC", 4) == 0)
		fPos = kWAVCHeaderSize;

	// signature (4), samples (4), channels (2), rate (2), levels/subblocks (2)
	if (size < fPos + 14)
		return false;
	const uint8* header = data + fPos;
	if (_ReadLE(header, 4) != kACMSignature)
		return false;

	fTotalSamples = _ReadLE(header + 4, 4);
	fChannels = (uint16)_ReadLE(header + 8, 2);
	fSampleRate = _ReadLE(header + 10, 2);
	const uint32 packed = _ReadLE(header + 12, 2);
	fSubblocks = (int)(packed >> 4);
	fLevels = (int)(packed & 15);
	fPos += 14;

	if (fChannels == 0 || fSubblocks == 0)
		return false;

	fSamplesLeft = fTotalSamples;
	fBlockSize = (1 << fLevels) * fSubblocks;
	fSubbandSize = 1 << fLevels;
	fBlock.assign(fBlockSize, 0);
	fAmplitudes.assign(0x10000, 0);
	// The first stage of the subband transform keeps two 16 bit values per
	// column, the others two ints.
	fFirstMemory.assign(fLevels == 0 ? 0 : fSubbandSize, 0);
	fMemory.assign(fLevels == 0 ? 0 : fSubbandSize - 2, 0);
	fNextBits = 0;
	fAvailBits = 0;
	fBlockPos = 0;
	fSamplesReady = 0;
	return true;
}


uint16
ACMDecoder::Channels() const
{
	return fChannels;
}


uint32
ACMDecoder::SampleRate() const
{
	return fSampleRate;
}


uint32
ACMDecoder::TotalSamples() const
{
	return fTotalSamples;
}


size_t
ACMDecoder::Read(int16* buffer, size_t count)
{
	size_t done = 0;
	while (done < count) {
		if (fSamplesReady == 0) {
			if (fSamplesLeft == 0)
				break;
			if (!_UnpackBlock())
				break;
			_SubbandDecode();
			fBlockPos = 0;
			fSamplesReady = (uint32)fBlockSize > fSamplesLeft ? fSamplesLeft : (uint32)fBlockSize;
			fSamplesLeft -= fSamplesReady;
		}
		buffer[done++] = (int16)(fBlock[fBlockPos++] >> fLevels);
		fSamplesReady--;
	}
	return done;
}


bool
ACMDecoder::DecodeAll(std::vector<uint8>& pcm)
{
	std::vector<int16> samples(fTotalSamples);
	const size_t got = Read(samples.data(), samples.size());
	if (got == 0 && fTotalSamples != 0)
		return false;
	for (size_t i = 0; i < got; i++) {
		pcm.push_back((uint8)(samples[i] & 0xff));
		pcm.push_back((uint8)((samples[i] >> 8) & 0xff));
	}
	return true;
}


// Bits come least significant first, out of bytes read in order; the end of
// the data reads as zeros.
void
ACMDecoder::_PrepareBits(int bits)
{
	while (bits > fAvailBits) {
		uint8 byte = 0;
		if (fPos < fSize)
			byte = fData[fPos++];
		fNextBits |= (uint32)byte << fAvailBits;
		fAvailBits += 8;
	}
}


int
ACMDecoder::_GetBits(int bits)
{
	_PrepareBits(bits);
	int result = (int)fNextBits;
	fAvailBits -= bits;
	fNextBits >>= bits;
	return result;
}


void
ACMDecoder::_Set(int pass, int row, int value)
{
	fBlock[row * fSubbandSize + pass] = value;
}


bool
ACMDecoder::_UnpackBlock()
{
	int16* middle = fAmplitudes.data() + 0x8000;

	const int power = _GetBits(4) & 0xF;
	const int value = _GetBits(16) & 0xFFFF;
	const int count = 1 << power;
	int v = 0;
	for (int i = 0; i < count; i++) {
		middle[i] = (int16)v;
		v += value;
	}
	v = -value;
	for (int i = 0; i < count; i++) {
		middle[-i - 1] = (int16)v;
		v -= value;
	}

	for (int pass = 0; pass < fSubbandSize; pass++) {
		const int selector = _GetBits(5) & 0x1F;
		if (!_Fill(pass, selector))
			return false;
	}
	return true;
}


// Fills column `pass` of the block. `selector` is the 5 bit code read from the
// stream: it picks the filler (kFillers) and, for the linear one, the number of
// bits per value.
bool
ACMDecoder::_Fill(int pass, int selector)
{
	const int16* middle = fAmplitudes.data() + 0x8000;
	const int rows = fSubblocks;

	switch (kFillers[selector]) {
		case FILL_INVALID:
			return false;

		case FILL_ZERO:
			for (int i = 0; i < rows; i++)
				_Set(pass, i, 0);
			return true;

		case FILL_LINEAR: {
			const int mask = (1 << selector) - 1;
			const int16* base = middle - (1 << (selector - 1));
			for (int i = 0; i < rows; i++)
				_Set(pass, i, base[_GetBits(selector) & mask]);
			return true;
		}

		case FILL_K1_3:
			// 0, +-1; runs of zeros are frequent.
			for (int i = 0; i < rows; i++) {
				_PrepareBits(3);
				if ((fNextBits & 1) == 0) {
					fAvailBits--;
					fNextBits >>= 1;
					_Set(pass, i, 0);
					if ((++i) == rows)
						break;
					_Set(pass, i, 0);
				} else if ((fNextBits & 2) == 0) {
					fAvailBits -= 2;
					fNextBits >>= 2;
					_Set(pass, i, 0);
				} else {
					_Set(pass, i, (fNextBits & 4) ? middle[1] : middle[-1]);
					fAvailBits -= 3;
					fNextBits >>= 3;
				}
			}
			return true;

		case FILL_K1_2:
			// 0, +-1.
			for (int i = 0; i < rows; i++) {
				_PrepareBits(2);
				if ((fNextBits & 1) == 0) {
					fAvailBits--;
					fNextBits >>= 1;
					_Set(pass, i, 0);
				} else {
					_Set(pass, i, (fNextBits & 2) ? middle[1] : middle[-1]);
					fAvailBits -= 2;
					fNextBits >>= 2;
				}
			}
			return true;

		case FILL_T1_5:
			// All the triplets of -1, 0, +1.
			for (int i = 0; i < rows; i++) {
				int bits = kTable1[_GetBits(5) & 0x1f];
				_Set(pass, i, middle[-1 + (bits & 3)]);
				if ((++i) == rows)
					break;
				bits >>= 2;
				_Set(pass, i, middle[-1 + (bits & 3)]);
				if ((++i) == rows)
					break;
				bits >>= 2;
				_Set(pass, i, middle[-1 + bits]);
			}
			return true;

		case FILL_K2_4:
			// 0, +-1, +-2; runs of zeros are frequent.
			for (int i = 0; i < rows; i++) {
				_PrepareBits(4);
				if ((fNextBits & 1) == 0) {
					fAvailBits--;
					fNextBits >>= 1;
					_Set(pass, i, 0);
					if ((++i) == rows)
						break;
					_Set(pass, i, 0);
				} else if ((fNextBits & 2) == 0) {
					fAvailBits -= 2;
					fNextBits >>= 2;
					_Set(pass, i, 0);
				} else {
					_Set(pass, i, (fNextBits & 8)
						? ((fNextBits & 4) ? middle[2] : middle[1])
						: ((fNextBits & 4) ? middle[-1] : middle[-2]));
					fAvailBits -= 4;
					fNextBits >>= 4;
				}
			}
			return true;

		case FILL_K2_3:
			// 0, +-1, +-2.
			for (int i = 0; i < rows; i++) {
				_PrepareBits(3);
				if ((fNextBits & 1) == 0) {
					fAvailBits--;
					fNextBits >>= 1;
					_Set(pass, i, 0);
				} else {
					_Set(pass, i, (fNextBits & 4)
						? ((fNextBits & 2) ? middle[2] : middle[1])
						: ((fNextBits & 2) ? middle[-1] : middle[-2]));
					fAvailBits -= 3;
					fNextBits >>= 3;
				}
			}
			return true;

		case FILL_T2_7:
			// All the triplets of -2..+2.
			for (int i = 0; i < rows; i++) {
				short val = kTable2[_GetBits(7) & 0x7f];
				_Set(pass, i, middle[-2 + (val & 7)]);
				if ((++i) == rows)
					break;
				val >>= 3;
				_Set(pass, i, middle[-2 + (val & 7)]);
				if ((++i) == rows)
					break;
				val >>= 3;
				_Set(pass, i, middle[-2 + val]);
			}
			return true;

		case FILL_K3_5:
			// -3..+3, runs of zeros are frequent.
			for (int i = 0; i < rows; i++) {
				_PrepareBits(5);
				if ((fNextBits & 1) == 0) {
					fAvailBits--;
					fNextBits >>= 1;
					_Set(pass, i, 0);
					if ((++i) == rows)
						break;
					_Set(pass, i, 0);
				} else if ((fNextBits & 2) == 0) {
					fAvailBits -= 2;
					fNextBits >>= 2;
					_Set(pass, i, 0);
				} else if ((fNextBits & 4) == 0) {
					_Set(pass, i, (fNextBits & 8) ? middle[1] : middle[-1]);
					fAvailBits -= 4;
					fNextBits >>= 4;
				} else {
					fAvailBits -= 5;
					int val = (fNextBits & 0x18) >> 3;
					fNextBits >>= 5;
					if (val >= 2)
						val += 3;
					_Set(pass, i, middle[-3 + val]);
				}
			}
			return true;

		case FILL_K3_4:
			// -3..+3.
			for (int i = 0; i < rows; i++) {
				_PrepareBits(4);
				if ((fNextBits & 1) == 0) {
					fAvailBits--;
					fNextBits >>= 1;
					_Set(pass, i, 0);
				} else if ((fNextBits & 2) == 0) {
					fAvailBits -= 3;
					_Set(pass, i, (fNextBits & 4) ? middle[1] : middle[-1]);
					fNextBits >>= 3;
				} else {
					int val = (fNextBits & 0xC) >> 2;
					fAvailBits -= 4;
					fNextBits >>= 4;
					if (val >= 2)
						val += 3;
					_Set(pass, i, middle[-3 + val]);
				}
			}
			return true;

		case FILL_K4_5:
			// -4..+4, runs of zeros are frequent.
			for (int i = 0; i < rows; i++) {
				_PrepareBits(5);
				if ((fNextBits & 1) == 0) {
					fAvailBits--;
					fNextBits >>= 1;
					_Set(pass, i, 0);
					if ((++i) == rows)
						break;
					_Set(pass, i, 0);
				} else if ((fNextBits & 2) == 0) {
					fAvailBits -= 2;
					fNextBits >>= 2;
					_Set(pass, i, 0);
				} else {
					int val = (fNextBits & 0x1C) >> 2;
					if (val >= 4)
						val++;
					_Set(pass, i, middle[-4 + val]);
					fAvailBits -= 5;
					fNextBits >>= 5;
				}
			}
			return true;

		case FILL_K4_4:
			// -4..+4.
			for (int i = 0; i < rows; i++) {
				_PrepareBits(4);
				if ((fNextBits & 1) == 0) {
					fAvailBits--;
					fNextBits >>= 1;
					_Set(pass, i, 0);
				} else {
					int val = (fNextBits & 0xE) >> 1;
					fAvailBits -= 4;
					fNextBits >>= 4;
					if (val >= 4)
						val++;
					_Set(pass, i, middle[-4 + val]);
				}
			}
			return true;

		case FILL_T3_7:
			// All the pairs of -5..+5.
			for (int i = 0; i < rows; i++) {
				uint8 val = kTable3[_GetBits(7) & 0x7f];
				_Set(pass, i, middle[-5 + (val & 0xF)]);
				if ((++i) == rows)
					break;
				val >>= 4;
				_Set(pass, i, middle[-5 + val]);
			}
			return true;
	}
	return false;
}


// The inverse subband transform of a block, in place.
void
ACMDecoder::_SubbandDecode()
{
	if (fLevels == 0)
		return;

	int* buffer = fBlock.data();
	int* memory = fMemory.data();
	int sbSize = fSubbandSize >> 1;
	int blocks = fSubblocks << 1;

	_Step4d3fcc(fFirstMemory.data(), buffer, sbSize, blocks);

	for (int i = 0; i < blocks; i++)
		buffer[i * sbSize]++;

	sbSize >>= 1;
	blocks <<= 1;

	while (sbSize != 0) {
		_Step4d420c(memory, buffer, sbSize, blocks);
		memory += sbSize << 1;
		sbSize >>= 1;
		blocks <<= 1;
	}
}


void
ACMDecoder::_Step4d3fcc(int16* memory, int* buffer, int sbSize, int blocks)
{
	int row0, row1, row2 = 0, row3 = 0, db0, db1;
	const int sbSize2 = sbSize * 2, sbSize3 = sbSize * 3;

	if (blocks == 2) {
		for (int i = 0; i < sbSize; i++) {
			row0 = buffer[0];
			row1 = buffer[sbSize];
			buffer[0] = buffer[0] + memory[0] + 2 * memory[1];
			buffer[sbSize] = 2 * row0 - memory[1] - buffer[sbSize];
			memory[0] = (int16)row0;
			memory[1] = (int16)row1;

			memory += 2;
			buffer++;
		}
	} else if (blocks == 4) {
		for (int i = 0; i < sbSize; i++) {
			row0 = buffer[0];
			row1 = buffer[sbSize];
			row2 = buffer[sbSize2];
			row3 = buffer[sbSize3];

			buffer[0] = memory[0] + 2 * memory[1] + row0;
			buffer[sbSize] = -memory[1] + 2 * row0 - row1;
			buffer[sbSize2] = row0 + 2 * row1 + row2;
			buffer[sbSize3] = -row1 + 2 * row2 - row3;

			memory[0] = (int16)row2;
			memory[1] = (int16)row3;

			memory += 2;
			buffer++;
		}
	} else {
		for (int i = 0; i < sbSize; i++) {
			int* ptr = buffer;
			if ((blocks >> 1) & 1) {
				row0 = ptr[0];
				row1 = ptr[sbSize];

				ptr[0] = memory[0] + 2 * memory[1] + row0;
				ptr[sbSize] = -memory[1] + 2 * row0 - row1;
				ptr += sbSize2;

				db0 = row0;
				db1 = row1;
			} else {
				db0 = memory[0];
				db1 = memory[1];
			}

			for (int j = 0; j < blocks >> 2; j++) {
				row0 = ptr[0];  ptr[0] = db0 + 2 * db1 + row0;  ptr += sbSize;
				row1 = ptr[0];  ptr[0] = -db1 + 2 * row0 - row1;  ptr += sbSize;
				row2 = ptr[0];  ptr[0] = row0 + 2 * row1 + row2;  ptr += sbSize;
				row3 = ptr[0];  ptr[0] = -row1 + 2 * row2 - row3;  ptr += sbSize;

				db0 = row2;
				db1 = row3;
			}
			memory[0] = (int16)row2;
			memory[1] = (int16)row3;
			memory += 2;
			buffer++;
		}
	}
}


void
ACMDecoder::_Step4d420c(int* memory, int* buffer, int sbSize, int blocks)
{
	int row0, row1, row2 = 0, row3 = 0, db0, db1;
	const int sbSize2 = sbSize * 2, sbSize3 = sbSize * 3;

	if (blocks == 4) {
		for (int i = 0; i < sbSize; i++) {
			row0 = buffer[0];
			row1 = buffer[sbSize];
			row2 = buffer[sbSize2];
			row3 = buffer[sbSize3];

			buffer[0] = memory[0] + 2 * memory[1] + row0;
			buffer[sbSize] = -memory[1] + 2 * row0 - row1;
			buffer[sbSize2] = row0 + 2 * row1 + row2;
			buffer[sbSize3] = -row1 + 2 * row2 - row3;

			memory[0] = row2;
			memory[1] = row3;

			memory += 2;
			buffer++;
		}
	} else {
		for (int i = 0; i < sbSize; i++) {
			int* ptr = buffer;
			db0 = memory[0];
			db1 = memory[1];
			for (int j = 0; j < blocks >> 2; j++) {
				row0 = ptr[0];  ptr[0] = db0 + 2 * db1 + row0;  ptr += sbSize;
				row1 = ptr[0];  ptr[0] = -db1 + 2 * row0 - row1;  ptr += sbSize;
				row2 = ptr[0];  ptr[0] = row0 + 2 * row1 + row2;  ptr += sbSize;
				row3 = ptr[0];  ptr[0] = -row1 + 2 * row2 - row3;  ptr += sbSize;

				db0 = row2;
				db1 = row3;
			}
			memory[0] = row2;
			memory[1] = row3;

			memory += 2;
			buffer++;
		}
	}
}
