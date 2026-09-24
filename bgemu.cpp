#include "AreaRoom.h"
#include "Core.h"
#include "Game.h"
#include "StartingParty.h"
#include "SavedGame.h"
#include "GraphicsEngine.h"
#include "Log.h"
#include "MovieDecoder.h"
#include "ResManager.h"
#include "Script.h"
#include "SoundEngine.h"

#include <filesystem>
#include <getopt.h>
#include <stdlib.h>

static int sList = 0;
static int sNoScripts = 0;
static int sNoNewGame = 0;
static int sSkipIntro = 0;
static int sFullScreen = 0;
static int sTest = 0;
static int sDebug = 0;
static uint16 sScreenWidth = 640;
static uint16 sScreenHeight = 480;
static const char *sPath;
static const char *sResourceName = NULL;
static const char *sPartyMembers = NULL;
static const char *sExecFile = NULL;
static const char *sStartingArea = NULL;
static const char *sCharacterSpec = NULL;
static const char *sSaveDirectory = NULL;

static
struct option sLongOptions[] = {
		{ "list-resources", no_argument, &sList, 'l' },
		{ "test", no_argument, NULL, 't' },
		{ "dump-resource", required_argument, NULL, 'd' },
		{ "path", required_argument, NULL, 'p'},
		{ "no-scripts", no_argument, &sNoScripts, 'n' },
		{ "no-newgame", no_argument, &sNoNewGame, 'N' },
		// Start without the intro movies. They are also left out whenever
		// something else says what to start with (--exec-file, --area,
		// --party, --character, --no-newgame, --test).
		{ "skip-intro", no_argument, &sSkipIntro, 'I' },
		{ "debug", no_argument, &sDebug, 'D' },
		{ "fullscreen", no_argument, &sFullScreen, 'f' },
		// Comma-separated list of CRE resrefs to start the party with,
		// overriding StartingParty's hardcoded default (e.g.
		// "-P ANOMEN10,Imoen,Minsc") - see StartingParty::SetMembers().
		{ "party", required_argument, NULL, 'P' },
		// Path to a test script (one GameConsole command per line, blank
		// lines and '#' comments ignored) - run automatically right after
		// the starting area/worldmap loads, then the game quits. See
		// Game::SetExecFile()/GameConsole::RunFile().
		{ "exec-file", required_argument, NULL, 'x' },
		// Area resref to load directly on startup, skipping both the
		// opening cutscene (LoadStartingArea()) and --no-newgame's
		// worldmap - e.g. "-a AR0602". Takes priority over --no-newgame
		// if both are given. See Game::SetStartingArea().
		{ "area", required_argument, NULL, 'a' },
		// Path to a character-creation spec file ("field value" lines,
		// '#' comments): gender/race/class/kit/alignment plus either the
		// six ability scores or a bare "roll" line. Builds the party
		// leader from scratch (roadmap Fase 47 / A). See
		// StartingParty::SetCharacterSpec().
		{ "character", required_argument, NULL, 'c' },
		// Directory for saves and area checkpoints, instead of
		// "<game path>/bgemu-save" (also settable with the BGEMU_SAVE_DIR
		// environment variable; this option wins). See
		// SavedGame::SetDirectory().
		{ "save-dir", required_argument, NULL, 'S' },
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
	while ((c = getopt_long(argc, argv, "g:p:Dd:nNIltfT:P:x:a:c:S:",
				sLongOptions, &optIndex)) != -1) {
		switch (c) {
			case 'p':
				sPath = optarg;
				break;
			case 'P':
				sPartyMembers = optarg;
				break;
			case 'x':
				sExecFile = optarg;
				break;
			case 'a':
				sStartingArea = optarg;
				break;
			case 'c':
				sCharacterSpec = optarg;
				break;
			case 'S':
				sSaveDirectory = optarg;
				break;
			case 'd':
				sResourceName = optarg;
				break;
			case 'D':
				sDebug = 1;
				break;
			// The long forms set these through getopt_long()'s flag pointer;
			// the short ones come back here.
			case 'n':
				sNoScripts = 1;
				break;
			case 'N':
				sNoNewGame = 1;
				break;
			case 'I':
				sSkipIntro = 1;
				break;
			case 'l':
				sList = 1;
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

	if (sResourceName != NULL) {
		std::cout << "Dump resource Mode" << std::endl;
		Resource* resource = gResManager->GetResource(sResourceName);
		if (resource != NULL)
			resource->Dump();
		gResManager->ReleaseResource(resource);
		Core::Destroy();
		return 0;
	}
	
	if (sDebug) {
		Script::SetDebug(true);
	}

	std::filesystem::path saveDirectory;
	if (sSaveDirectory != NULL)
		saveDirectory = sSaveDirectory;
	else if (const char* fromEnvironment = ::getenv("BGEMU_SAVE_DIR"))
		saveDirectory = fromEnvironment;
	else
		saveDirectory = std::filesystem::path(sPath) / "bgemu-save";
	Game::Get()->Saves().SetDirectory(saveDirectory.string());

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
		Game::Get()->Starting().SetMembers(names);
	}

	if (sExecFile != NULL)
		Game::Get()->SetExecFile(sExecFile);

	Game::Get()->SetShowIntro(!sSkipIntro && sExecFile == NULL && sStartingArea == NULL
		&& sPartyMembers == NULL && sCharacterSpec == NULL && !sNoNewGame && !sTest);

	if (sStartingArea != NULL)
		Game::Get()->SetStartingArea(sStartingArea);

	if (sCharacterSpec != NULL)
		Game::Get()->Starting().SetCharacterSpec(sCharacterSpec);

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

	// Guards against a *previous* run's checkpoints (left behind by a
	// crash or a kill - the exit-time ClearAreaCheckpoints() below never
	// got a chance to run) bleeding into this one. A Game::Load() further
	// into this run restores whatever that save's own archive had into
	// this same directory before touching any area (see AreaRoom::
	// AreaCheckpointDir()'s own comment), so this is only ever about a
	// stale leftover from *before* any load happens this run.
	AreaRoom::ClearAreaCheckpoints();

	try {
			Game::Get()->Loop(sNoNewGame, !sNoScripts);
	} catch (std::exception &error) {
		std::cerr << RED(error.what()) << std::endl;
	} catch (...) {
		std::cerr << RED("Game Loop exited with unknown error") << std::endl;
	}
	
	GraphicsEngine::Destroy();
	SoundEngine::Destroy();
	Core::Destroy();

	// For now, don't keep area checkpoints across program runs (see the
	// method's own comment).
	AreaRoom::ClearAreaCheckpoints();
	return 0;
}
