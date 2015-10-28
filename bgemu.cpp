#include "Actor.h"
#include "Archive.h"
#include "Bitmap.h"
#include "Control.h"
#include "Core.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "MovieDecoder.h"
#include "ResManager.h"
#include "Room.h"
#include "SoundEngine.h"
#include "Timer.h"
#include "WorldMap.h"
#include "WMAPResource.h"

#include <getopt.h>
#include <SDL.h>

static int sList = 0;
static int sNoScripts = 0;
static int sFullScreen = 0;
static int sTest = 0;
static uint16 sScreenWidth = 640;
static uint16 sScreenHeight = 480;
static const char *sPath;
static const char *sResourceName = NULL;

static
struct option sLongOptions[] = {
		{ "list", no_argument, &sList, 1 },
		{ "test", no_argument, NULL, 't' },
		{ "showresource", required_argument, NULL, 's'},
		{ "path", required_argument, NULL, 'p'},
		{ "noscripts", no_argument, &sNoScripts, 'n' },
		{ "fullscreen", no_argument, &sFullScreen, 'f' },
		{ 0, 0, 0, 0 }
};


static void
ParseScreenGeometry(char* string)
{
	char* rest = NULL;
	sScreenWidth = ::strtoul(string, &rest, 10);
	std::cout << sScreenWidth << std::endl;
	sScreenHeight = ::strtoul(rest + 1, NULL, 10);
}


static void
ParseArgs(int argc, char **argv)
{
	int optIndex = 0;
	int c = 0;
	while ((c = getopt_long(argc, argv, "g:p:s:ltnf",
				sLongOptions, &optIndex)) != -1) {
		switch (c) {
			case 'p':
				sPath = optarg;
				break;
			case 's':
				sResourceName = optarg;
				break;
			case 'f':
				sFullScreen = 1;
				break;
			case 't':
				sTest = 1;
				break;
			case 'g':
				ParseScreenGeometry(optarg);
				break;
			default:
				break;
		}
	}
}


int
main(int argc, char **argv)
{
	ParseArgs(argc, argv);

	if (sTest) {
		// TODO: Do more tests
		std::cout << "Testing Mode" << std::endl;
		MovieDecoder decoder;
		int status = decoder.Test();
		return status;
	}
	
	if (!Core::Initialize(sPath)) {
		std::cerr << "Failed to initialize Core!" << std::endl;
		return -1;
	}

	if (sList) {
		gResManager->PrintResources();
		Core::Destroy();
		return 0;
	}

	if (sResourceName != NULL) {
		std::cout << "Dump resource Mode" << std::endl;
		Resource* resource = gResManager->GetResource(sResourceName);
		if (resource != NULL)
			resource->Dump();
		gResManager->ReleaseResource(resource);
		Core::Destroy();
		return 0;
	}

	if (!GraphicsEngine::Initialize()) {
		Core::Destroy();
		std::cerr << "Failed to initialize Graphics Engine!" << std::endl;
		return -1;
	}

	if (!SoundEngine::Initialize())
		std::cerr << "Failed to initialize Sound Engine! Continuing anyway..." << std::endl;
	

	int flags = 0;
	if (sFullScreen)
		flags = GraphicsEngine::VIDEOMODE_FULLSCREEN;
	GraphicsEngine::Get()->SetVideoMode(sScreenWidth, sScreenHeight, 16, flags);

	// TODO: Move this to Core::Initialize() (or Core::Start())
	
	if (!GUI::Initialize(sScreenWidth, sScreenHeight)) {
		std::cerr << "Initializing GUI failed" << std::endl;
		GraphicsEngine::Destroy();
		SoundEngine::Destroy();
		Core::Destroy();
	}
	
	RoomContainer* container = RoomContainer::Get();
	if (!container->LoadWorldMap()) {
		std::cerr << "LoadWorldMap failed" << std::endl;
		GraphicsEngine::Destroy();
		SoundEngine::Destroy();
		Core::Destroy();
		return -1;
	}

	if (container != NULL)
		container->StartLoop(!sNoScripts);

	RoomContainer::Delete();
	GUI::Destroy();
	GraphicsEngine::Destroy();
	SoundEngine::Destroy();
	Core::Destroy();
	return 0;
}
