#include "Core.h"
#include "GraphicsEngine.h"
#include "PathFind.h"

#include "SDL.h"

#include <cstdlib>
#include <iostream>

#define DEBUG 0

Bitmap* gMap;
Bitmap* gSearchMap;
Bitmap* gBitmap;

const int16 gNumRowsMap = 600;
const int16 gNumColumnsMap = 600;
const int kBlockSize = 8;

uint32 gRed;
uint32 gGreen;

int kBlackIndex = 0;
int kWhiteIndex = 1;

#if DEBUG
static void
plot_point(const IE::point& pt)
{
	gBitmap->Lock();
	gBitmap->StrokeCircle(pt.x, pt.y, 1, 1);
	gBitmap->Unlock();
	GraphicsEngine::Get()->BlitToScreen(gBitmap, NULL, NULL);
	GraphicsEngine::Get()->Flip();
}
#endif


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

	for (int r = 0; r < numRows; r++) {
		for (int c = 0; c < numColumns; c++) {
			int wall = (Core::RandomNumber(0, 3) ? 0 : 1);
			gSearchMap->PutPixel(c, r, wall);
			int wallColor = (wall ? (Core::RandomNumber(2, 4)) : kWhiteIndex);
			GFX::rect rect(c * kBlockSize, r * kBlockSize,
				c * kBlockSize + kBlockSize, r * kBlockSize + kBlockSize);
			gMap->FillRect(rect, wallColor);
		}
	}
}


static bool
IsWalkable(const IE::point& point)
{
	return gSearchMap->GetPixel(point.x / kBlockSize, point.y / kBlockSize) == 0;
}


static bool
NewPath(PathFinder& p, IE::point& start, IE::point& end)
{
	clock_t startTime = clock();
	p.SetPoints(start, end);

	if (!p.IsEmpty())
		std::cout << "Path found in " << (clock() - startTime) / 1000 << " ms" << std::endl;
	return !p.IsEmpty();
}


static bool
ResetState(PathFinder&p, Bitmap* bitmap, IE::point& start, IE::point& end)
{
	InitializeSearchMap();

	// skip non walkable points
	do {
		start.y = Core::RandomNumber(0, gNumRowsMap - 1);
	} while (!IsWalkable(start));

	do {
		end.y = Core::RandomNumber(0, gNumRowsMap - 1);
	} while (!IsWalkable(end));

	GraphicsEngine::BlitBitmap(gMap, NULL, bitmap, NULL);

	gRed = bitmap->MapColor(255, 0, 0);
	gGreen = bitmap->MapColor(0, 255, 0);
	bitmap->StrokeCircle(start.x, start.y, 8, gRed);
	bitmap->StrokeCircle(end.x, end.y, 8, gRed);
#if DEBUG
	p.SetDebug(plot_point);
#endif
	if (!NewPath(p, start, end))
		return false;

	return true;	
}


int main()
{
#if 0
	::srand(::time(NULL));
#endif
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
	
	PathFinder pathFinder(2, IsWalkable);
	
	IE::point start = { 0, 0 };
	IE::point end = { gNumColumnsMap, gNumRowsMap };
	while (!ResetState(pathFinder, gBitmap, start, end)) {
		std::cout << "Path not found!" << std::endl;	
	}
		
	SDL_Event event;
	bool quitting = false;
	while (!quitting) {
		while (SDL_PollEvent(&event) != 0) {
			switch (event.type) {
				case SDL_KEYDOWN: {
					switch (event.key.keysym.sym) {
						case SDLK_n: {
							while (!ResetState(pathFinder, gBitmap, start, end))
								std::cout << "Path not found!" << std::endl;
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
		if (!pathFinder.IsEmpty()) {
			IE::point point = pathFinder.NextWayPoint();
			gBitmap->Lock();
			gBitmap->FillCircle(point.x, point.y, 3, gGreen);
			gBitmap->Unlock();
		}
		GraphicsEngine::Get()->BlitToScreen(gBitmap, NULL, NULL);
		GraphicsEngine::Get()->Flip();
		SDL_Delay(30);
	}
	
	gSearchMap->Release();
	gMap->Release();
	gBitmap->Release();

	GraphicsEngine::Destroy();
}



