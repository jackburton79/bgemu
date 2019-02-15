//#include "graphics/Bitmap.h"
#include "GraphicsEngine.h"
#include "PathFind.h"

#include "SDL.h"

#include <cstdlib>
#include <iostream>

Bitmap* gSearchMap;
int gNumRows = 300;
int gNumColumns = 300;

int kRedIndex = 2;

static void
InitializeSearchMap()
{
	gSearchMap = new Bitmap(300, 300, 8);
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
	

	for (int r = 0; r < gNumRows - 16; r+=16) {
		for (int c = 0; c < gNumColumns - 16 ; c+=16) {
			uint8 value = ((::rand() % 2)) ? 0 : 1;
			GFX::rect rect(c, r, c + 16, r + 16);
			gSearchMap->FillRect(rect, value);
		}
	}
}


static bool
IsWalkable(const IE::point& point)
{
	return gSearchMap->GetPixel(point.x, point.y) == 0;
}


int main()
{
	//::srand(clock());
	
	if (!GraphicsEngine::Initialize()) {
		std::cerr << "Failed to initialize Graphics Engine!" << std::endl;
		return -1;
	}

	GraphicsEngine::Get()->SetVideoMode(300, 300, 16, 0);

	Bitmap* bitmap = new Bitmap(300, 300, 16);
	
	InitializeSearchMap();
	
	GraphicsEngine::BlitBitmap(gSearchMap, NULL, bitmap, NULL);
	
	clock_t startTime = clock();
	PathFinder pathFinder(1, IsWalkable);
	IE::point start = { 0, 0 };
	IE::point end = { 299, 299 };
	// skip non walkable points
	while (!IsWalkable(start) && start.x < 299)
		start.x++;
	while (!IsWalkable(end) && end.y > 0)
		end.y--;		
	pathFinder.SetPoints(start, end);
	
	if (!pathFinder.IsEmpty())
		std::cout << "Path found in " << (clock() - startTime) / 1000 << " ms" << std::endl;
	else
		std::cout << "Path not found!" << std::endl;
	
	bitmap->StrokeCircle(start.x, start.y, 5, 10000);
	bitmap->StrokeCircle(end.x, end.y, 5, 10000);
	
	SDL_Event event;
	bool quitting = false;
	while (!quitting) {
		while (SDL_PollEvent(&event) != 0) {
			switch (event.type) {
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
			bitmap->PutPixel(point.x, point.y, 12000);
			bitmap->Unlock();
		}
		GraphicsEngine::Get()->BlitToScreen(bitmap, NULL, NULL);
		GraphicsEngine::Get()->Flip();
		SDL_Delay(50);
	}
	
	gSearchMap->Release();
	bitmap->Release();

	GraphicsEngine::Destroy();
}



