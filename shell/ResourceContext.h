/*
 * ResourceContext.h
 *
 *  Created on: 07/ott/2012
 *      Author: stefano
 */

#ifndef __RESOURCECONTEXT_H_
#define __RESOURCECONTEXT_H_

#include "ShellContext.h"

class Resource;
class ResourceContext : public ShellContext {
public:
	ResourceContext(ShellContext* parent, const ref_type&);
	virtual ShellContext* HandleCommand(std::string command);
	virtual void PrintPrompt();
	virtual void ListCommands();
	virtual ~ResourceContext();

	static ResourceContext* CreateResourceContext(
				ShellContext* context,
				const ref_type& refType);
protected:
	Resource* fResource;
};


#endif /* __RESOURCECONTEXT_H_ */
