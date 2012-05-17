#include "BCSResource.h"
#include "MemoryStream.h"
#include "Parsing.h"
#include "Script.h"


BCSResource::BCSResource(const res_ref &name)
	:
	Resource(name, RES_BCS)
{
}


BCSResource::~BCSResource()
{

}


bool
BCSResource::Load(Archive *archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;

	//DumpToFile("bcs_script.txt");

	_LoadScript();

	return true;
}


void
BCSResource::RunScript()
{



}


void
BCSResource::_LoadScript()
{
	printf("loadscript\n");
	try {
		Parser parser;
		parser.SetTo(fData);

		parser.Read();
	} catch (const char *message) {
		printf("exception thrown: %s\n", message);
	} catch (...) {
		printf("other exception thrown\n");
		throw "SUCKERPUNCH!!";
	}
}
