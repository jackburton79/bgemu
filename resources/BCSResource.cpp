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
/*
	std::string fileName;
	fileName.append(Name()).append(".bcs");
	this->DumpToFile(fileName.c_str());*/
	return true;
}


Script*
BCSResource::GetScript() const
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
