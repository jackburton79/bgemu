#include "GraphicsEngine.h"
#include "MemoryStream.h"
#include "MovieDecoder.h"

#include <iomanip>

static void
DumpData(const uint8 *data, int size)
{
	for (int i = 0; i < size; i++) {
		std::cout << std::setw(2) << std::setfill('0');
		std::cout << (int)(*(data + i)) << " ";
		if ((i % 8) == 7)
			std::cout << std::endl;
	}
	std::cout << std::endl;
}


int
MovieDecoder::Test()
{
	try {
		TestOpcode7A();
		TestOpcode7B();
		//TestOpcode8A();
		//TestOpcode8B();
		//TestOpcode8C();
		//TestOpcode9A();
		//TestOpcode9B();
		TestOpcodeA2();
		TestOpcodeB();
		TestOpcodeC();
		TestOpcodeD();
		TestOpcodeE();
		TestOpcodeF();
	} catch (std::exception& e) {
		std::cerr << "MovieDecoder::Test(): " << e.what() << std::endl;
		return -1;
	} catch (...) {
		std::cerr << "MovieDecoder::Test(): FAILURE!" << std::endl;
		return -1;
	}

	return 0;
}


#if DEBUG
void
MovieDecoder::TestInit(uint8 opcode, const uint8 data[], uint32 size, const char* name)
{
	std::cout << "MovieDecoder::TestInit(opcode 0x" << std::hex;
	std::cout << (int)opcode;
	if (name != NULL)
		std::cout << " (" << name << ") ";
	std::cout << ")" << std::endl;

	AllocateBuffer(640, 480, 99, false);

	uint8 *decodingMap = new uint8[1];
	decodingMap[0] = opcode;
	SetDecodingMap(decodingMap, 1);

	MemoryStream stream(data, size);
	DecodeDataBlock(&stream, stream.Size());
}


void
MovieDecoder::TestFinish(const uint8 data[], uint32 dataSize)
{
	int result = memcmp(data, fScratchBuffer->Pixels(), dataSize);
	std::cout << "MovieDecoder::TestFinish(opcode 0x" << std::hex;
	std::cout << (int)(fDecodingMap[0] & 0xF) << "): ";
	std::cout << (result ? "FAILURE" : "OK") << std::endl;

	if (result) {
		std::cout << "Data is:" << std::endl;
		DumpData((uint8*)fScratchBuffer->Pixels(), 64);
		std::cout << "should be:" << std::endl;
		DumpData(data, dataSize);
		throw std::runtime_error("TestFinish error!");
	}

	fNewFrame->Release();
	fCurrentFrame->Release();
	fNewFrame = NULL;
	fCurrentFrame = NULL;
}


void
MovieDecoder::TestOpcode7A()
{
	// Test opcode 0x7:
	const uint8 data[] = {
			0x11, 0x22, 0xff, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xff }; // opcode7, first case

	TestInit(0x7, data, sizeof(data), "P0 <= P1");

	const uint8 resultData[] = { // opcode 7, first case
			0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
			0x22, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x22,
			0x22, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x22,
			0x22, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x22,
			0x22, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x22,
			0x22, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x22,
			0x22, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x22,
			0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22
	};

	TestFinish(resultData, sizeof(resultData));
}


void
MovieDecoder::TestOpcode7B()
{
	// Test opcode 0x7:
	const uint8 data[] = { 	0x22, 0x11, 0xff, 0x81 }; // opcode7, second case

	TestInit(0x7, data, sizeof(data), "P0 > P1 ");

	const uint8 resultData[] = { // opcode 7, second case
			0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
			0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
			0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
			0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
			0x11, 0x11, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
			0x11, 0x11, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
			0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x11, 0x11,
			0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x11, 0x11,
	};

	TestFinish(resultData, sizeof(resultData));
}


void
MovieDecoder::TestOpcode8A()
{
	const uint8 data[] = {
			0x00, 0x22, 0xf9, 0x9f, 0x11, 0x33, 0xcc, 0x33,
			0x44, 0x55, 0xaa, 0x55, 0x66, 0x77, 0x01, 0xef
	};

	TestInit(0x8, data, sizeof(data), "P0 <= P1");

	const uint8 result[] = {
			0x22, 0x22, 0x22, 0x22, 0x33, 0x33, 0x11, 0x11,
            0x22, 0x00, 0x00, 0x22, 0x33, 0x33, 0x11, 0x11,
            0x22, 0x00, 0x00, 0x22, 0x11, 0x11, 0x33, 0x33,
            0x22, 0x22, 0x22, 0x22, 0x11, 0x11, 0x33, 0x33,
            0x55, 0x44, 0x55, 0x44, 0x66, 0x66, 0x66, 0x66,
            0x55, 0x44, 0x55, 0x44, 0x66, 0x66, 0x66, 0x77,
            0x44, 0x55, 0x44, 0x55, 0x77, 0x77, 0x77, 0x66,
            0x44, 0x55, 0x44, 0x55, 0x77, 0x77, 0x77, 0x77
	};

	TestFinish(result, sizeof(result));
}


void
MovieDecoder::TestOpcode8B()
{
	const uint8 data[] = {
		0x22, 0x00, 0x01, 0x37, 0xf7, 0x31,
		0x11, 0x66, 0x8c, 0xe6, 0x73, 0x31
	};

	TestInit(0x8, data, sizeof(data), "P0 > P1 && P2 <= P3");

	const uint8 result[] = {
		0x22, 0x22, 0x22, 0x22, 0x66, 0x11, 0x11, 0x11,
		0x22, 0x22, 0x22, 0x00, 0x66, 0x66, 0x11, 0x11,
		0x22, 0x22, 0x00, 0x00, 0x66, 0x66, 0x66, 0x11,
		0x22, 0x00, 0x00, 0x00, 0x11, 0x66, 0x66, 0x11,
		0x00, 0x00, 0x00, 0x00, 0x11, 0x66, 0x66, 0x66,
		0x22, 0x00, 0x00, 0x00, 0x11, 0x11, 0x66, 0x66,
		0x22, 0x22, 0x00, 0x00, 0x11, 0x11, 0x66, 0x66,
		0x22, 0x22, 0x22, 0x00, 0x11, 0x11, 0x11, 0x66
	};

	TestFinish(result, sizeof(result));
}


void
MovieDecoder::TestOpcode8C()
{
	const uint8 data[] = {
		0x22, 0x00, 0xcc, 0x66, 0x33, 0x19,
		0x66, 0x11, 0x18, 0x24, 0x42, 0x81
	};

	TestInit(0x8, data, sizeof(data), "P0 > P1 && P2 > P3");

	const uint8 result[] = {
		0x00, 0x00, 0x22, 0x22, 0x00, 0x00, 0x22, 0x22,
		0x22, 0x00, 0x00, 0x22, 0x22, 0x00, 0x00, 0x22,
		0x22, 0x22, 0x00, 0x00, 0x22, 0x22, 0x00, 0x00,
		0x22, 0x22, 0x22, 0x00, 0x00, 0x22, 0x22, 0x00,
		0x66, 0x66, 0x66, 0x11, 0x11, 0x66, 0x66, 0x66,
		0x66, 0x66, 0x11, 0x66, 0x66, 0x11, 0x66, 0x66,
		0x66, 0x11, 0x66, 0x66, 0x66, 0x66, 0x11, 0x66,
		0x11, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x11
	};

	TestFinish(result, sizeof(result));
}


void
MovieDecoder::TestOpcode9A()
{
	const uint8 data[] = {
			0x00, 0x22, 0xcc, 0xdd,
			0x00, 0x00, 0x00, 0x11,
			0x00, 0x00, 0x00, 0x08,
			0xF0, 0x0F, 0xF0, 0x0F,
			0xFF, 0xFF, 0xFF, 0xFF
	};

	TestInit(0x9, data, sizeof(data), "P0 <= P1 && P2 <= P3");

	const uint8 result[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x00, 0x22,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xcc, 0x00,
		0xdd, 0xdd, 0x00, 0x00, 0x00, 0x00, 0xdd, 0xdd,
		0xdd, 0xdd, 0x00, 0x00, 0x00, 0x00, 0xdd, 0xdd,
		0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
		0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd
	};

	TestFinish(result, sizeof(result));
}


void
MovieDecoder::TestOpcode9B()
{
	const uint8 data[] = {
			0x00, 0x22, 0xdd, 0xcc,
			0x01, 0x24, 0xFF, 0x11
	};

	TestInit(0x9, data, sizeof(data), "P0 <= P1 && P2 > P3");

	const uint8 result[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x00, 0x22,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xcc, 0x00,
		0xdd, 0xdd, 0x00, 0x00, 0x00, 0x00, 0xdd, 0xdd,
		0xdd, 0xdd, 0x00, 0x00, 0x00, 0x00, 0xdd, 0xdd,
		0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
		0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd
	};

	TestFinish(result, sizeof(result));
}


void
MovieDecoder::TestOpcodeA2()
{
	const uint8 data[] = {
			0x22, 0x00, 0xdd, 0xcc, 0x11, 0x11,
			0x00, 0xFF, 0x11, 0xFF, 0x0F, 0x0F,
			0x33, 0x11, 0xFF, 0x11, 0x12, 0x14,
			0x25, 0xFF, 0x11, 0xFF, 0x0F, 0x0F
	};

	TestInit(0xA, data, sizeof(data), "P0 > P1");

	const uint8 result[] = {
		0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22,
		0x22, 0x22, 0x22, 0x22, 0xcc, 0xcc, 0xcc, 0xcc,
		0x00, 0x22, 0x00, 0x22, 0xcc, 0xcc, 0xcc, 0xcc,
		0xcc, 0xcc, 0x22, 0x22, 0xcc, 0xcc, 0x22, 0x22,
		0xff, 0x33, 0x11, 0x33, 0x33, 0x11, 0x11, 0x33,
		0x11, 0x11, 0xff, 0x33, 0x11, 0x11, 0x11, 0x11,
		0x11, 0x33, 0x11, 0x33, 0x11, 0x11, 0x11, 0x11,
		0x11, 0x11, 0x33, 0x33, 0x11, 0x11, 0x33, 0x33
	};

	TestFinish(result, sizeof(result));
}


void
MovieDecoder::TestOpcodeB()
{
	const uint8 data[] = {
		0x01, 0x00, 0x22, 0x22, 0x00, 0x00, 0x22, 0x22,
		0x22, 0x02, 0x00, 0x22, 0x22, 0x00, 0x00, 0x22,
		0x22, 0x22, 0x03, 0x00, 0x22, 0x22, 0x00, 0x00,
		0x22, 0x22, 0x22, 0x05, 0x00, 0x22, 0x22, 0x00,
		0x66, 0x66, 0x66, 0x11, 0x43, 0x66, 0x66, 0x66,
		0x66, 0x66, 0x11, 0x66, 0x66, 0x16, 0x66, 0x66,
		0x66, 0x11, 0x66, 0x66, 0x66, 0x12, 0x11, 0xA3,
		0x11, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x11
	};

	TestInit(0xB, data, sizeof(data));

	const uint8 result[] = {
		0x01, 0x00, 0x22, 0x22, 0x00, 0x00, 0x22, 0x22,
		0x22, 0x02, 0x00, 0x22, 0x22, 0x00, 0x00, 0x22,
		0x22, 0x22, 0x03, 0x00, 0x22, 0x22, 0x00, 0x00,
		0x22, 0x22, 0x22, 0x05, 0x00, 0x22, 0x22, 0x00,
		0x66, 0x66, 0x66, 0x11, 0x43, 0x66, 0x66, 0x66,
		0x66, 0x66, 0x11, 0x66, 0x66, 0x16, 0x66, 0x66,
		0x66, 0x11, 0x66, 0x66, 0x66, 0x12, 0x11, 0xA3,
		0x11, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x11
	};

	TestFinish(result, sizeof(result));
}


void
MovieDecoder::TestOpcodeC()
{
	const uint8 data[] = {
		0x01, 0x00, 0x22, 0x23,
		0x24, 0x40, 0x55, 0x60,
		0x11, 0x0B, 0xF0, 0x12,
		0x13, 0x15, 0x45, 0x70
	};

	TestInit(0xC, data, sizeof(data));

	const uint8 result[] = {
		0x01, 0x01, 0x00, 0x00, 0x22, 0x22, 0x23, 0x23,
		0x01, 0x01, 0x00, 0x00, 0x22, 0x22, 0x23, 0x23,
		0x24, 0x24, 0x40, 0x40, 0x55, 0x55, 0x60, 0x60,
		0x24, 0x24, 0x40, 0x40, 0x55, 0x55, 0x60, 0x60,
		0x11, 0x11, 0x0B, 0x0B, 0xF0, 0xF0, 0x12, 0x12,
		0x11, 0x11, 0x0B, 0x0B, 0xF0, 0xF0, 0x12, 0x12,
		0x13, 0x13, 0x15, 0x15, 0x45, 0x45, 0x70, 0x70,
		0x13, 0x13, 0x15, 0x15, 0x45, 0x45, 0x70, 0x70
	};

	TestFinish(result, sizeof(result));
}


void
MovieDecoder::TestOpcodeD()
{
	const uint8 data[] = {
		0x01, 0x22, 0x40, 0x10
	};

	TestInit(0xD, data, sizeof(data));

	const uint8 result[] = {
		0x01, 0x01, 0x01, 0x01, 0x22, 0x22, 0x22, 0x22,
		0x01, 0x01, 0x01, 0x01, 0x22, 0x22, 0x22, 0x22,
		0x01, 0x01, 0x01, 0x01, 0x22, 0x22, 0x22, 0x22,
		0x01, 0x01, 0x01, 0x01, 0x22, 0x22, 0x22, 0x22,
		0x40, 0x40, 0x40, 0x40, 0x10, 0x10, 0x10, 0x10,
		0x40, 0x40, 0x40, 0x40, 0x10, 0x10, 0x10, 0x10,
		0x40, 0x40, 0x40, 0x40, 0x10, 0x10, 0x10, 0x10,
		0x40, 0x40, 0x40, 0x40, 0x10, 0x10, 0x10, 0x10
	};

	TestFinish(result, sizeof(result));
}


void
MovieDecoder::TestOpcodeE()
{
	const uint8 data[] = {
		0x01
	};

	TestInit(0xE, data, sizeof(data));

	const uint8 result[] = {
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01
	};

	TestFinish(result, sizeof(result));
}


void
MovieDecoder::TestOpcodeF()
{
	const uint8 data[] = {
		0x01, 0x10
	};

	TestInit(0xF, data, sizeof(data));

	const uint8 result[] = {
		0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10,
		0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01,
		0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10,
		0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01,
		0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10,
		0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01,
		0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10,
		0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01
	};

	TestFinish(result, sizeof(result));
}
#endif
