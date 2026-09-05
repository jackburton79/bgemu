#include "AnimationTester.h"
#include "Core.h"
#include "Game.h"
#include "GraphicsEngine.h"
#include "Log.h"
#include "MovieDecoder.h"
#include "ResManager.h"
#include "Script.h"
#include "SoundEngine.h"

#include <getopt.h>

static int sList = 0;
static int sNoScripts = 0;
static int sNoNewGame = 0;
static int sFullScreen = 0;
static int sTest = 0;
static int sTestAnimation = 0;
static int sDebug = 0;
static uint16 sScreenWidth = 640;
static uint16 sScreenHeight = 480;
static const char *sPath;
static const char *sResourceName = NULL;
static const char *sPartyMembers = NULL;

static
struct option sLongOptions[] = {
		{ "list-resources", no_argument, &sList, 'l' },
		{ "test", no_argument, NULL, 't' },
		{ "test-animation", required_argument, NULL, 'T' },
		{ "dump-resource", required_argument, NULL, 'd' },
		{ "path", required_argument, NULL, 'p'},
		{ "no-scripts", no_argument, &sNoScripts, 'n' },
		{ "no-newgame", no_argument, &sNoNewGame, 'N' },
		{ "debug", no_argument, &sDebug, 'D' },
		{ "fullscreen", no_argument, &sFullScreen, 'f' },
		// Comma-separated list of CRE resrefs to start the party with,
		// overriding Game::CreateParty()'s hardcoded default (e.g.
		// "-P ANOMEN10,Imoen,Minsc") - see Game::SetStartingPartyMembers().
		{ "party", required_argument, NULL, 'P' },
		{ 0, 0, 0, 0 }
};


static void
ParseScreenGeometry(char* string)
{
	char* rest = NULL;
	sScreenWidth = ::strtoul(string, &rest, 10);
	sScreenHeight = ::strtoul(rest + 1, NULL, 10);
}


static void
ParseArgs(int argc, char **argv)
{
	int optIndex = 0;
	int c = 0;
	while ((c = getopt_long(argc, argv, "g:p:Dd:nNltfT:P:",
				sLongOptions, &optIndex)) != -1) {
		switch (c) {
			case 'p':
				sPath = optarg;
				break;
			case 'P':
				sPartyMembers = optarg;
				break;
			case 'd':
				sResourceName = optarg;
				break;
			case 'D':
				sDebug = 1;
				break;
			case 'f':
				sFullScreen = 1;
				break;
			case 't':
				sTest = 1;
				break;
			case 'T':
				sTestAnimation = 1;
				sResourceName = optarg;
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
	check_types_size();
	
	ParseArgs(argc, argv);

	if (sTest) {
		// TODO: Do more tests
		std::cout << "Testing Mode" << std::endl;
		MovieDecoder decoder;
		int status = decoder.Test();
		if (status != 0)
			std::cerr << RED("Movie Decoding test failed!") << std::endl;
		Game::Get()->SetTestMode(true);
	}
	
	if (!Core::Initialize(sPath)) {
		std::cerr << RED("Core initialization failed!") << std::endl;
		return -1;
	}

	if (sList) {
		gResManager->PrintResources();
		Core::Destroy();
		return 0;
	}

	if (sResourceName != NULL && !sTestAnimation) {
		std::cout << "Dump resource Mode" << std::endl;
		Resource* resource = gResManager->GetResource(sResourceName);
		if (resource != NULL)
			resource->Dump();
		gResManager->ReleaseResource(resource);
		Core::Destroy();
		return 0;
	}
	
	if (sDebug) {
		//gResManager->SetDebug(2);
		Script::SetDebug(true);
		//Object::SetDebug(true);
	}

	if (sPartyMembers != NULL) {
		std::vector<std::string> names;
		std::string remaining = sPartyMembers;
		size_t comma;
		while ((comma = remaining.find(',')) != std::string::npos) {
			names.push_back(remaining.substr(0, comma));
			remaining.erase(0, comma + 1);
		}
		if (!remaining.empty())
			names.push_back(remaining);
		Game::Get()->SetStartingPartyMembers(names);
	}

	if (!GraphicsEngine::Initialize()) {
		Core::Destroy();
		std::cerr << RED("Failed to initialize Graphics Engine!") << std::endl;
		return -1;
	}

	int flags = 0;
	if (sFullScreen)
		flags = GraphicsEngine::VIDEOMODE_FULLSCREEN;
	GraphicsEngine::Get()->SetVideoMode(sScreenWidth, sScreenHeight, 16, flags);
	
	if (!SoundEngine::Initialize())
		std::cerr << RED("Failed to initialize Sound Engine! Continuing anyway...") << std::endl;

	try {
		if (sTestAnimation) {
			AnimationTester animTester(sResourceName);
			animTester.Loop();
		} else
			Game::Get()->Loop(sNoNewGame, !sNoScripts);
	} catch (std::exception &error) {
		std::cerr << RED(error.what()) << std::endl;
	} catch (...) {
		std::cerr << RED("Game Loop exited with unknown error") << std::endl;
	}
	
	GraphicsEngine::Destroy();
	SoundEngine::Destroy();
	Core::Destroy();
	return 0;
}
