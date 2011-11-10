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

static
struct option sLongOptions[] = {
		{ "list", no_argument, &sList, 1 },
		{ "path", required_argument, 0, 'p'},
		{ 0, 0, 0, 0 }
};


static const char *sPath = "../BG";
static const char *sRoom = NULL;


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
		sRoom = argv[optIndex + 1];
}


int
main(int argc, char **argv)
{
	ParseArgs(argc, argv);

	//chdir(sPath);
	try {
		gResManager->Initialize(sPath);
	} catch (...) {
		return -1;
	}

	if (sList) {
		gResManager->PrintResources();
		return 0;
	}

	if (sRoom == NULL) {
		printf("No room name specified. Exiting...\n");
		return 0;
	}

	SDL_Init(SDL_INIT_VIDEO);
	SDL_Surface *screen = SDL_SetVideoMode(1100, 700, 16, 0);
	SDL_WM_SetCaption(sRoom, NULL);

	World world;
	world.EnterArea(sRoom);
	Room *map = world.CurrentArea();
	uint16 lastMouseX = 0;
	uint16 lastMouseY = 0;
	if (map != NULL) {
		const SDL_Rect mapRect = map->Rect();
		SDL_Rect rect;
		rect.w = screen->w;
		rect.h = screen->h;
		rect.x = 0;
		rect.y = 0;

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
							rect.x -= event.motion.xrel;
							rect.y -= event.motion.yrel;
							if (rect.x < 0)
								rect.x = 0;
							if (rect.y < 0)
								rect.y = 0;
							if (rect.x + rect.w > mapRect.w)
								rect.x = mapRect.w - rect.w;
							if (rect.y + rect.h > mapRect.h)
								rect.y = mapRect.h - rect.h;
						}
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
			map->Draw(screen, rect);
			SDL_Delay(50);
		}

		SDL_FreeSurface(screen);
	}
	SDL_Quit();
	return 0;
}
