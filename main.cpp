#include "Archive.h"
#include "Bitmap.h"
#include "BamResource.h"
#include "Core.h"
#include "GraphicsEngine.h"
#include "MOSResource.h"
#include "ResManager.h"
#include "Room.h"
#include "TisResource.h"
#include "WMAPResource.h"
#include "Stream.h"

#include <getopt.h>
#include <SDL.h>

static int sList = 0;
static const char *sPath = "../BG";
static const char *sRoomName = NULL;

static
struct option sLongOptions[] = {
		{ "list", no_argument, &sList, 1 },
		{ "path", required_argument, 0, 'p'},
		{ 0, 0, 0, 0 }
};


void
ParseArgs(int argc, char **argv)
{
	int optIndex = 0;
	int c = 0;
	while ((c = getopt_long(argc, argv, "p:l",
				sLongOptions, &optIndex)) != -1) {
		switch (c) {
			case 'l':
				break;
			case 'p':
				sPath = optarg;
				break;
			default:
				break;
		}
	}

	if (optIndex < argc)
		sRoomName = argv[argc - 1];
}


int
main(int argc, char **argv)
{
	ParseArgs(argc, argv);

	try {
		if (!gResManager->Initialize(sPath)) {
			printf("Failed to initialize Resource Manager!\n");
			return -1;
		}
	} catch (...) {
		return -1;
	}

	if (sList) {
		gResManager->PrintResources();
		return 0;
	}

	if (sRoomName == NULL) {
		printf("No room name specified. Exiting...\n");
		return 0;
	}

	if (!GraphicsEngine::Initialize()) {
		printf("Failed to initialize Graphics Engine\n");
		return -1;
	}

	GraphicsEngine* graphicsEngine = GraphicsEngine::Get();

	WMAPResource* worldMap = gResManager->GetWMAP("WORLDMAP");

	worldmap_entry entry;
	if (!worldMap->GetWorldMap(entry))
		return -1;

	MOSResource* background = gResManager->GetMOS(entry.background_mos);
	Bitmap *bitmap = background->Image();
	//SDL_SaveBMP(bitmap->Surface(), "test.bmp");
	graphicsEngine->SetVideoMode(bitmap->Width(), bitmap->Height(), 16, 0);
	SDL_Event event;
	bool quitting = false;
	while (!quitting) {
		while (SDL_PollEvent(&event) != 0) {
			switch (event.type) {
				case SDL_KEYDOWN: {
					switch (event.key.keysym.sym) {
						case SDLK_q:
							quitting = true;
							break;
						default:
							break;
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
		graphicsEngine->BlitToScreen(bitmap, NULL, NULL);
		graphicsEngine->Flip();
		SDL_Delay(100);
	}

	gResManager->ReleaseResource(background);
	gResManager->ReleaseResource(worldMap);
	/*

	graphicsEngine->SetWindowCaption(sRoomName);

	if (!Core::Get()->EnterArea(sRoomName)) {;
		printf("EnterArea failed\n");
		GraphicsEngine::Destroy();
		return -1;
	}

	Room *map = Core::Get()->CurrentArea();

	GFX::rect rect = graphicsEngine->VideoArea();
	map->SetViewPort(rect);
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
						break;
					case SDL_MOUSEBUTTONUP:
						if (downMouseX == event.button.x
							&& downMouseY == event.button.y)
							map->Clicked(event.button.x, event.button.y);
						break;
					case SDL_MOUSEMOTION:
						lastMouseX = event.motion.x;
						lastMouseY = event.motion.y;
						break;
					case SDL_KEYDOWN: {
						switch (event.key.keysym.sym) {
							case SDLK_a:
								map->ToggleAnimations();
								break;
							case SDLK_l:
								map->ToggleLightMap();
								break;
							case SDLK_h:
								map->ToggleHeightMap();
								break;
							case SDLK_s:
								map->ToggleSearchMap();
								break;
							case SDLK_o:
								map->ToggleOverlays();
								break;
							case SDLK_p:
								map->TogglePolygons();
								break;
							case SDLK_q:
								quitting = true;
								break;
							default:
								break;
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
			map->MouseOver(lastMouseX, lastMouseY);
			Core::Get()->UpdateLogic();
			map->Draw(NULL);
			graphicsEngine->Flip();
			SDL_Delay(10);
		}
	}
*/
	//GraphicsEngine::Destroy();

	return 0;
}
