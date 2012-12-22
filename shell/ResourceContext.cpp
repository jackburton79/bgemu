/*
 * ResourceContext.cpp
 *
 *  Created on: 07/ott/2012
 *      Author: stefano
 */

#include "AreaResource.h"
#include "BamResource.h"
#include "Bitmap.h"
#include "DoorContext.h"
#include "GraphicsEngine.h"
#include "Polygon.h"
#include "Resource.h"
#include "ResManager.h"
#include "ResourceContext.h"
#include "ShellContext.h"
#include "WedResource.h"

#include <stdlib.h>
#include <sstream>

class WEDResourceContext : public ResourceContext {
public:
	WEDResourceContext(ShellContext* parent,
			const ref_type& resource);
	virtual ShellContext* HandleCommand(std::string line);
	virtual void ListCommands();

};


class AREAResourceContext : public ResourceContext {
public:
	AREAResourceContext(ShellContext* parent,
			const ref_type& resource);
	virtual ShellContext* HandleCommand(std::string line);
	virtual void ListCommands();
};


class BAMResourceContext : public ResourceContext {
public:
	BAMResourceContext(ShellContext* parent,
			const ref_type& resource);
	virtual ShellContext* HandleCommand(std::string line);
	virtual void ListCommands();
private:
	void DisplaySequence(int32 index);
};


// ResourceContext
ResourceContext::ResourceContext(ShellContext* context,
		const ref_type& resource)
	:
	ShellContext(context)
{
	fResource = gResManager->_GetResource(resource.name, resource.type);
	if (fResource == NULL)
		throw "cannot load resource";
}


ResourceContext::~ResourceContext()
{
	gResManager->ReleaseResource(fResource);
}


ShellContext*
ResourceContext::HandleCommand(std::string line)
{
	std::string text;
	std::istringstream cmdString(line);
	cmdString >> text;
	if (!text.compare("select")) {

	} else
		fParentContext->HandleCommand(line);

	return this;
}


void
ResourceContext::ListCommands()
{
	fParentContext->ListCommands();
}


void
ResourceContext::PrintPrompt()
{
	std::cout << "[";
	std::cout << fResource->Name() << res_extension(fResource->Type());
	std::cout << "] ";
}


/* static */
ResourceContext*
ResourceContext::CreateResourceContext(
				ShellContext* parent,
				const ref_type& refType)
{
	try {
		switch (refType.type) {
			case RES_BAM:
				return new BAMResourceContext(parent, refType);
			case RES_WED:
				return new WEDResourceContext(parent, refType);
			case RES_ARA:
				return new AREAResourceContext(parent, refType);
			default:
				return new ResourceContext(parent, refType);
		}
	} catch (const char* string) {
		printf("%s\n", string);
	} catch (...) {

	}
	return NULL;
}


// BAMResourceContext
BAMResourceContext::BAMResourceContext(ShellContext* parent,
		const ref_type& resRef)
	:
	ResourceContext(parent, resRef)
{

}


ShellContext*
BAMResourceContext::HandleCommand(std::string line)
{
	BAMResource* bam = dynamic_cast<BAMResource*>(fResource);
	if (bam == NULL)
		return this;

	std::string text;
	std::string stringParam;
	std::istringstream cmdString(line);
	cmdString >> text;
	uint32 numParam;
	if (!text.compare("list")) {
		cmdString >> stringParam;
		if (!stringParam.compare("sequences")) {
			for (uint32 i = 0; i < bam->CountCycles(); i++)
				std::cout << "sequence " << i << std::endl;
		}
	} else if (!text.compare("print")) {
		cmdString >> stringParam;
		if (!stringParam.compare("sequence")) {
			cmdString >> numParam;
			if (numParam < bam->CountCycles()) {
				bam->PrintFrames(numParam);
			}
		}
	} else if (!text.compare("play")) {
		cmdString >> stringParam;
		if (!stringParam.compare("sequence")) {
			cmdString >> numParam;
			if (numParam < bam->CountCycles()) {
				DisplaySequence(numParam);
			}
		}
	} else
		fParentContext->HandleCommand(line);

	return this;
}


void
BAMResourceContext::ListCommands()
{
	std::cout << "BAMResourceContext commands" << std::endl;
	std::cout << "exit" << "\t";
	std::cout << "Exits the context." << std::endl;

	std::cout << "list sequences" << std::endl;
	std::cout << "play sequence <num>" << std::endl;

	ResourceContext::ListCommands();
}


void
BAMResourceContext::DisplaySequence(int32 index)
{
	if (!GraphicsEngine::Initialize())
		return;

	GraphicsEngine* gfx = GraphicsEngine::Get();
	BAMResource* bam = dynamic_cast<BAMResource*>(fResource);
	gfx->SetVideoMode(640, 480, 16, 0);
	for (int32 numFrame = 0; numFrame < bam->CountFrames(index); numFrame++) {
		Frame frame = bam->FrameForCycle(index, numFrame);
		gfx->BlitToScreen(frame.bitmap, &frame.rect, NULL);
		gfx->Flip();
		GraphicsEngine::DeleteBitmap(frame.bitmap);
		sleep(0.4);
	}

	sleep(2);
	GraphicsEngine::Destroy();
}


// WEDResourceContext
WEDResourceContext::WEDResourceContext(ShellContext* parent,
		const ref_type& resRef)
	:
	ResourceContext(parent, resRef)
{

}


ShellContext*
WEDResourceContext::HandleCommand(std::string line)
{
	WEDResource* wed = dynamic_cast<WEDResource*>(fResource);
	if (wed == NULL)
		return this;

	std::string text;
	std::string stringParam;
	std::istringstream cmdString(line);
	cmdString >> text;
	if (!text.compare("list")) {
		cmdString >> stringParam;
		if (!stringParam.compare("polygons")) {
			for (uint32 i = 0; i < wed->CountPolygons(); i++)
				std::cout << "polygon " << i << std::endl;
		}
	} else if (!text.compare("print")) {
		cmdString >> stringParam;
		if (!stringParam.compare("polygon")) {
			uint32 intParam;
			cmdString >> intParam;
			if (intParam < wed->CountPolygons())
				wed->PolygonAt(intParam)->Print();
		}
	} else
		fParentContext->HandleCommand(line);

	return this;
}


void
WEDResourceContext::ListCommands()
{
	std::cout << "WEDResourceContext commands" << std::endl;
	std::cout << "exit" << "\t";
	std::cout << "Exits the context." << std::endl;

	std::cout << "list polygons" << std::endl;
	std::cout << "print polygon <num>" << std::endl;

	ResourceContext::ListCommands();
}


// AREAResourceContext
AREAResourceContext::AREAResourceContext(ShellContext* parent,
		const ref_type& resRef)
	:
	ResourceContext(parent, resRef)
{

}


ShellContext*
AREAResourceContext::HandleCommand(std::string line)
{
	ARAResource* area = dynamic_cast<ARAResource*>(fResource);
	if (area == NULL)
		return this;

	std::string text;
	std::string stringParam;
	uint32 intParam;
	std::istringstream cmdString(line);
	cmdString >> text;
	/*if (!text.compare("actor")) {
		uint32 numParam;
		cmdString >> numParam;
		if (numParam <= (uint32)area->CountActors()) {
			IE::actor* actor = area->ActorAt(numParam);
			if (actor != NULL) {
				return new ActorContext(this, actor);
			}
		}
	} else */
	if (!text.compare("list")) {
		cmdString >> stringParam;
		if (!stringParam.compare("actors")) {
			for (uint32 i = 0; i < area->CountActors(); i++) {
				/*std::cout << "Actor " << i
					<< " " << area->ActorAt(i)->name
					<< std::endl;*/
			}
		} else if (!stringParam.compare("animations")) {
			for (uint32 i = 0; i < area->CountAnimations(); i++) {
				std::cout << "animation " << i
					<< " " << area->AnimationAt(i)->name
					<< std::endl;
			}
		}
	} else if (!text.compare("print")) {
		cmdString >> stringParam;
		if (!stringParam.compare("actor")) {
			cmdString >> intParam;
			//if (intParam < area->CountActors())
				//area->ActorAt(intParam)->Print();
		} else if (!stringParam.compare("animation")) {
			cmdString >> intParam;
			if (intParam < area->CountAnimations()) {
				area->AnimationAt(intParam)->Print();
			}
		}
	} else
		fParentContext->HandleCommand(line);

	return this;
}


void
AREAResourceContext::ListCommands()
{
	std::cout << "AREAResourceContext commands" << std::endl;
	std::cout << "exit" << "\t";
	std::cout << "Exits the context." << std::endl;

	std::cout << "list actors" << std::endl;
	std::cout << "list animations" << std::endl;
	std::cout << "print actor <num>" << std::endl;
	std::cout << "print animation <num>" << std::endl;

	ResourceContext::ListCommands();
}
