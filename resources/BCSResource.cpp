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
	node* rootNode = NULL;
	Script* script = NULL;
	try {
		Parser parser;
		if (fName == "ICFUNGUS") {
			parser.SetDebug(true);
			DumpToFile("icfungus-bcs.dump");
		}
		parser.SetTo(fData);
		parser.Read(rootNode);

		// Takes ownership of the node tree.
		script = new Script(rootNode);

	} catch (...) {

	}

	return script;
}
