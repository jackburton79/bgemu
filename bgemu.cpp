#include "Actor.h"
#include "Archive.h"
#include "Bitmap.h"
#include "Control.h"
#include "Core.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "InputConsole.h"
#include "MovieDecoder.h"
#include "OutputConsole.h"
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


void
ParseArgs(int argc, char **argv)
{
	int optIndex = 0;
	int c = 0;
	while ((c = getopt_long(argc, argv, "p:s:ltnf",
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
		return 0;
	}

	if (sResourceName != NULL) {
		std::cout << "Dump resource Mode" << std::endl;
		Resource* resource = gResManager->GetResource(sResourceName);
		if (resource != NULL)
			resource->Dump();
		gResManager->ReleaseResource(resource);
		return 0;
	}

	if (!GraphicsEngine::Initialize()) {
		Core::Destroy();
		std::cerr << "Failed to initialize Graphics Engine!" << std::endl;
		return -1;
	}

	if (!SoundEngine::Initialize())
		std::cerr << "Failed to initialize Sound Engine! Continuing anyway..." << std::endl;
	
	uint16 screenWidth = 800;
	uint16 screenHeight = 600;
	int flags = 0;
	if (sFullScreen)
		flags = GraphicsEngine::VIDEOMODE_FULLSCREEN;
	GraphicsEngine::Get()->SetVideoMode(screenWidth, screenHeight, 16, flags);

	// TODO: Move this to Core::Initialize() (or Core::Start())
	
	if (!GUI::Initialize(screenWidth, screenHeight)) {
		std::cerr << "Initializing GUI failed" << std::endl;
		GraphicsEngine::Destroy();
		SoundEngine::Destroy();
		Core::Destroy();
	}
	
	Room* map = Room::Get();
	if (!map->LoadWorldMap()) {
		std::cerr << "LoadWorldMap failed" << std::endl;
		GraphicsEngine::Destroy();
		SoundEngine::Destroy();
		Core::Destroy();
		return -1;
	}

	GUI* gui = GUI::Get();
	/*GFX::rect consoleRect = { 0, 200, 1100, 484 };
	OutputConsole* console = new OutputConsole(consoleRect);
	consoleRect.h = 20;
	consoleRect.y = 685;
	InputConsole* inputConsole = new InputConsole(consoleRect);
*/

	uint16 lastMouseX = 0;
	uint16 lastMouseY = 0;
	//uint16 downMouseX = 0;
	//uint16 downMouseY = 0;
	if (map != NULL) {
		SDL_Event event;
		bool quitting = false;
		while (!quitting) {
			while (SDL_PollEvent(&event) != 0) {
				switch (event.type) {
					case SDL_MOUSEBUTTONDOWN:
						//downMouseX = event.button.x;
						//downMouseY = event.button.y;
						gui->MouseDown(event.button.x, event.button.y);
						break;
					case SDL_MOUSEBUTTONUP:
						/*if (downMouseX == event.button.x
							&& downMouseY == event.button.y)
							map->Clicked(event.button.x, event.button.y);*/
						gui->MouseUp(event.button.x, event.button.y);

						break;
					case SDL_MOUSEMOTION:
						lastMouseX = event.motion.x;
						lastMouseY = event.motion.y;

						break;
					case SDL_KEYDOWN: {
						/*if (event.key.keysym.sym == SDLK_ESCAPE) {
							console->Toggle();
							inputConsole->Toggle();
						} else if (console->IsActive())
							inputConsole->HandleInput(event.key.keysym.sym);
						else */{
							switch (event.key.keysym.sym) {
								/*case SDLK_o:
									map->ToggleOverlays();
									break;
								*/
								// TODO: Move to GUI class
								case SDLK_h:
									map->ToggleGUI();
									break;
								case SDLK_a:
									map->ToggleAnimations();
									break;
								case SDLK_p:
									map->TogglePolygons();
									break;
								case SDLK_w:
									map->LoadWorldMap();
									break;
								case SDLK_s:
									map->ToggleSearchMap();
									break;
								case SDLK_n:
									map->ToggleDayNight();
									break;
								case SDLK_q:
									quitting = true;
									break;
								case SDLK_1:
									GUI::Get()->ToggleWindow(1);
									break;
								case SDLK_2:
									GUI::Get()->ToggleWindow(2);
									break;
								case SDLK_3:
									GUI::Get()->ToggleWindow(3);
									break;
								case SDLK_4:
									GUI::Get()->ToggleWindow(4);
									break;
								case SDLK_SPACE:
									Core::Get()->TogglePause();
									break;
								default:
									break;
							}
						}
					}
					break;

					case SDL_QUIT:
						quitting = true;
						break;
					default:
						break;
				}
			}

			// TODO: When MouseOver() doesn't draw anymore, reorder
			// these three calls. Draw() should be the last.
			gui->Draw();

			// TODO: needs to be called at every loop, not only when the mouse
			// is moved
			gui->MouseMoved(lastMouseX, lastMouseY);

			/*console->Draw();
			inputConsole->Draw();*/
			Core::Get()->UpdateLogic(!sNoScripts);

			GraphicsEngine::Get()->Flip();
			Timer::Wait(50);
		}
	}

	//delete console;
	// delete inputConsole;
	GUI::Destroy();
	Core::Destroy();
	GraphicsEngine::Destroy();
	SoundEngine::Destroy();

	return 0;
}
