#include "BCSResource.h"

#include "Log.h"
#include "MemoryStream.h"
#include "Parsing.h"
#include "Script.h"

#include <time.h>


/* static */
Resource*
BCSResource::Create(const res_ref& name)
{
	return new BCSResource(name);	
}


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

	return true;
}


Script*
BCSResource::GetScript() const
{
	Script* script = NULL;

	try {
		Parser parser;
		parser.SetTo(fData);

		clock_t start = clock();
		node* rootNode = NULL;
		parser.Read(rootNode);
		
		std::cout << "Parsing script " << Name() << " took " << std::dec << clock() - start << " usecs!" << std::endl;
		
		// Takes ownership of the node tree.
		script = new Script(rootNode);
	} catch (std::exception& e) {
		std::cerr << RED(e.what()) << std::endl;
		script = NULL;
	}

	return script;
}


/* virtual */
void
BCSResource::Dump()
{
	Script* script = GetScript();
	script->Print();
	delete script;
}
