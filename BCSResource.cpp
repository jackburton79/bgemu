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
	delete fScript;
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
BCSResource::GetScript() const
{
	return fScript;
}


void
BCSResource::_LoadScript()
{
	node *rootNode = NULL;

	Parser parser;
	parser.SetTo(fData);
	parser.Read(rootNode);

	// Takes ownership of the node tree.
	fScript = new Script(rootNode);

	DropData();
}
