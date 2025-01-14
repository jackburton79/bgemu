#include "Core.h"
#include "GraphicsEngine.h"
#include "PathFind.h"

#include "SDL.h"

#include <getopt.h>
#include <cstdlib>
#include <iostream>

static int sDebug = 0;
static int sRandom = 0;

Bitmap* gMap;
Bitmap* gSearchMap;
Bitmap* gBitmap;

const int16 gNumRowsMap = 600;
const int16 gNumColumnsMap = 600;
const int kBlockSize = 20;

uint32 gRed;
uint32 gGreen;

const int kBlackIndex = 0;
const int kWhiteIndex = 1;

const int kPassable = 0;
const int kWall = 1;

static int sStep = 2;

static
struct option sLongOptions[] = {
		{ "random", no_argument, NULL, 'r' },
		{ "debug", no_argument, &sDebug, 'D' },
		{ "step", required_argument, NULL, 's' },
		{ 0, 0, 0, 0 }
};


static void
plot_point(const IE::point& pt)
{
	gBitmap->Lock();
	gBitmap->StrokeCircle(pt.x, pt.y, 1, 1);
	gBitmap->Unlock();
	GraphicsEngine::Get()->BlitToScreen(gBitmap, NULL, NULL);
	GraphicsEngine::Get()->Update();
}


static void
InitializeSearchMap()
{
	int16 numRows = gNumRowsMap / kBlockSize;
	int16 numColumns = gNumColumnsMap / kBlockSize;

	GFX::Color colors[256];

	colors[kBlackIndex].r = 0;
	colors[kBlackIndex].g = 0;
	colors[kBlackIndex].b = 0;
	colors[kBlackIndex].a = 0;

	colors[kWhiteIndex].r = 255;
	colors[kWhiteIndex].g = 255;
	colors[kWhiteIndex].b = 255;
	colors[kWhiteIndex].a = 0;

	colors[2].r = 16;
	colors[2].g = 16;
	colors[2].b = 16;
	colors[2].a = 0;
	colors[3].r = 36;
	colors[3].g = 20;
	colors[3].b = 20;
	colors[3].a = 0;
	colors[4].r = 56;
	colors[4].g = 47;
	colors[4].b = 47;
	colors[4].a = 0;
	gSearchMap->SetColors(colors, 0, 2);

	gMap->SetColors(colors, 0, 5);
#if 0
	for (int r = 0; r < numRows; r++) {
		bool wallRow = ((r % 2) == 0) ? 0 : 1;
		int exit = Core::RandomNumber(0, numColumns);
		for (int c = 0; c < numColumns; c++) {
			bool wall = wallRow && (c != exit);
			if (wall)
				gSearchMap->PutPixel(c, r, kWall);
			else
				gSearchMap->PutPixel(c, r, kPassable);
			int wallColor = (wall ? kBlackIndex : kWhiteIndex);
			GFX::rect rect(c * kBlockSize, r * kBlockSize,
				c * kBlockSize + kBlockSize, r * kBlockSize + kBlockSize);
			gMap->FillRect(rect, wallColor);
		}
	}
#else
	for (int r = 0; r < numRows; r++) {
		for (int c = 0; c < numColumns; c++) {
			bool isWall = (Core::RandomNumber(0, 3) ? false : true);
			gSearchMap->PutPixel(c, r, isWall ? kWall : kPassable);
			int wallColor = (isWall ? Core::RandomNumber(2, 4) : kWhiteIndex);
			GFX::rect rect(c * kBlockSize, r * kBlockSize,
				c * kBlockSize + kBlockSize, r * kBlockSize + kBlockSize);
			gMap->FillRect(rect, wallColor);
		}
	}
#endif
}


static bool
IsWalkable(const IE::point& point)
{
	if (point.x < 0 || point.y < 0
			|| (point.x / kBlockSize >= (gNumColumnsMap) / kBlockSize)
			|| (point.y / kBlockSize >= (gNumRowsMap) / kBlockSize))
		return false;
	return gSearchMap->GetPixel(point.x / kBlockSize, point.y / kBlockSize) == kPassable;
}


static bool
NewPath(Path& p, IE::point& start, IE::point& end)
{
	clock_t startTime = clock();
	bool found = true;
	try {
		p.Set(start, end, IsWalkable);
	} catch (const PathNotFoundException& e) {
		found = false;
	}
	clock_t elapsed = (clock() - startTime) / 1000;
	if (!found)
		std::cout << "Path not found (" << elapsed << "ms)" << std::endl;
	else
		std::cout << "Path found (" << elapsed << " ms)" << std::endl;
	return found;
}


static bool
ResetState(Path& p, Bitmap* bitmap, IE::point& start, IE::point& end)
{
	InitializeSearchMap();

	// skip non walkable points
	start.y = gNumRowsMap;
	do {
		start.y--;
	} while (!IsWalkable(start));
	end.x -= 5;
	do {
		end.y = Core::RandomNumber(0, gNumRowsMap - 1);
	} while (!IsWalkable(end));

	GraphicsEngine::BlitBitmap(gMap, NULL, bitmap, NULL);

	gRed = bitmap->MapColor(255, 0, 0);
	gGreen = bitmap->MapColor(0, 255, 0);
	bitmap->StrokeCircle(start.x, start.y, 8, gRed);
	bitmap->StrokeCircle(end.x, end.y, 8, gRed);
	if (sDebug)
		PathFinder::SetDebug(plot_point);

	if (!NewPath(p, start, end))
		return false;

	return true;	
}


static void
ParseArgs(int argc, char **argv)
{
	int optIndex = 0;
	int c = 0;
	while ((c = getopt_long(argc, argv, "g:p:Ds:ltnf",
				sLongOptions, &optIndex)) != -1) {
		switch (c) {
			case 'D':
				sDebug = 1;
				break;
			case 'r':
				sRandom = 1;
				break;
			case 's':
				sStep = ::strtol(optarg, NULL, 0);
				break;
			default:
				break;
		}
	}
}


int main(int argc, char **argv)
{
	ParseArgs(argc, argv);

	if (sRandom)
		::srand(::time(NULL));

	if (!GraphicsEngine::Initialize()) {
		std::cerr << "Failed to initialize Graphics Engine!" << std::endl;
		return -1;
	}

	GraphicsEngine::Get()->SetVideoMode(gNumColumnsMap, gNumRowsMap, 16, 0);

	int16 numRows = gNumRowsMap / kBlockSize;
	int16 numColumns = gNumColumnsMap / kBlockSize;

	gSearchMap = new Bitmap(numColumns, numRows, 8);
	gMap = new Bitmap(gNumColumnsMap, gNumRowsMap, 8);
	gBitmap = new Bitmap(gNumColumnsMap, gNumRowsMap, 16);
	
	std::cout << "Step: " << sStep << std::endl;
	Path path;
	
	IE::point start = { 0, 0 };
	IE::point end = { gNumColumnsMap, gNumRowsMap };
	while (!ResetState(path, gBitmap, start, end))
		;
		
	SDL_Event event;
	bool quitting = false;
	while (!quitting) {
		while (SDL_PollEvent(&event) != 0) {
			switch (event.type) {
				case SDL_KEYDOWN: {
					switch (event.key.keysym.sym) {
						case SDLK_n: {
							start = { 0, 0 };
							end = { gNumColumnsMap, gNumRowsMap };
							while (!ResetState(path, gBitmap, start, end))
								;
							break;
						}
						case SDLK_q:
							quitting = true;
							break;
						default:
							break;
					}
					break;
				}
				case SDL_QUIT:
					quitting = true;
					break;
						
				default:
					break;
			}
		}
		if (!path.IsEmpty() && !path.IsEnd()) {
			IE::point point = path.NextStep();
			gBitmap->Lock();
			gBitmap->FillCircle(point.x, point.y, 3, gGreen);
			gBitmap->Unlock();
		}
		GraphicsEngine::Get()->BlitToScreen(gBitmap, NULL, NULL);
		GraphicsEngine::Get()->Update();
		SDL_Delay(10);
	}
	
	gSearchMap->Release();
	gMap->Release();
	gBitmap->Release();

	GraphicsEngine::Destroy();
}



