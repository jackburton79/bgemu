/*
 * EncryptionKey.cpp
 *
 *  Created on: 23/dec/2020
 *      Author: Stefano Ceccherini
 */
#include "EncryptionKey.h"

const size_t kEncryptionKeySize = 64;

const uint8 kEncryptionKey[kEncryptionKeySize] = {
	0x88, 0xa8, 0x8f, 0xba, 0x8a, 0xd3, 0xb9, 0xf5,
	0xed, 0xb1, 0xcf, 0xea, 0xaa, 0xe4, 0xb5, 0xfb,
	0xeb, 0x82, 0xf9, 0x90, 0xca, 0xc9, 0xb5, 0xe7,
	0xdc, 0x8e, 0xb7, 0xac, 0xee, 0xf7, 0xe0, 0xca,
	0x8e, 0xea, 0xca, 0x80, 0xce, 0xc5, 0xad, 0xb7,
	0xc4, 0xd0, 0x84, 0x93, 0xd5, 0xf0, 0xeb, 0xc8,
	0xb4, 0x9d, 0xcc, 0xaf, 0xa5, 0x95, 0xba, 0x99,
	0x87, 0xd2, 0x9d, 0xe3, 0x91, 0xba, 0x90, 0xca
};

