#include "Archive.h"
#include "Bitmap.h"
#include "Control.h"
#include "Core.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "InputConsole.h"
#include "OutputConsole.h"
#include "ResManager.h"
#include "Room.h"
#include "WorldMap.h"
#include "WMAPResource.h"

#include <getopt.h>
#include <SDL.h>

static int sList = 0;
static int sNoScripts = 0;
static const char *sPath = "../BG";

static
struct option sLongOptions[] = {
		{ "list", no_argument, &sList, 1 },
		{ "path", required_argument, 0, 'p'},
		{ "noscripts", no_argument, &sNoScripts, 1 },
		{ 0, 0, 0, 0 }
};


void
ParseArgs(int argc, char **argv)
{
	int optIndex = 0;
	int c = 0;
	while ((c = getopt_long(argc, argv, "p:ln",
				sLongOptions, &optIndex)) != -1) {
		switch (c) {
			case 'p':
				sPath = optarg;
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

	if (!gResManager->Initialize(sPath)) {
		std::cerr << "Failed to initialize Resource Manager!" << std::endl;
		return -1;
	}

	if (sList) {
		gResManager->PrintResources();
		return 0;
	}

	if (!Core::Initialize()) {
		std::cerr << "Failed to initialize Core!" << std::endl;
		return -1;
	}

	if (!GraphicsEngine::Initialize()) {
		GraphicsEngine::Destroy();
		std::cerr << "Failed to initialize Graphics Engine!" << std::endl;
		return -1;
	}

	GUI* gui = GUI::Default();
	GraphicsEngine* graphicsEngine = GraphicsEngine::Get();

	Room *map = new Room();

	if (!map->LoadWorldMap()) {
		std::cerr << "LoadWorldMap failed" << std::endl;
		GraphicsEngine::Destroy();
		Core::Destroy();
		return -1;
	}

	graphicsEngine->SetWindowCaption(map->AreaName().CString());
	graphicsEngine->SetVideoMode(640, 480, 16, 0);

	/*GFX::rect consoleRect = { 0, 200, 1100, 484 };
	OutputConsole* console = new OutputConsole(consoleRect);
	consoleRect.h = 20;
	consoleRect.y = 685;
	InputConsole* inputConsole = new InputConsole(consoleRect);
*/

	uint16 lastMouseX = 0;
	uint16 lastMouseY = 0;
	uint16 downMouseX = 0;
	uint16 downMouseY = 0;
	if (map != NULL) {
		SDL_Event event;
		bool quitting = false;
		while (!quitting) {
			while (SDL_PollEvent(&event) != 0) {
				switch (event.type) {
					case SDL_MOUSEBUTTONDOWN:
						downMouseX = event.button.x;
						downMouseY = event.button.y;
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
								case SDLK_a:
									map->ToggleAnimations();
									break;
								case SDLK_p:
									map->TogglePolygons();
									break;
								case SDLK_w:
									map->LoadWorldMap();
									break;
								case SDLK_q:
									quitting = true;
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
			//if (!sNoScripts)
				//Core::Get()->UpdateLogic();

			graphicsEngine->Flip();
			SDL_Delay(10);
		}
	}

	//delete console;
	// delete inputConsole;
	GraphicsEngine::Destroy();
	Core::Destroy();

	return 0;
}
