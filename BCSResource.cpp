#include "BCSResource.h"
#include "MemoryStream.h"
#include "Parsing.h"
#include "Script.h"


BCSResource::BCSResource(const res_ref &name)
	:
	Resource(name, RES_BCS),
	fScript(NULL)
{
}


BCSResource::~BCSResource()
{
	//delete fScript;
}


bool
BCSResource::Load(Archive *archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;
	try {
		_LoadScript();
	} catch (...) {
		return false;
	}
	return true;
}


Script*
BCSResource::GetScript()
{
	node *rootNode = NULL;
	try {
		Parser parser;
		parser.SetTo(fData);
		parser.Read(rootNode);

		// Takes ownership of the node tree.
		return new Script(rootNode);
	} catch (...) {
		return NULL;
	}
}


void
BCSResource::_LoadScript()
{


	//DropData();
}
