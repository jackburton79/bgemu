#include "Core.h"
#include "GraphicsEngine.h"
#include "PathFind.h"

#include "SDL.h"

#include <getopt.h>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stack>

enum MapType {
	kRandomMap,
	kMazeMap,
	kRoomsMap
};

static int sDebug = 0;
static int sRandom = 0;

Bitmap* gMap;
Bitmap* gSearchMap;
Bitmap* gBitmap;

const int16 gNumRowsMap = 620;
const int16 gNumColumnsMap = 620;
const int kBlockSize = 20;

uint32 gRed;
uint32 gGreen;

const int kBlackIndex = 0;
const int kWhiteIndex = 1;

const int kPassable = 0;
const int kWall = 1;

static int sStep = 2;
static MapType sMapType = kMazeMap;

static
struct option sLongOptions[] = {
		{ "random", no_argument, NULL, 'r' },
		{ "maptype", required_argument, NULL, 0 },
		{ "debug", no_argument, &sDebug, 'D' },
		{ "step", required_argument, NULL, 's' },
		{ 0, 0, 0, 0 }
};



struct Room {
	int x;
	int y;
	int width;
	int height;

	int
	CenterX() const
	{
		return x + width / 2;
	}

	int
	CenterY() const
	{
		return y + height / 2;
	}
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


static
bool
Intersects(const Room& a, const Room& b)
{
	return a.x < b.x + b.width && a.x + a.width > b.x && a.y < b.y + b.height
			&& a.y + a.height > b.y;
}

static
void
CreateRoom(std::vector<std::vector<unsigned char>>& map, const Room& room,
			int kPassable)
{
	for (int y = room.y; y < room.y + room.height; ++y) {
		for (int x = room.x; x < room.x + room.width; ++x) {
			map[y][x] = kPassable;
		}
	}
}

static
void
CreateHorizontalCorridor(std::vector<std::vector<unsigned char>>& map, int x1,
							int x2, int y, int kPassable)
{
	if (x1 > x2)
		std::swap(x1, x2);

	for (int x = x1; x <= x2; ++x)
		map[y][x] = kPassable;
}

static
void
CreateVerticalCorridor(std::vector<std::vector<unsigned char>>& map, int y1,
						int y2, int x, int kPassable)
{
	if (y1 > y2)
		std::swap(y1, y2);

	for (int y = y1; y <= y2; ++y)
		map[y][x] = kPassable;
}


static void
InitializeRoomsMap(std::vector<std::vector<uint8>> &map, int cols, int rows)
{
	std::vector<Room> rooms;

	const int kNumRooms = 20;

	for (int i = 0; i < kNumRooms; ++i) {
		Room room;

		room.width = Core::RandomNumber(4, 8);
		room.height = Core::RandomNumber(4, 8);

		room.x = Core::RandomNumber(1, cols - room.width - 2);

		room.y = Core::RandomNumber(1, rows - room.height - 2);

		bool overlaps = false;

		for (const auto& other : rooms) {
			if (Intersects(room, other)) {
				overlaps = true;
				break;
			}
		}

		if (overlaps)
			continue;

		CreateRoom(map, room, kPassable);
		rooms.push_back(room);
	}

	for (size_t i = 1; i < rooms.size(); ++i) {
		const Room& prev = rooms[i - 1];
		const Room& curr = rooms[i];

		int x1 = prev.CenterX();
		int y1 = prev.CenterY();
		int x2 = curr.CenterX();
		int y2 = curr.CenterY();

		if (Core::RandomNumber(0, 1)) {
			CreateHorizontalCorridor(map, x1, x2, y1, kPassable);
			CreateVerticalCorridor(map, y1, y2, x2, kPassable);
		} else {
			CreateVerticalCorridor(map, y1, y2, x1, kPassable);
			CreateHorizontalCorridor(map, x1, x2, y2, kPassable);
		}
	}
}


static void
InitializeMazeMap(std::vector<std::vector<uint8>> &maze, int cols, int rows)
{
	// Recursive Backtracker
	std::stack<IE::point> stack;
	IE::point start = { 1, 1 };

	maze[start.y][start.x] = kPassable;
	stack.push(start);

	const IE::point directions[] =
	{
		{ -2, 0 },
		{ 2, 0 },
		{ 0, -2 },
		{ 0, 2 }
	};

	while (!stack.empty()) {
		IE::point current = stack.top();

		std::vector<IE::point> candidates;
		for (const auto& dir : directions) {
			IE::point next =
				{ int16(current.x + dir.x), int16(current.y + dir.y) };

			if (next.x <= 0 || next.y <= 0 || next.x >= cols - 1
					|| next.y >= rows - 1)
				continue;

			if (maze[next.y][next.x] == kWall)
				candidates.push_back(next);
		}

		if (candidates.empty()) {
			stack.pop();
			continue;
		}

		IE::point next = candidates[Core::RandomNumber(0, static_cast<int>(candidates.size()) - 1)];

		const int wallX = (current.x + next.x) / 2;
		const int wallY = (current.y + next.y) / 2;

		maze[wallY][wallX] = kPassable;
		maze[next.y][next.x] = kPassable;

		stack.push(next);
	}
}


static void
InitializeSearchMap()
{
	const int rows = gNumRowsMap / kBlockSize;
	const int cols = gNumColumnsMap / kBlockSize;

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

	// Full wall
	std::vector<std::vector<uint8>> maze(rows, std::vector<uint8>(cols, kWall));

	if (sMapType == kMazeMap)
		InitializeMazeMap(maze, cols, rows);
	else if (sMapType == kRoomsMap)
		InitializeRoomsMap(maze, cols, rows);

	// Copy into bitmap and search map
	for (int r = 0; r < rows; r++) {
		for (int c = 0; c < cols; c++) {
			gSearchMap->PutPixel(c, r, maze[r][c]);

			int color = kWhiteIndex;
			if (maze[r][c] == kWall)
				color = Core::RandomNumber(2, 4);

			GFX::rect rect(c * kBlockSize, r * kBlockSize,
							c * kBlockSize + kBlockSize,
							r * kBlockSize + kBlockSize);

			gMap->FillRect(rect, color);
		}
	}
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


static IE::point
RandomWalkablePoint()
{
	IE::point pt;

	do {
		int cellX = Core::RandomNumber(0, gNumColumnsMap / kBlockSize - 1);

		int cellY = Core::RandomNumber(0, gNumRowsMap / kBlockSize - 1);

		pt.x = cellX * kBlockSize + kBlockSize / 2;
		pt.y = cellY * kBlockSize + kBlockSize / 2;

	} while (!IsWalkable(pt));

	return pt;
}


static bool
NewPath(Path& p, IE::point& start, IE::point& end)
{
	auto startTime = std::chrono::high_resolution_clock::now();
	bool found = true;
	try {
		p.Set(start, end, IsWalkable);
	} catch (const PathNotFoundException& e) {
		found = false;
	}

	auto endTime = std::chrono::high_resolution_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
	if (!found)
		std::cout << "Path not found (" << elapsed.count() << "us)" << std::endl;
	else
		std::cout << "Path found (" << elapsed.count() << " us)" << std::endl;

	p.Statistics().Dump();

	return found;
}


static bool
ResetState(Path& p, Bitmap* bitmap, IE::point& start, IE::point& end)
{
	InitializeSearchMap();

	// skip non walkable points
	start = RandomWalkablePoint();
	end = RandomWalkablePoint();

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
			case 0:
				if (::strcmp(sLongOptions[optIndex].name, "maptype") == 0) {
					if (::strcmp(optarg, "maze") == 0)
						sMapType = kMazeMap;
					else if (::strcmp(optarg, "rooms") == 0)
						sMapType = kRoomsMap;
				}
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



