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
}


bool
BCSResource::Load(Archive *archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;

	return true;
}


Script*
BCSResource::GetScript()
{
	Script* script = NULL;
	try {
		Parser parser;
		parser.SetTo(fData);

		node* rootNode = NULL;
		parser.Read(rootNode);

		// Takes ownership of the node tree.
		script = new Script(rootNode);

	} catch (...) {
		script = NULL;
	}

	return script;
}
