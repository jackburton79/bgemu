#include "SupportDefs.h"
#include "Utils.h"

void
check_types_size()
{
	assert_size(sizeof(int32), 4);
	assert_size(sizeof(uint32), 4);
	assert_size(sizeof(sint32), 4);
	assert_size(sizeof(int16), 2);
	assert_size(sizeof(uint16), 2);
	assert_size(sizeof(sint16), 2);
	assert_size(sizeof(int8), 1);
	assert_size(sizeof(uint8), 1);
	assert_size(sizeof(sint8), 1);
	assert_size(sizeof(char), 1);
}
