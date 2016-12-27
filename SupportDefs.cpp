#include "SupportDefs.h"
#include "Utils.h"

void
check_types_size()
{
	assert_size(sizeof(int32), 4);
	assert_size(sizeof(sint32), 4);
	assert_size(sizeof(uint32), 4);
}
