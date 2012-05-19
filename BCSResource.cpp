#include "BCSResource.h"
#include "MemoryStream.h"
#include "Parsing.h"
#include "Script.h"


BCSResource::BCSResource(const res_ref &name)
	:
	Resource(name, RES_BCS),
	fRootNode(NULL)
{
}


BCSResource::~BCSResource()
{
	delete fRootNode;
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


Script*
BCSResource::GetScript()
{
	return new Script(fRootNode);
}


void
BCSResource::_LoadScript()
{
	printf("loadscript\n");
	try {
		Parser parser;
		parser.SetTo(fData);

		parser.Read(fRootNode);
	} catch (const char *message) {
		printf("exception thrown: %s\n", message);
	} catch (...) {
		printf("other exception thrown\n");
		throw "SUCKERPUNCH!!";
	}

	DropData();
}
