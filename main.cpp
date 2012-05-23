#include "BamResource.h"
#include "GraphicsEngine.h"
#include "ResManager.h"
#include "Room.h"
#include "MveResource.h"
#include "Stream.h"
#include "World.h"

#include <getopt.h>
#include <SDL.h>

static int sList = 0;
static const char *sPath = "../BG";
static const char *sRoomName = NULL;
static int sDumpOverlays = 0;

static
struct option sLongOptions[] = {
		{ "list", no_argument, &sList, 1 },
		{ "dump_overlays", no_argument, &sDumpOverlays, 0 },
		{ "path", required_argument, 0, 'p'},
		{ 0, 0, 0, 0 }
};


void
ParseArgs(int argc, char **argv)
{
	int optIndex = 0;
	int c = 0;
	while ((c = getopt_long(argc, argv, "dp:l",
				sLongOptions, &optIndex)) != -1) {
		switch (c) {
			case 'l':
				break;
			case 'p':
				sPath = optarg;
				break;
			case 'd':
				sDumpOverlays = 1;
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

	SDL_Init(SDL_INIT_VIDEO);
	SDL_Surface *screen = SDL_SetVideoMode(1100, 700, 16, 0);
	SDL_WM_SetCaption(sRoomName, NULL);

	World world;
	world.EnterArea(sRoomName);
	Room *map = world.CurrentArea();
	if (sDumpOverlays)
		map->DumpOverlays("/home/stefano/dumps");
	SDL_Rect rect = { 0, 0, screen->w, screen->h };
	map->SetViewPort(rect);
	uint16 lastMouseX = 0;
	uint16 lastMouseY = 0;
	if (map != NULL) {
		SDL_Event event;
		bool quitting = false;
		bool dragging = false;
		while (!quitting) {
			while (SDL_PollEvent(&event) != 0) {
				switch (event.type) {
					case SDL_MOUSEBUTTONDOWN:
						lastMouseX = event.button.x;
						lastMouseY = event.button.y;
						dragging = true;
						break;
					case SDL_MOUSEBUTTONUP:
						dragging = false;
						if (lastMouseX == event.button.x
							&& lastMouseY == event.button.y)
							map->Clicked(event.button.x, event.button.y);
						break;
					case SDL_MOUSEMOTION:
						if (dragging) {
							SDL_Rect rect = map->ViewPort();
							rect.x -= event.motion.xrel;
							rect.y -= event.motion.yrel;
							map->SetViewPort(rect);
						} else
							map->MouseOver(event.motion.x, event.motion.y);

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
			map->Draw(screen);
			SDL_Delay(100);
		}

		SDL_FreeSurface(screen);
	}
	SDL_Quit();
	return 0;
}
