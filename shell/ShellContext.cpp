/*
 * ShellContext.cpp
 *
 *  Created on: 07/ott/2012
 *      Author: stefano
 */

#include "Resource.h"
#include "ResManager.h"
#include "ResourceContext.h"
#include "ShellContext.h"

#include <stdlib.h>
#include <sstream>

struct res_string {
	uint16 type;
	const char* string;
};

const res_string kResString[] = {
	{ RES_KEY, "RES_KEY" },
	{ RES_BMP, "RES_BMP" },
	{ RES_MVE, "RES_MVE" },
	{ RES_WAV, "RES_WAV" },
	{ RES_WFX, "RES_WFX" },
	{ RES_PLT, "RES_PLT" },
	{ RES_BAM, "RES_BAM" },
	{ RES_WED, "RES_WED" },
	{ RES_CHU, "RES_CHU" },
	{ RES_TIS, "RES_TIS" },
	{ RES_MOS, "RES_MOS" },
	{ RES_ITM, "RES_ITM" },
	{ RES_SPL, "RES_SPL" },
	{ RES_BCS, "RES_BCS" },
	{ RES_IDS, "RES_IDS" },
	{ RES_CRE, "RES_CRE" },
	{ RES_ARA, "RES_ARA" },
	{ RES_DLG, "RES_DLG" },
	{ RES_2DA, "RES_2DA" },
	{ RES_GAM, "RES_GAM" },
	{ RES_STO, "RES_STO" },
	{ RES_WMP, "RES_WMP" },
	{ RES_EFF, "RES_EFF" },
	{ RES_VVC, "RES_VVC" },
	{ RES_PRO, "RES_PRO" },
};


static
uint16
res_type_for_string(const char* string)
{
	for (size_t i = 0; i < sizeof(kResString) / sizeof(kResString[0]); i++) {
		const char* resStr = kResString[i].string;
		if (resStr != NULL && !strcasecmp(resStr, string)) {
			return kResString[i].type;
		}
	}
	throw -1;
	return 0;
}


ShellContext::ShellContext(ShellContext* context)
	:
	fParentContext(context)
{
}


ShellContext::~ShellContext()
{
}


/* static */
ShellContext*
ShellContext::ExitContext(ShellContext* context)
{
	ShellContext* parent = context->fParentContext;
	delete context;

	return parent;
}


// BaseContext
BaseContext::BaseContext()
	:
	ShellContext(NULL)
{
}


BaseContext::~BaseContext()
{
}


ShellContext*
BaseContext::HandleCommand(std::string line)
{
	ShellContext *context = this;
	std::istringstream cmdString(line);
	std::string text;
	cmdString >> text;
	if (!text.compare("list")) {
		std::string strParam;
		cmdString >> strParam;
		int16 resType = -1;
		try {
			if (!strParam.empty())
				resType = res_type_for_string(strParam.c_str());
		} catch (...) {
			resType = -1;
		}
		gResManager->PrintResources(resType);
	} else {
		ref_type refType;
		int pos = text.find('.');
		if (pos > 0) {
			text.copy(refType.name.name, pos);
			refType.name.name[pos] = '\0';
			refType.type = res_string_to_type(text.c_str());
			if (refType.type != (uint16)-1) {
				context = ResourceContext::CreateResourceContext(
												this, refType);
				if (context == NULL)
					context = this;
			}
			// TODO: pass remaining cmdline to new context
			// TODO: Error messages ??
		}
	}
	return context;
}


void
BaseContext::PrintPrompt()
{
	std::cout << "> " << std::flush;
}


void
BaseContext::ListCommands()
{
	std::cout << "exit" << "\t";
	std::cout << "Exits the application." << std::endl;

	std::cout << "<resource>" << "\t";
	std::cout << "Load the named resource ";
	std::cout << "and enters the resource context." << std::endl;
}
