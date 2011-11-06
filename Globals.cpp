#include "Resource.h"
#include "Types.h"

#include <cstdio>

static char sTemp[256];

// This function is not thread safe
const char *
strresource(int type)
{
	switch (type) {
		case RES_BMP:
			return "Bitmap (BMP) format";
		case RES_AREA:
			return "AREA format";
		case RES_MVE:
			return "Movie (MVE) format";
		case RES_WAV:
			return "WAV format";
		case RES_PLT:
			return "Paper Dolls (PLT) format";
		case RES_ITM:
			return "Item";
		case RES_BAM:
	 		return "BAM format";
	 	case RES_WED:
	 		return "WED format";
	 	case RES_TIS:
	 		return "TIS format";
	 	case RES_MOS:
	 		return "MOS format";
	 	case RES_SPL:
			return "SPL format";
		case RES_COMPILED_SCRIPT:
			return "Compiled script (BCS format)";
		case RES_CRE:
		 	return "Creature";
		case RES_DLG:
		 	return "Dialog";
		case RES_EFF:
		 	return "EFF Effect";
		case RES_VVC:
		 	return "VVC Effect";
		case RES_WFX:
		default:
		 	sprintf(sTemp, "unknown (0x%x)", type);
			return sTemp;
	}
}


res_ref::res_ref()
{
}


res_ref::res_ref(const char *string)
{
	int32 len = string ? strlen(string) : 0;
	len = std::min(len, 8);
	memcpy(name, string, len);
	if (len < 8) {
		memset(name + len, '\0', 8 - len);
	}
}


res_ref::res_ref(const res_ref &ref)
{
	memcpy(name, ref.name, 8);
}


res_ref::operator const char*() const
{
	static char str[9];
	str[8] = '\0';
	memcpy(str, name, 8);
	return (const char *)str;
};


bool
operator<(const res_ref &ref1, const res_ref &ref2)
{
	return strncasecmp(ref1.name, ref2.name, 8) < 0;
}


bool
operator==(const res_ref &ref1, const res_ref &ref2)
{
	return strncasecmp(ref1.name, ref2.name, 8) == 0;
}


bool
operator<(const ref_type &ref1, const ref_type &ref2)
{
	int nameDiff = strncasecmp(ref1.name.name, ref2.name.name, 8);
	if (nameDiff < 0)
		return true;

	if (nameDiff > 0)
		return false;

	return ref1.type < ref2.type;
}


std::ostream &
operator<<(std::ostream &os, res_ref ref)
{
	for (int32 i = 0; i < 8; i++) {
		if (ref.name[i] == '\0')
			break;
		std::cout << ref.name[i];
	}
	return os;
};

