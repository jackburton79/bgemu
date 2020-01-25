//#include "graphics/Bitmap.h"
#include "Core.h"
#include "GraphicsEngine.h"
#include "PathFind.h"

#include "SDL.h"

#include <cstdlib>
#include <iostream>

Bitmap* gSearchMap;
int16 gNumRows = 600;
int16 gNumColumns = 600;

const int kBlockSize = 16;

int kRedIndex = 2;

uint32 gRed;
uint32 gGreen;

static void
InitializeSearchMap()
{
	GFX::Color colors[256];
	colors[0].r = 255;
	colors[0].g = 255;
	colors[0].b = 255;
	colors[0].a = 255;
	colors[1].r = 0;
	colors[1].g = 0;
	colors[1].b = 0;
	colors[1].a = 255;
	colors[kRedIndex].r = 255;
	colors[kRedIndex].g = 0;
	colors[kRedIndex].b = 0;
	colors[kRedIndex].a = 0;
	gSearchMap->SetColors(colors, 0, 3);
	

	for (int r = 0; r < gNumRows - kBlockSize; r += kBlockSize) {
		for (int c = 0; c < gNumColumns - kBlockSize ; c += kBlockSize) {
			uint8 value = ((::rand() % 5)) ? 0 : 1;
			GFX::rect rect(c, r, c + kBlockSize, r + kBlockSize);
			gSearchMap->FillRect(rect, value);
		}
	}
}


static bool
IsWalkable(const IE::point& point)
{
	return gSearchMap->GetPixel(point.x, point.y) == 0;
}


static bool
NewPath(PathFinder& p, IE::point& start, IE::point& end)
{
	clock_t startTime = clock();
	// skip non walkable points
	start.y = Core::RandomNumber(0, gNumRows - 1);
	end.y = Core::RandomNumber(0, gNumRows - 1);
	while (!IsWalkable(start) && start.y < gNumRows - 1)
		start.y++;
	while (!IsWalkable(end) && end.y > 0)
		end.y--;		
	p.SetPoints(start, end);

	if (!p.IsEmpty())
		std::cout << "Path found in " << (clock() - startTime) / 1000 << " ms" << std::endl;
	return !p.IsEmpty();
}


static bool
ResetState(PathFinder&p, Bitmap* bitmap, IE::point& start, IE::point& end)
{
	InitializeSearchMap();

	if (!NewPath(p, start, end))
		return false;

	GraphicsEngine::BlitBitmap(gSearchMap, NULL, bitmap, NULL);
	
	gRed = bitmap->MapColor(255, 0, 0);
	gGreen = bitmap->MapColor(0, 255, 0);
	bitmap->StrokeCircle(start.x, start.y, 5, gRed);
	bitmap->StrokeCircle(end.x, end.y, 5, gRed);
	return true;	
}


int main()
{
	if (!GraphicsEngine::Initialize()) {
		std::cerr << "Failed to initialize Graphics Engine!" << std::endl;
		return -1;
	}

	GraphicsEngine::Get()->SetVideoMode(gNumColumns, gNumRows, 16, 0);

	gSearchMap = new Bitmap(gNumColumns, gNumRows, 8);
	Bitmap* bitmap = new Bitmap(gNumColumns, gNumRows, 16);
	
	PathFinder pathFinder(2, IsWalkable);
	
	IE::point start = { 0, 0 };
	IE::point end = { gNumColumns, gNumRows };	
	if (!ResetState(pathFinder, bitmap, start, end))
		std::cout << "Path not found!" << std::endl;
		
	SDL_Event event;
	bool quitting = false;
	while (!quitting) {
		while (SDL_PollEvent(&event) != 0) {
			switch (event.type) {
				case SDL_KEYDOWN: {
					switch (event.key.keysym.sym) {
						case SDLK_n: {
							if (!ResetState(pathFinder, bitmap, start, end))
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
			bitmap->Lock();
			bitmap->StrokeCircle(point.x, point.y, 2, gGreen);
			bitmap->Unlock();
		}
		GraphicsEngine::Get()->BlitToScreen(bitmap, NULL, NULL);
		GraphicsEngine::Get()->Flip();
		SDL_Delay(30);
	}
	
	gSearchMap->Release();
	bitmap->Release();

	GraphicsEngine::Destroy();
}



