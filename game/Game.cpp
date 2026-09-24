/*
 * Game.cpp
 *
 *  Created on: 29/mar/2015
 *      Author: stefano
 */

#include "Game.h"

#include "2DAResource.h"
#include "Actor.h"
#include "AnimationFactory.h"
#include "AreaResource.h"
#include "ActionBar.h"
#include "AreaRoom.h"
#include "CharacterBuilder.h"
#include "BamResource.h"
#include "SPLResource.h"
#include "Bitmap.h"
#include "BmpResource.h"
#include "Button.h"
#include "ColorRange.h"
#include "Core.h"
#include "CreResource.h"
#include "Dialog.h"
#include "GameConsole.h"
#include "GamResource.h"
#include "InventoryScreen.h"
#include "JournalScreen.h"
#include "LootWindow.h"
#include "GameTimer.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "ITMResource.h"
#include "Label.h"
#include "Container.h"
#include "Scrollbar.h"
#include "STOResource.h"
#include "Store.h"
#include "MemoryStream.h"
#include "PLTResource.h"
#include "Parsing.h"
#include "Party.h"
#include "RecordScreen.h"
#include "ResManager.h"
#include "RoomBase.h"
#include "SaveLoadScreen.h"
#include "ScreenSupport.h"
#include "SpellbookScreen.h"
#include "StoreScreen.h"
#include "ScreenManager.h"
#include "TextArea.h"
#include "Window.h"


#include <algorithm>
#include <assert.h>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <sys/stat.h>
#include <utility>

#include <SDL.h>

static uint32 sFrames = 0;
static uint32 sLastFrame = 0;
static uint32 sLastTime = 0;

static Game* sGame;

/* static */
Game*
Game::Get()
{
	if (sGame == NULL)
		sGame = new Game();
	return sGame;
}


Game::Game()
	:
	fDialog(NULL),
	fParty(NULL),
	fTempState(NULL),
	// 15 Hz, standard Infinity Engine pace (AI_UPDATE_FREQ)
	// IESDP's documented
	// clock model (docs/iesdp-gh-pages/appendices/timers.htm).
	fDelay(67),
	fTestMode(false),
	fLoot(NULL),
	fBar(NULL),
	fShownCharacter(0),
	fScreens(NULL)
{
	fScreens = new ScreenManager;
	fLoot = new LootWindow;
	fBar = new ActionBar(*this);
	fScreens->Add(new RecordScreen(*this));
	fScreens->Add(new InventoryScreen(*this));
	fScreens->Add(new JournalScreen(*this));
	fScreens->Add(new SaveLoadScreen(*this, true));
	fScreens->Add(new SaveLoadScreen(*this, false));
	fScreens->Add(new SpellbookScreen(*this, false));
	fScreens->Add(new SpellbookScreen(*this, true));
	fScreens->Add(new StoreScreen(*this));
	fTempState = new Game::TempState;
	fAreaCache = new Game::AreaCache;
	fCharBuilder = new CharacterBuilder;
}


Game::~Game()
{
	TerminateDialog();
	delete fParty;
	_ClearNPCs();
	delete fTempState;

	// Same convention as everything else this session is careful to
	// balance, even though it only matters for the ASan leak report at
	// this specific point (the whole process is about to go away
	// regardless).
	_ClearAreaCache();
	delete fAreaCache;
	delete fCharBuilder;
	delete fScreens;
	delete fLoot;
	delete fBar;
}


void
Game::_ClearAreaCache()
{
	for (auto& entry : fAreaCache->areas) {
		for (Actor* actor : entry.second.actors)
			actor->Release();
		gResManager->ReleaseResource(entry.second.area);
	}
	fAreaCache->areas.clear();
}


Game::AreaCache*
Game::GetAreaCache()
{
	return fAreaCache;
}


CharacterBuilder&
Game::GetCharacterBuilder()
{
	return *fCharBuilder;
}


void
DisplayClock(void *param)
{
	std::string clock = GameTimer::GameTimeString();
	GFX::rect frame = GraphicsEngine::Get()->ScreenFrame();
	GUI::Get()->DisplayString(clock.c_str(), frame.x + 10, frame.h - 50, 2500);
}


void
DisplayFrameRate(void* param)
{
	uint32 currentTime = SDL_GetTicks();
	GFX::rect frame = GraphicsEngine::Get()->ScreenFrame();
	char frameRate[32];
	uint32 numFrames = sFrames - sLastFrame;
	uint32 elapsedTime = currentTime - sLastTime;
	if (elapsedTime == 0)
		elapsedTime = 1;
	snprintf(frameRate, sizeof(frameRate), "FPS: %d", 1000 * numFrames / elapsedTime);
	GUI::Get()->DisplayString(frameRate, frame.x + 70, 20, 1000);
	sLastFrame = sFrames;
	sLastTime = currentTime;
}


void
Game::Loop(bool noNewGame, bool executeScripts)
{
	// TODO: Move this ? where ?
	if (!InitColorRanges()) {
		std::cerr << "Initializing color range failed" << std::endl;
		// Not a bad error, we can still continue
	}

	std::cout << "Game::Loop()" << std::endl;
	if (!GUI::Initialize(GraphicsEngine::Get()->ScreenFrame().w,
						 GraphicsEngine::Get()->ScreenFrame().h)) {
		throw std::runtime_error("Initializing GUI failed");
	}

	GFX::rect screenRect = GraphicsEngine::Get()->ScreenFrame();
	GUI* gui = GUI::Get();
	GFX::rect consoleRect(
			0,
			0,
			screenRect.w,
			screenRect.h);

	GameConsole* inputConsole = NULL;
	std::cout << "Setting up console...";
	std::flush(std::cout);
	inputConsole = new GameConsole(consoleRect, false);
	std::cout << "OK!" << std::endl;
	// Redirection of stdout into the on-screen console buffer stays
	// off by default (toggle with 'd' at runtime) - keeping stdout on
	// the normal stream/terminal is what the headless ASan test
	// workflow relies on.
	if (inputConsole != NULL)
		inputConsole->Initialize();
	std::cout << "OK!" << std::endl;

	bool quitting = false;

	if (TestMode()) {
		GUI::Get()->Load("GUITEST");
		// Parsing tests
		Parser::Test();
	} else {
		try {
			CreateParty();
		} catch (...) {
			throw std::runtime_error("Error creating player!");
		}
		_LoadStartingNPCs();
		if (!fStartingArea.empty())
			Core::Get()->LoadArea(fStartingArea.c_str(), "", "");
		else if (noNewGame)
			Core::Get()->LoadWorldMap();
		else
			LoadStartingArea();

		if (!fExecFile.empty()) {
			_RunExecFile(inputConsole);
			quitting = true;
		}
	}


	std::cout << "Game: Started game loop." << std::endl;
	//int clockTimer = Timer::AddPeriodicTimer(8000, DisplayClock, NULL);
	//int fpsTimer = Timer::AddPeriodicTimer(1000, DisplayFrameRate, NULL);

	SDL_Event event;
	while (!quitting) {
		uint32 startTicks = Timer::Ticks();
		while (SDL_PollEvent(&event) != 0) {
			RoomBase* room = Core::Get()->CurrentRoom();
			switch (event.type) {
				case SDL_USEREVENT: {
					// Used to handle timers
					void (*event_func)(void*) = (void (*)(void*))event.user.data1;
					event_func(event.user.data2);
					break;
				}
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == SDL_BUTTON_RIGHT)
						gui->RightMouseDown(event.button.x, event.button.y);
					else
						gui->MouseDown(event.button.x, event.button.y);
					break;
				case SDL_MOUSEBUTTONUP:
					// A right click was delivered on the way down (or fell
					// back to MouseDown there); no matching MouseUp so a
					// Button doesn't also fire its normal left-click action.
					if (event.button.button != SDL_BUTTON_RIGHT)
						gui->MouseUp(event.button.x, event.button.y);
					break;
				case SDL_MOUSEMOTION:
					gui->MouseMoved(event.motion.x, event.motion.y);
					break;
				case SDL_KEYDOWN: {
					if (event.key.keysym.sym == SDLK_ESCAPE) {
						if (inputConsole != NULL)
							inputConsole->Toggle();
					} else if (inputConsole != NULL && inputConsole->IsActive()) {
						if (event.key.keysym.scancode < 0x80 && event.key.keysym.scancode > 0) {
							uint8 key = event.key.keysym.sym;
							if (event.key.keysym.mod & (KMOD_LSHIFT|KMOD_RSHIFT))
								key -= 32;
							if (inputConsole != NULL)
								inputConsole->HandleInput(key);
						}
					} else {
						switch (event.key.keysym.sym) {
							case SDLK_o:
								if (room != NULL)
									room->ToggleOverlays();
								break;
							case SDLK_d:
								if (inputConsole != NULL) {
									if (inputConsole->HasOutputRedirected())
										inputConsole->DisableRedirect();
									else
										inputConsole->EnableRedirect();
									break;
								}
							case SDLK_h:
								ToggleHUD();
								break;
							case SDLK_a:
								if (room != NULL)
									room->ToggleAnimations();
								break;
							case SDLK_p:
								if (room != NULL)
									room->TogglePolygons();
								break;
							case SDLK_w:
								if (Core::Get()->IsWorldMap())
									Core::Get()->ReturnFromWorldMap();
								else
									Core::Get()->LoadWorldMap();
								break;
							case SDLK_s:
								if (room != NULL)
									room->ToggleSearchMap();
								break;
							case SDLK_n:
								ToggleDayNight();
								break;
							case SDLK_i:
								fScreens->Toggle<InventoryScreen>();
								break;
							case SDLK_r:
								fScreens->Toggle<RecordScreen>();
								break;
							case SDLK_j:
								fScreens->Toggle<JournalScreen>();
								break;
							case SDLK_k:
								fScreens->Toggle("GUIMG");
								break;
							case SDLK_F5:
								fScreens->Toggle("GUISAVE");
								break;
							case SDLK_F9:
								fScreens->Toggle("GUILOAD");
								break;
							case SDLK_q:
								quitting = true;
								break;
							case SDLK_PLUS:
							{
								fDelay -= 17;
								if (fDelay <= 0)
									fDelay = 0;
								break;
							}
							case SDLK_MINUS:
								fDelay += 17;
								break;
							// Party member selection (1-6, real BG2's own
							// number-key convention)
							case SDLK_1:
								SelectPartyMember(0);
								break;
							case SDLK_2:
								SelectPartyMember(1);
								break;
							case SDLK_3:
								SelectPartyMember(2);
								break;
							case SDLK_4:
								SelectPartyMember(3);
								break;
							case SDLK_5:
								SelectPartyMember(4);
								break;
							case SDLK_6:
								SelectPartyMember(5);
								break;
							case SDLK_SPACE:
								Core::Get()->TogglePause();
								break;
							case SDLK_TAB:
								ToggleHUD();
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

		gui->Draw();

		if (inputConsole != NULL)
			inputConsole->Draw();
		if (!TestMode())
			Core::Get()->UpdateLogic(executeScripts);
		GraphicsEngine::Get()->Update();

		sFrames++;
		Timer::WaitSync(startTicks, fDelay);
	}

	//Timer::RemovePeriodicTimer(clockTimer);
	//Timer::RemovePeriodicTimer(fpsTimer);

	delete inputConsole;

	std::cout << "Game: Input loop stopped." << std::endl;

	// Before GUI::Destroy() below tears down every Window: the current
	// room swaps *itself* into its own window as a Control (AreaRoom's/
	// WorldMap's own constructors)
	Core::Get()->UnloadCurrentRoom();

	GUI::Destroy();
	GameTimer::DisposeTimers();
}


void
Game::CreateParty()
{
	assert(fParty == NULL);
	IE::point point = { 20, 20 };
	fParty = new ::Party();

	if (!fCharacterSpec.empty() && _CreateCharacterFromSpec(point))
		return;

	if (!fStartingPartyMembers.empty()) {
		for (const std::string& name : fStartingPartyMembers)
			fParty->AddActor(new Actor(name.c_str(), point, 0));
		return;
	}

	// No -P/--party override given: fall back to the original hardcoded
	// default party. TODO: a real character-creation flow (race/class/
	// stats/kit) is still missing - this is only "pick existing CREs to
	// start with", not "create a character from scratch".
	if (Core::Get()->Game() == game::GAME_BALDURSGATE)
		fParty->AddActor(new Actor("AJANTI", point, 0));
	else {
		fParty->AddActor(new Actor("ANOMEN10", point, 0));
	}
}


void
Game::SetStartingPartyMembers(const std::vector<std::string>& names)
{
	fStartingPartyMembers = names;
}


void
Game::SetCharacterSpec(const char* path)
{
	fCharacterSpec = path != NULL ? path : "";
}


bool
Game::_CreateCharacterFromSpec(const IE::point& position)
{
	std::ifstream file(fCharacterSpec);
	if (!file) {
		std::cerr << "character spec: cannot open " << fCharacterSpec << std::endl;
		return false;
	}

	CharacterBuilder& builder = *fCharBuilder;
	builder.Reset();
	bool doRoll = false;
	static const char* kAbilityNames[] = { "str", "dex", "con", "int", "wis", "chr" };

	std::string line;
	while (std::getline(file, line)) {
		size_t hash = line.find('#');
		if (hash != std::string::npos)
			line.erase(hash);
		std::istringstream stream(line);
		std::string field, value;
		if (!(stream >> field))
			continue;
		stream >> value;
		for (char& c : field) c = (char)tolower((unsigned char)c);

		if (field == "roll") {
			doRoll = true;
		} else if (field == "gender") {
			builder.SetGender(value);
		} else if (field == "race") {
			builder.SetRace(value);
		} else if (field == "class") {
			builder.SetClass(value);
		} else if (field == "kit") {
			builder.SetKit(value);
		} else if (field == "alignment") {
			builder.SetAlignment(value);
		} else if (field == "name") {
			// The rest of the line, so names can contain spaces.
			std::string rest;
			std::getline(stream, rest);
			builder.SetName(value + rest);
		} else if (field == "portrait") {
			builder.SetPortraits(value + "S", value + "M");
		} else if (field == "portrait_small") {
			builder.SetPortraits(value, builder.PortraitLarge());
		} else if (field == "portrait_large") {
			builder.SetPortraits(builder.PortraitSmall(), value);
		} else if (field.rfind("color_", 0) == 0) {
			builder.SetColor(field.substr(6), atoi(value.c_str()));
		} else if (field == "spell") {
			if (!builder.AddSpell(value))
				std::cerr << "character spec: unknown spell " << value << std::endl;
		} else if (field == "skill_openlocks") {
			builder.SetThiefSkill("openlocks", atoi(value.c_str()));
		} else if (field == "skill_findtraps") {
			builder.SetThiefSkill("findtraps", atoi(value.c_str()));
		} else {
			for (int i = 0; i < CharacterBuilder::kNumAbilities; i++) {
				if (field == kAbilityNames[i])
					builder.SetAbility(i, atoi(value.c_str()));
			}
		}
	}

	if (doRoll)
		builder.RollAbilities();

	std::vector<std::string> problems;
	if (!builder.IsComplete(problems)) {
		std::cerr << "character spec: incomplete -" << std::endl;
		for (const std::string& p : problems)
			std::cerr << "  " << p << std::endl;
		return false;
	}

	std::vector<uint8> creData;
	if (!builder.BuildCREData(creData))
		return false;

	MemoryStream stream(creData.data(), creData.size(), false);
	CREResource* cre = new CREResource(res_ref("PLAYER1"));
	cre->Acquire(); // resources start at refcount 0
	if (!cre->Load(&stream, 0, creData.size())) {
		gResManager->ReleaseResource(cre); // 1 -> 0, deleted
		return false;
	}
	cre->Init();
	gResManager->InjectResource(res_ref("PLAYER1"), RES_CRE, cre);
	gResManager->ReleaseResource(cre); // drop our ref; InjectResource holds its own

	Actor* player = new Actor("PLAYER1", position, 0);
	if (!builder.Name().empty())
		player->SetLongName(builder.Name().c_str());
	fParty->AddActor(player);

	std::cout << "Created character:" << std::endl;
	builder.Print();
	return true;
}


void
Game::SetExecFile(const char* path)
{
	fExecFile = path != NULL ? path : "";
}


void
Game::SetStartingArea(const char* areaName)
{
	fStartingArea = areaName != NULL ? areaName : "";
}


ScreenManager&
Game::Screens()
{
	return *fScreens;
}


ActionBar&
Game::Bar()
{
	return *fBar;
}


LootWindow&
Game::Loot()
{
	return *fLoot;
}


/* static */
void
Game::CloseContainerWindowIfAny()
{
	if (sGame != NULL)
		sGame->fLoot->Close();
}


// Shared with gui/GUI.cpp's own HUD command bar
const CommandBarButton kCommandBarButtons[9] = {
	{ 1, [] { Core::Get()->LoadWorldMap(); } },
	{ 2, [] { Game::Get()->Screens().Toggle<JournalScreen>(); } },
	{ 3, [] { Game::Get()->Screens().Toggle<InventoryScreen>(); } },
	{ 4, [] { Game::Get()->Screens().Toggle<RecordScreen>(); } },
	{ 5, [] { Game::Get()->Screens().Toggle("GUIMG"); } },
	{ 6, [] { Game::Get()->Screens().Toggle("GUIPR"); } },
	{ 7, [] { Game::Get()->Screens().Toggle("GUISAVE"); } },
	{ 9, [] { Core::Get()->TogglePause(); } },
	{ 11, [] { Game::Get()->TriggerRest(); } }
};


// The left-hand command icon strip (ids 0-8, image GUILSOP) + Rest
// button (id 9, GUIRSBUT) appear identically - same position, same
// cycle - in every full-screen panel's own window 0 (GUIINV/GUIREC/
// GUIJRNL/GUIMG/GUIPR all confirmed via a real CHU dump to share this
// exact layout), duplicating GUIW's own WINDOW_COMMANDS bar so the
// player can still switch screens/rest while one of them is open. Ids
// 1-7 come from kCommandBarButtons above; Rest gets its own entry here
// since its local id differs from GUIW's (9 vs. 11).
static const struct { uint32 controlID; void (*action)(); }
kAuxCommandBarButtonsExtra[] = {
	{ 9, [] { Game::Get()->TriggerRest(); } },
};


/* static */
void
Game::AuxCommandBarInvoked(uint32 controlID)
{
	for (const auto& button : kCommandBarButtons) {
		if (button.controlID == controlID) {
			button.action();
			return;
		}
	}
	for (const auto& button : kAuxCommandBarButtonsExtra) {
		if (button.controlID == controlID) {
			button.action();
			return;
		}
	}
}




void
Game::TriggerRest()
{
	if (fParty == NULL || fParty->CountActors() == 0)
		return;

	action_params* params = new action_params;
	params->id = 230; // RESTPARTY
	fParty->ActorAt(0)->AddAction(params);
	params->Release();
}


// Real BG2 numbers save files per slot; every save (the Save/Load screens'
// and SAVEGAME(190)'s) is Game::SaveSlotPath(index), under
// Game::SaveDirectory().


void
Game::SetSaveDirectory(const std::string& path)
{
	fSaveDirectory = path;
}


const std::string&
Game::SaveDirectory() const
{
	return fSaveDirectory;
}


std::string
Game::SaveSlotPath(uint32 index) const
{
	return fSaveDirectory + "/savegame_slot" + std::to_string(index) + ".gam";
}




Actor*
Game::ShownActor() const
{
	if (fParty == NULL || fParty->CountActors() == 0)
		return NULL;
	uint16 index = fShownCharacter < fParty->CountActors() ? fShownCharacter : 0;
	return fParty->ActorAt(index);
}


void
Game::SetShownActor(Actor* actor)
{
	for (uint32 i = 0; fParty != NULL && i < fParty->CountActors(); i++) {
		if (fParty->ActorAt(i) == actor)
			fShownCharacter = static_cast<uint16>(i);
	}
}


void
Game::ShowCharacter(uint16 partyIndex)
{
	// The portrait column drives the same "which member is current"
	// state the world selection (number keys 1-6) uses.
	SelectPartyMember(partyIndex);
}


// Re-populates whichever of the character screens are open for
// the current fShownCharacter - called after any change of who's shown.
void
Game::_RefreshCharacterScreens()
{
	RefreshHUDPortraits();
	fScreens->ShownCharacterChanged();
}


// Fills a portrait column's buttons (ids 0..count-1) with the party's
// small portraits, clearing the rest. Same column layout on the HUD
// (GUIW window 1, 6 slots) and the Inventory/Record side panel (GUIINV/
// GUIREC window 1, 4 slots).
void
Game::UpdatePortraitColumn(Window* window, uint32 count)
{
	if (window == NULL || fParty == NULL)
		return;

	for (uint32 i = 0; i < count; i++) {
		Button* button = dynamic_cast<Button*>(window->GetControlByID(i));
		if (button == NULL)
			continue;
		Bitmap* portrait = NULL;
		Actor* member = i < fParty->CountActors() ? fParty->ActorAt(i) : NULL;
		res_ref portraitRef = member != NULL && member->CRE() != NULL
			? member->CRE()->SmallPortrait() : res_ref("");
		if (portraitRef.CString()[0] != '\0') {
			BMPResource* bmp = gResManager->GetBMP(portraitRef);
			if (bmp != NULL) {
				portrait = bmp->Image();
				gResManager->ReleaseResource(bmp);
			}
		}
		button->SetIcon(portrait, false);
		button->SetHighlighted(member != NULL && i == fShownCharacter);
	}
}


// The HUD portrait bar (GUIW's WINDOW_PLAYER_SLOTS) - populated on area
// entry and re-populated whenever who's selected/in the party changes.
void
Game::RefreshHUDPortraits()
{
	UpdatePortraitColumn(GUI::Get()->GetWindow(GUI::WINDOW_PLAYER_SLOTS), 6);
	fBar->Refresh();
}


// Runs every non-blank, non-'#'-comment line of fExecFile as a GameConsole
// command, in order, printing each before running it (so the transcript
// is self-documenting) - console commands already print their own output
// to stdout (unredirected by default, see GameConsole's constructor
// comment), which is exactly what a headless ASan test run captures.
void
Game::_RunExecFile(GameConsole* console)
{
	std::ifstream file(fExecFile.c_str());
	if (!file.is_open()) {
		std::cerr << "Game::_RunExecFile(): cannot open " << fExecFile << std::endl;
		return;
	}

	std::cout << "Game: running exec-file " << fExecFile << std::endl;
	std::string line;
	while (std::getline(file, line)) {
		if (!line.empty() && line.back() == '\r')
			line.pop_back();

		size_t start = line.find_first_not_of(" \t");
		if (start == std::string::npos || line[start] == '#')
			continue;
		line = line.substr(start);

		std::cout << "TestScript> " << line << std::endl;

		// A "WAIT-TICKS <n>" line isn't a real console command - it runs
		// n logic ticks before the next line. Console commands only queue
		// an action (Object::AddAction()); it runs immediately only if
		// the game's INSTANT.IDS marks that action id as instant AND the
		// target's action list was empty - otherwise it just sits queued
		// until something ticks logic. Not done automatically after every
		// line: ticking logic also re-runs the current area's own AI
		// scripts (e.g. an in-progress opening cutscene), which can be
		// slow/long-running - so opt in explicitly with WAIT-TICKS right
		// after a command that needs it, rather than paying that cost on
		// every line.
		if (line.compare(0, 10, "WAIT-TICKS") == 0) {
			int ticks = ::atoi(line.c_str() + 10);
			for (int i = 0; i < ticks; i++)
				Core::Get()->UpdateLogic(true);
			continue;
		}

		console->ExecuteCommand(line);
	}
	std::cout << "Game: exec-file done, quitting" << std::endl;
}


// Copies every on-disk area checkpoint from one directory to another
// (creating `to` if needed) - shared by Game::Save() (the live session's
// single checkpoint directory -> this save's own archive) and Game::
// Load() (that archive -> the live session's directory), mirroring
// GemRB's own single-cache-directory approach (see AreaRoom::
// AreaCheckpointDir()'s own comment) with a plain directory copy instead
// of a real archive format.
static void
_CopyAreaCheckpoints(const std::string& from, const std::string& to)
{
	std::error_code error;
	if (!std::filesystem::exists(from, error))
		return;

	std::filesystem::create_directories(to, error);
	std::filesystem::copy(from, to,
		std::filesystem::copy_options::recursive
			| std::filesystem::copy_options::overwrite_existing,
		error);
}


bool
Game::Save(const char* name)
{
	if (fParty == NULL || fParty->CountActors() == 0) {
		std::cerr << "Game::Save(): no party to save" << std::endl;
		return false;
	}

	RoomBase* room = Core::Get()->CurrentRoom();
	res_ref areaName(room != NULL ? room->Name() : "");

	// Snapshot the area actually being stood in too - _UnloadArea() only
	// checkpoints an area once it's actually left, so without this a save
	// made without ever having left the current area since arriving would
	// come back pristine on a later load.
	if (AreaRoom* areaRoom = dynamic_cast<AreaRoom*>(room))
		areaRoom->WriteCheckpoint();

	// Every area checkpointed at any point this session - not just the
	// current one - lives in AreaRoom's single, session-long checkpoint
	// directory (see its own comment): copy all of it into this save's
	// own archive directory, so an area visited and left long before
	// this save, and never revisited since, is captured too.
	_CopyAreaCheckpoints(AreaRoom::AreaCheckpointDir(), std::string(name) + ".arecache");

	GamResource* gam = new GamResource(res_ref("SAVE"));
	gam->SetCurrentArea(areaName);

	for (uint16 i = 0; i < fParty->CountActors(); i++) {
		Actor* actor = fParty->ActorAt(i);

		gam->AddPartyMember(_GamMember(actor, areaName), actor->CRE());
	}

	for (Actor* npc : fNPCs)
		gam->AddOutOfPartyMember(_GamMember(npc, npc->AreaName()), npc->CRE());

	gam->SetVariables(Core::Get()->Vars().All());
	gam->SetGameTime(GameTimer::GameTime());
	gam->SetRealTime(GameTimer::RealTime());
	std::vector<gam_journal_entry> journal;
	for (const journal_entry& entry : fJournal)
		journal.push_back({ entry.strref, entry.time, entry.chapter, entry.section, entry.group });
	gam->SetJournalEntries(journal);
	// Every party member's own reputation byte is kept in sync by
	// REPUTATIONSET/REPUTATIONINC (scripting/Actions.cpp) - the leader's
	// is as good as any (see GamResource.h's own comment on why this is
	// write-only).
	gam->SetReputation(fParty->ActorAt(0)->CRE()->Reputation());

	bool result = gam->WriteToFile(name);
	// `gam` is a runtime-built Resource (key 0, never went through
	// GetResource()/the cache) - plain Release() would leak it, since
	// nothing else holds a reference to drop it to 0 and Resource's own
	// destructor is protected (only ResourceManager can call it).
	gResManager->ReleaseResource(gam);
	return result;
}


bool
Game::Load(const char* name)
{
	GamResource* gam = new GamResource(res_ref("SAVE"));
	if (!gam->LoadFromFile(name)) {
		gResManager->ReleaseResource(gam);
		return false;
	}

	// Abandon whatever's currently loaded - a load restores *that save's*
	// world, not whatever the live session still happens to be holding
	// onto (see _ClearAreaCache()'s own comment below). Its own on-unload
	// checkpoint write still happens (into the live session's single
	// checkpoint directory, see AreaRoom::AreaCheckpointDir()), but that's
	// harmless: the wipe-and-restore right below discards it along with
	// everything else already there, unconditionally.
	Core::Get()->UnloadCurrentRoom();

	std::error_code error;
	std::filesystem::remove_all(AreaRoom::AreaCheckpointDir(), error);
	_CopyAreaCheckpoints(std::string(name) + ".arecache", AreaRoom::AreaCheckpointDir());

	// The in-memory session cache (live Actor/ARAResource C++ objects
	// from areas visited earlier this session) is separate from the
	// on-disk checkpoints just restored above, and isn't reset by
	// replacing them - drop it too, or a revisited area would resurrect
	// this abandoned session's own state instead of reading back what
	// was just restored from disk.
	_ClearAreaCache();
	// Same for a store's stock: the loaded save's world starts from the
	// stores' original stock.
	fScreens->Find<StoreScreen>()->ClearStores();

	delete fParty;
	fParty = new ::Party();

	_ClearNPCs();

	uint32 count = gam->PartyMemberCount();
	std::vector<IE::point> savedPositions;
	for (uint32 i = 0; i < count; i++) {
		gam_party_member member = gam->PartyMemberAt(i);

		Actor* actor = _RestoreActor(member, gam->PartyMemberCRE(i));
		fParty->AddActor(actor);
		savedPositions.push_back(member.position);
	}

	_LoadNPCs(gam);

	for (const auto& variable : gam->Variables())
		Core::Get()->Vars().Set(variable.first.c_str(), variable.second);

	GameTimer::SetGameTime(gam->GameTime());
	fJournal.clear();
	for (const gam_journal_entry& entry : gam->JournalEntries())
		fJournal.push_back({ entry.strref, entry.section, entry.group, entry.chapter, entry.time });

	res_ref area = gam->CurrentArea();
	gResManager->ReleaseResource(gam);
	if (!Core::Get()->LoadArea(area, "", ""))
		return false;

	// LoadArea() above builds a fresh AreaRoom, which - having no real
	// entrance name to go on, a load isn't an actual area transition -
	// parks every party member at that area's own EntranceAt(0) instead
	// (see AreaRoom::AreaRoom()'s own per-member spawn loop). Put them
	// back where this save actually had them; each Actor() above already
	// got its saved position as a constructor argument, but that's long
	// since been overwritten by the entrance-repositioning above.
	for (uint16 i = 0; i < fParty->CountActors() && i < savedPositions.size(); i++)
		fParty->ActorAt(i)->SetPosition(savedPositions[i]);

	if (Actor* leader = fParty->ActorAt(0)) {
		if (RoomBase* room = Core::Get()->CurrentRoom())
			room->SetAreaOffsetCenter(leader->Position());
	}

	return true;
}


void
Game::InitiateDialog(Actor* actor, Actor* target)
{
	assert(fDialog == NULL);

	const res_ref dialogFile = actor->CRE()->DialogFile();
	if (dialogFile.name[0] == '\0'
			|| strcasecmp(dialogFile.CString(), "None") == 0) {
		std::cout << "EMPTY DIALOG FILE" << std::endl;
		return;
	}

	GUI::Get()->EnsureShowDialogArea();

	actor->AddTrigger(trigger_entry("LastTalkedToBy", target));
	std::cout << "initiates dialog with " << actor->LongName() << std::endl;
	std::cout << "Dialog file: " << dialogFile << std::endl;

	fDialog = new DialogHandler(actor, target, dialogFile);
	if (fDialog->Resource() == NULL) {
		std::cerr << "InitiateDialog: dialog file \"" << dialogFile
			<< "\" not found" << std::endl;
		delete fDialog;
		fDialog = NULL;
		return;
	}

	if (!fDialog->Continue())
		TerminateDialog();
}


bool
Game::InDialogMode() const
{
	return fDialog != NULL;
}


void
Game::TerminateDialog()
{
	if (InDialogMode()) {
		std::cout << fDialog->Actor()->Name() << " TerminateDialog()" << std::endl;
		fDialog->Actor()->IncrementNumTimesTalkedTo();
	}
	delete fDialog;
	fDialog = NULL;
	GUI::Get()->EnsureShowNormalMessageArea();
}


DialogHandler*
Game::Dialog()
{
	return fDialog;
}


void
Game::SetToken(const std::string& name, const std::string& value)
{
	fTokens[name] = value;
}


const std::map<std::string, std::string>&
Game::Tokens() const
{
	return fTokens;
}


bool
Game::AddJournalEntry(uint32 strref, uint8 section, uint8 group)
{
	const uint8 chapter = (uint8)Core::Get()->Vars().Get("CHAPTER");

	for (journal_entry& entry : fJournal) {
		if (entry.strref != strref)
			continue;
		// Already there: nothing to do in the same section.
		if (entry.section == section)
			return false;
		// Finishing a quest of a group replaces the group with this entry.
		if (section == JOURNAL_DONE && group != 0) {
			RemoveJournalGroup(group);
			break;
		}
		entry.section = section;
		entry.group = group;
		entry.chapter = chapter;
		entry.time = GameTimer::GameTime();
		return true;
	}

	journal_entry entry;
	entry.strref = strref;
	entry.section = section;
	entry.group = group;
	entry.chapter = chapter;
	entry.time = GameTimer::GameTime();
	fJournal.push_back(entry);
	return true;
}


void
Game::RemoveJournalEntry(uint32 strref)
{
	auto it = std::find_if(fJournal.begin(), fJournal.end(),
		[strref](const journal_entry& entry) { return entry.strref == strref; });
	if (it != fJournal.end())
		fJournal.erase(it);
}


void
Game::RemoveJournalGroup(uint8 group)
{
	fJournal.erase(std::remove_if(fJournal.begin(), fJournal.end(),
		[group](const journal_entry& entry) { return entry.group == group; }),
		fJournal.end());
}


const std::vector<journal_entry>&
Game::Journal() const
{
	return fJournal;
}


std::vector<uint32>
Game::JournalEntries() const
{
	std::vector<uint32> strrefs;
	for (const journal_entry& entry : fJournal)
		strrefs.push_back(entry.strref);
	return strrefs;
}


void
Game::SetJournal(const std::vector<journal_entry>& entries)
{
	fJournal = entries;
}


void
Game::SetAreaMapVisible(const std::string& areaName, bool visible)
{
	fAreaMapVisibility[areaName] = visible;
}


bool
Game::AreaMapVisibleOverride(const std::string& areaName, bool* visible) const
{
	auto it = fAreaMapVisibility.find(areaName);
	if (it == fAreaMapVisibility.end())
		return false;
	*visible = it->second;
	return true;
}


Party*
Game::Party()
{
	return fParty;
}


uint16
Game::CountNPCs() const
{
	return fNPCs.size();
}


Actor*
Game::NPCAt(uint16 index) const
{
	return index < fNPCs.size() ? fNPCs[index] : NULL;
}


bool
Game::IsNPC(const Actor* actor) const
{
	return std::find(fNPCs.begin(), fNPCs.end(), actor) != fNPCs.end();
}


Actor*
Game::FindNPC(const char* name) const
{
	for (Actor* npc : fNPCs) {
		if (strcasecmp(name, npc->Name()) == 0)
			return npc;
		const CREResource* cre = npc->CRE();
		if (cre != NULL && !cre->DeathVariable().empty()
				&& strcasecmp(name, cre->DeathVariable().c_str()) == 0) {
			return npc;
		}
	}
	return NULL;
}


void
Game::AddNPC(Actor* actor)
{
	if (actor != NULL)
		fNPCs.push_back(actor);
}


void
Game::RemoveNPC(Actor* actor)
{
	auto i = std::find(fNPCs.begin(), fNPCs.end(), actor);
	if (i != fNPCs.end()) {
		(*i)->Release();
		fNPCs.erase(i);
	}
}


void
Game::_LoadStartingNPCs()
{
	// A new game starts from BALDUR.GAM: its out-of-party NPC table says
	// where every companion and story character begins.
	GamResource* gam = gResManager->GetGAM(res_ref("BALDUR"));
	if (gam == NULL)
		return;
	_LoadNPCs(gam);
	gResManager->ReleaseResource(gam);
}


void
Game::MakeNPC(Actor* actor)
{
	if (fParty->HasActor(actor) || IsNPC(actor))
		return;

	if (AreaRoom* room = actor->Area())
		room->ForgetPlacedActor(actor);
	actor->Acquire();
	AddNPC(actor);
}


void
Game::MoveNPC(Actor* npc, const res_ref& area, const IE::point& position,
	int orientation)
{
	AreaRoom* room = npc->Area();
	if (room != NULL && strcasecmp(room->Name(), area.CString()) != 0) {
		room->RemoveObject(npc);
		npc->SetArea(NULL);
		// The room's own reference: the list's keeps the NPC alive.
		npc->Release();
	}

	npc->SetAreaName(area);
	npc->SetPosition(position);
	if (orientation >= 0)
		npc->SetOrientation(orientation);

	// Moved into the room that is loaded right now, from an area that isn't.
	if (npc->Area() == NULL) {
		AreaRoom* current = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
		if (current != NULL && strcasecmp(current->Name(), area.CString()) == 0) {
			npc->Acquire();
			current->AddObject(npc);
			npc->SetPosition(position);
		}
	}
}


void
Game::JoinParty(Actor* actor)
{
	if (fParty->HasActor(actor))
		return;

	// The party's reference is a new one; a global NPC's own goes with
	// the list entry.
	actor->Acquire();
	fParty->AddActor(actor);
	if (IsNPC(actor)) {
		RemoveNPC(actor);
	} else if (AreaRoom* room = actor->Area()) {
		room->ForgetPlacedActor(actor);
	}
	RefreshHUDPortraits();
}


void
Game::LeaveParty(Actor* actor)
{
	if (!fParty->HasActor(actor))
		return;

	actor->Acquire();
	fParty->RemoveActor(actor);
	AddNPC(actor);
	if (fShownCharacter >= fParty->CountActors())
		fShownCharacter = 0;
	RefreshHUDPortraits();
}


void
Game::_ClearNPCs()
{
	for (Actor* actor : fNPCs)
		actor->Release();
	fNPCs.clear();
}


void
Game::_LoadNPCs(GamResource* gam)
{
	for (uint32 i = 0; i < gam->OutOfPartyCount(); i++) {
		gam_party_member member = gam->OutOfPartyAt(i);

		// Someone already in the party (a -P override or the default
		// party names the same creatures) can't also be standing
		// somewhere else.
		bool inParty = false;
		for (uint16 p = 0; p < fParty->CountActors(); p++) {
			if (strcasecmp(fParty->ActorAt(p)->Name(), member.creName.CString()) == 0)
				inParty = true;
		}
		if (inParty)
			continue;

		Actor* npc = _RestoreActor(member, gam->OutOfPartyCRE(i));
		npc->SetAreaName(member.areaName);
		AddNPC(npc);
	}
}


Actor*
Game::_RestoreActor(const gam_party_member& member, CREResource* savedCre)
{
	// Actor()'s normal constructor fetches the character's original,
	// unmodified CRE from the game's own files (ResourceManager) - this
	// reuses all of Actor's existing init logic (animation factory,
	// etc.) safely. The saved CRE state (inventory, spellbook, HP,
	// status, ...) is then applied on top of it.
	Actor* actor = new Actor(member.creName.CString(), member.position,
		member.orientation);

	if (savedCre != NULL) {
		actor->CRE()->CopyDataFrom(savedCre);
		actor->RefreshColors();
		gResManager->ReleaseResource(savedCre);
	}

	for (uint32 q = 0; q < Actor::kNumQuickSpells; q++)
		actor->SetQuickSpell(q, member.quickSpells[q]);

	return actor;
}


gam_party_member
Game::_GamMember(Actor* actor, const res_ref& areaName) const
{
	gam_party_member member;
	member.creName = res_ref(actor->Name());
	member.name = actor->Name();
	member.position = actor->Position();
	member.orientation = (uint16)actor->Orientation();
	member.areaName = areaName;
	for (uint32 q = 0; q < Actor::kNumQuickSpells; q++)
		member.quickSpells[q] = actor->QuickSpell(q);
	return member;
}


void
Game::LoadStartingArea()
{
	std::cout << "Load Starting Area...";
	TWODAResource* resource = gResManager->Get2DA("STARTARE");
	if (resource == NULL) {
		std::cout << "Failed!" << std::endl;
		return;
	}

	std::cout << "OK!" << std::endl;
	std::string startingArea = resource->ValueFor("START_AREA", "VALUE");
	IE::point viewPosition;
	viewPosition.x = resource->IntegerValueFor("START_XPOS", "VALUE");
	viewPosition.y = resource->IntegerValueFor("START_YPOS", "VALUE");

	std::cout << "Starting area: " << startingArea << std::endl;
	std::cout << "Starting position: " << viewPosition.x << "," << viewPosition.y << std::endl;

	TWODAResource* startPosResource = gResManager->Get2DA("STARTPOS");
	if (startPosResource == NULL) {
		std::cout << "Failed!" << std::endl;
		gResManager->ReleaseResource(resource);
		return;
	}

	Core::Get()->LoadArea(startingArea.c_str(), "foo", "");
	Core::Get()->CurrentRoom()->SetAreaOffsetCenter(viewPosition);

	if (fParty != NULL) {
		for (int16 i = 0; i < fParty->CountActors(); i++) {
			char column[16];
			snprintf(column, sizeof(column), "%d", i + 1);
			IE::point startPos;
			startPos.x = startPosResource->IntegerValueFor("START_XPOS", column);
			startPos.y = startPosResource->IntegerValueFor("START_YPOS", column);
			fParty->ActorAt(i)->SetPosition(startPos);
			fParty->ActorAt(i)->ClearDestination();
		}
	}

	gResManager->ReleaseResource(resource);
	gResManager->ReleaseResource(startPosResource);
}


void
Game::ToggleDayNight()
{
	GameTimer::AdvanceTime(12, 0, 0);

	RoomBase* area = Core::Get()->CurrentRoom();
	if (area != NULL)
		area->ReloadArea();
	// TODO: Update Area
}


void
Game::ToggleHUD()
{
	GUI::Get()->ToggleHUD();
}


void
Game::SelectPartyMember(uint16 index)
{
	if (fParty == nullptr || index >= fParty->CountActors())
		return;

	Actor* member = fParty->ActorAt(index);
	if (member == nullptr)
		return;

	fShownCharacter = index;
	fBar->ResetPage();

	AreaRoom* room = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
	if (room != nullptr)
		room->SelectActor(member);

	_RefreshCharacterScreens();
}


void
Game::ToggleSelectedPartyMember(uint16 index)
{
	if (fParty == nullptr || index >= fParty->CountActors())
		return;

	Actor* member = fParty->ActorAt(index);
	if (member == nullptr)
		return;

	AreaRoom* room = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
	if (room != nullptr)
		room->ToggleSelected(member);
}


void
Game::CenterViewOnPartyMember(uint16 index)
{
	if (fParty == nullptr || index >= fParty->CountActors())
		return;

	Actor* member = fParty->ActorAt(index);
	if (member == nullptr)
		return;

	const IE::point position = member->Position();
	AreaRoom* room = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
	if (room != nullptr)
		room->SetAreaOffsetCenter(position);
}


Game::TempState*
Game::GetTempState()
{
	return fTempState;
}


void
Game::SetTestMode(bool value)
{
	fTestMode = value;
}


bool
Game::TestMode() const
{
	return fTestMode;
}


