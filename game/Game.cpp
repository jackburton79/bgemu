/*
 * Game.cpp
 *
 *  Created on: 29/mar/2015
 *      Author: stefano
 */

#include "Game.h"

#include "2DAResource.h"
#include "Actor.h"
#include "AreaResource.h"
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
#include "GameTimer.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "ITMResource.h"
#include "Label.h"
#include "PLTResource.h"
#include "TextArea.h"
#include "Parsing.h"
#include "Party.h"
#include "ResManager.h"
#include "RoomBase.h"
#include "Window.h"


#include <algorithm>
#include <assert.h>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <utility>

#include "MemoryStream.h"


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
	fInvDragSlot(-1),
	fShownCharacter(0)
{
	fTempState = new Game::TempState;
	fAreaCache = new Game::AreaCache;
	fCharBuilder = new CharacterBuilder;
}


Game::~Game()
{
	TerminateDialog();
	delete fParty;
	delete fTempState;

	// Release what's still held onto (areas never revisited before the
	// process exits) - same convention as everything else this session
	// is careful to balance, even though it only matters for the ASan
	// leak report at this specific point (the whole process is about to
	// go away regardless).
	for (auto& entry : fAreaCache->areas) {
		for (Actor* actor : entry.second.actors)
			actor->Release();
		gResManager->ReleaseResource(entry.second.area);
	}
	delete fAreaCache;
	delete fCharBuilder;
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
								ToggleInventoryWindow();
								break;
							case SDLK_r:
								ToggleRecordWindow();
								break;
							case SDLK_j:
								ToggleJournalWindow();
								break;
							case SDLK_k:
								ToggleSpellbookWindow();
								break;
							case SDLK_F5:
								ToggleSaveWindow();
								break;
							case SDLK_F9:
								ToggleLoadWindow();
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
		fParty->AddActor(new Actor("Imoen", point, 0));
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
	if (Core::Get()->Game() == game::GAME_BALDURSGATE2)
		fParty->AddActor(new Actor("Imoen", position, 0));

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


// The full-screen panels (Inventory/Record/Journal/Spellbook/Save/Load)
// are mutually exclusive in real BG2 - opening one replaces whichever
// other one is open, it doesn't layer on top of it. Registry of each
// one's aux-window group, used by _CloseOtherScreens() below.
static const struct { const char* chu; uint16 windows[3]; uint8 count; }
kScreenGroups[] = {
	{ "GUIINV",  { 2, 0, 1 }, 3 },
	{ "GUIREC",  { 2, 0, 1 }, 3 },
	{ "GUIJRNL", { 2, 0, 1 }, 3 },
	{ "GUIMG",   { 2, 0, 1 }, 3 },
	{ "GUIPR",   { 2, 0, 1 }, 3 },
	{ "GUISAVE", { 0 },       1 },
	{ "GUILOAD", { 0 },       1 },
};


// Hides every registered screen group except `exceptCHU` - called before
// opening one, so opening a new screen always replaces any other one
// left open instead of stacking on top of it (harmless/no-op if they're
// already hidden).
static void
_CloseOtherScreens(const char* exceptCHU)
{
	for (const auto& group : kScreenGroups) {
		if (::strcasecmp(group.chu, exceptCHU) == 0)
			continue;
		for (uint8 i = 0; i < group.count; i++)
			GUI::Get()->HideAuxWindow(group.chu, group.windows[i]);
	}
}


// The left-hand command icon strip (ids 0-8, image GUILSOP) + Rest
// button (id 9, GUIRSBUT) appear identically - same position, same
// cycle - in every full-screen panel's own window 0 (GUIINV/GUIREC/
// GUIJRNL/GUIMG/GUIPR all confirmed via a real CHU dump to share this
// exact layout), duplicating GUIW's own WINDOW_COMMANDS bar so the
// player can still switch screens/rest while one of them is open.
// Reuses the confirmed GUIW mappings (Map/Inventory/Record/Spellbook/
// Save - see kHUDCommandButtons in gui/GUI.cpp); Rest gets its own
// entry here since its local id differs from GUIW's (9 vs. 11).
static const struct { uint32 controlID; void (*action)(); }
kAuxCommandBarButtons[] = {
	{ 1, [] { Core::Get()->LoadWorldMap(); } },
	{ 3, [] { Game::Get()->ToggleInventoryWindow(); } },
	{ 4, [] { Game::Get()->ToggleRecordWindow(); } },
	{ 5, [] { Game::Get()->ToggleSpellbookWindow(); } },
	{ 7, [] { Game::Get()->ToggleSaveWindow(); } },
	{ 9, [] { Game::Get()->TriggerRest(); } },
};


static void
_AuxCommandBarInvoked(uint32 controlID)
{
	for (const auto& button : kAuxCommandBarButtons) {
		if (button.controlID == controlID) {
			button.action();
			return;
		}
	}
}


// Shows/hides the Inventory window (GUIINV). Window 2 is the actual
// inventory panel (paperdoll + item slots); 0/1 are the persistent
// left/right side columns (portraits, quick items) that flank it -
// confirmed against real GUIINV.CHU data (64+512+64 = 640, the reference
// width). Public (rather than folded into the SDLK_i handler) so it can
// also be driven from the HUD inventory button and from test tooling.
void
Game::ToggleInventoryWindow()
{
	// A half-finished drag doesn't survive the window closing (or a fresh
	// open) - drop whatever's on the cursor back where it came from.
	fInvDragSlot = -1;
	GUI::Get()->SetDragBitmap(NULL);

	_CloseOtherScreens("GUIINV");
	if (GUI::Get()->ToggleAuxWindowGroup("GUIINV", {2, 0, 1})) {
		_UpdatePortraitColumn(GUI::Get()->GetAuxWindow("GUIINV", 1), 4);
		_UpdateInventoryIcons();
	}
}


// Shows/hides the "General" tab of the character Record window (GUIREC),
// same 3-window pattern as GUIINV (window 2 is the actual content panel;
// 0/1 are the same persistent side columns GUIINV also uses - GUIWLSP/
// GUIWRSP backgrounds confirmed identical via a real GUIREC.CHU dump).
void
Game::ToggleRecordWindow()
{
	_CloseOtherScreens("GUIREC");
	if (GUI::Get()->ToggleAuxWindowGroup("GUIREC", {2, 0, 1})) {
		_UpdatePortraitColumn(GUI::Get()->GetAuxWindow("GUIREC", 1), 4);
		_UpdateRecordLabels();
	}
}


// Shows/hides the (single-slot) Save/Load screen - just window 0, no
// side columns (it's a standalone full-screen 640x480 CHU, unlike
// GUIINV/GUIREC's 512-wide content panel flanked by the persistent
// portrait columns).
void
Game::ToggleSaveWindow()
{
	_CloseOtherScreens("GUISAVE");
	if (GUI::Get()->ToggleAuxWindowGroup("GUISAVE", {0}))
		_UpdateSaveLoadLabels("GUISAVE");
}


void
Game::ToggleLoadWindow()
{
	_CloseOtherScreens("GUILOAD");
	if (GUI::Get()->ToggleAuxWindowGroup("GUILOAD", {0}))
		_UpdateSaveLoadLabels("GUILOAD");
}


// Shows/hides the Journal window (GUIJRNL), same 3-window pattern as
// GUIINV/GUIREC (window 2 is the actual content panel; 0/1 the same
// persistent side columns).
void
Game::ToggleJournalWindow()
{
	_CloseOtherScreens("GUIJRNL");
	if (GUI::Get()->ToggleAuxWindowGroup("GUIJRNL", {2, 0, 1})) {
		_UpdatePortraitColumn(GUI::Get()->GetAuxWindow("GUIJRNL", 1), 4);
		_UpdateJournalLabels();
	}
}


void
Game::JournalControlInvoked(uint32 controlID, uint16 windowID)
{
	if (windowID == 0) {
		_AuxCommandBarInvoked(controlID);
		return;
	}
	// Window 1 is the portrait column (same layout as GUIINV/GUIREC's).
	if (windowID == 1 && controlID <= 3)
		ShowCharacter((uint16)controlID);
}


static Bitmap* _MakeSpellIcon(const res_ref& spellName); // defined below

// GUIMG/GUIPR window-2 control ids (identical layout, from the CHU dump):
// left page = the memorized-spell grid (3-col, ids 3-14), right page =
// the known-spell grid (4-col, ids 27-38).
static const uint32 kSpellMemoControls[] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };
static const uint32 kSpellKnownControls[] = { 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38 };
static const uint32 kSpellNameLabelID = 268435455;


// Whether the character casts divine spells (uses GUIPR) rather than
// arcane (GUIMG) - true for a pure priest/druid/paladin/ranger, or when
// they've a priest-type known spell.
static bool
_UsesDivineSpellbook(Actor* actor)
{
	if (actor == NULL || actor->CRE() == NULL)
		return false;
	for (const cre_known_spell& s : actor->CRE()->KnownSpells()) {
		if (s.type == 0)
			return true;
		if (s.type == 1)
			return false;
	}
	std::string className = IDTable::ClassAt(actor->CRE()->Class());
	for (char& c : className) c = (char)toupper((unsigned char)c);
	static const char* kArcane[] = { "MAGE", "SORCERER", "BARD" };
	for (const char* name : kArcane)
		if (className.find(name) != std::string::npos)
			return false;
	static const char* kDivine[] = { "CLERIC", "DRUID", "PALADIN", "RANGER" };
	for (const char* name : kDivine)
		if (className.find(name) != std::string::npos)
			return true;
	return false;
}


void
Game::ToggleSpellbookWindow()
{
	const char* chu = _UsesDivineSpellbook(_ShownActor()) ? "GUIPR" : "GUIMG";
	fSpellbookCHU = chu;
	fSpellbookLevel = 1;
	_CloseOtherScreens(chu);
	if (GUI::Get()->ToggleAuxWindowGroup(chu, {2, 0, 1})) {
		_UpdatePortraitColumn(GUI::Get()->GetAuxWindow(chu, 1), 4);
		_UpdateSpellbookScreen();
	}
}


// Fills the known/memorized grids with the shown character's arcane
// spells (level 1 only - no page navigation yet), remembering which
// spell each grid button shows so SpellbookControlInvoked() can act on
// a click.
void
Game::_UpdateSpellbookScreen()
{
	fSpellbookKnown.clear();
	fSpellbookMemo.clear();

	if (fSpellbookCHU.empty())
		return;
	const uint16 spellType = (fSpellbookCHU == "GUIPR") ? 0 : 1; // 0 priest, 1 wizard
	Window* window = GUI::Get()->GetAuxWindow(fSpellbookCHU.c_str(), 2);
	Actor* actor = _ShownActor();
	if (window == NULL || actor == NULL || actor->CRE() == NULL)
		return;
	CREResource* cre = actor->CRE();

	const uint16 level = fSpellbookLevel;

	Label* nameLabel = dynamic_cast<Label*>(window->GetControlByID(kSpellNameLabelID));
	if (nameLabel != NULL)
		nameLabel->SetText(actor->LongName() + " - level " + std::to_string(level));

	// Known spells of the current level/type.
	std::vector<cre_known_spell> known = cre->KnownSpells();
	size_t k = 0;
	for (uint32 controlID : kSpellKnownControls) {
		Button* button = dynamic_cast<Button*>(window->GetControlByID(controlID));
		if (button == NULL)
			continue;
		while (k < known.size() && (known[k].type != spellType || known[k].level != level))
			k++;
		if (k < known.size()) {
			button->SetIcon(_MakeSpellIcon(known[k].spell), true);
			fSpellbookKnown[controlID] = known[k].spell;
			k++;
		} else {
			button->SetIcon(NULL);
		}
	}

	// Memorized spells of the current level/type - the memo-info rows
	// point at each level/type's slice of the memorized table.
	std::vector<cre_memorized_spell> memo = cre->MemorizedSpells();
	std::vector<cre_spell_memorization_info> info = cre->SpellMemorizationInfo();
	std::vector<cre_memorized_spell> levelMemo;
	for (const cre_spell_memorization_info& row : info) {
		if (row.level != level || row.type != spellType)
			continue;
		for (uint32 i = 0; i < row.memorizedCount; i++) {
			uint32 idx = row.firstMemorizedIndex + i;
			if (idx < memo.size())
				levelMemo.push_back(memo[idx]);
		}
	}
	size_t m = 0;
	for (uint32 controlID : kSpellMemoControls) {
		Button* button = dynamic_cast<Button*>(window->GetControlByID(controlID));
		if (button == NULL)
			continue;
		if (m < levelMemo.size()) {
			button->SetIcon(_MakeSpellIcon(levelMemo[m].spell), true);
			if (levelMemo[m].flags & 1)
				fSpellbookMemo[controlID] = levelMemo[m].spell;
			m++;
		} else {
			button->SetIcon(NULL);
		}
	}
}


void
Game::SpellbookControlInvoked(uint32 controlID, uint16 windowID)
{
	if (windowID == 0)
		return _AuxCommandBarInvoked(controlID);
	if (windowID == 1 && controlID <= 3) {
		ShowCharacter((uint16)controlID);
		return;
	}
	if (windowID == 3) {
		// The spell-info popup: any button closes it.
		if (!fSpellbookCHU.empty())
			GUI::Get()->HideAuxWindow(fSpellbookCHU.c_str(), 3);
		return;
	}
	if (windowID != 2)
		return;

	// Page arrows: control 1 = previous spell level, control 2 = next.
	if (controlID == 1 || controlID == 2) {
		if (controlID == 1 && fSpellbookLevel > 1)
			fSpellbookLevel--;
		else if (controlID == 2 && fSpellbookLevel < 9)
			fSpellbookLevel++;
		_UpdateSpellbookScreen();
		return;
	}

	Actor* actor = _ShownActor();
	if (actor == NULL || actor->CRE() == NULL)
		return;

	// Click a known spell -> memorize it into a free slot; click a
	// memorized spell -> release it (un-memorize).
	auto known = fSpellbookKnown.find(controlID);
	if (known != fSpellbookKnown.end()) {
		actor->CRE()->MemorizeSpell(known->second);
		_UpdateSpellbookScreen();
		return;
	}
	auto memo = fSpellbookMemo.find(controlID);
	if (memo != fSpellbookMemo.end()) {
		actor->CRE()->ConsumeMemorizedSpell(memo->second);
		_UpdateSpellbookScreen();
	}
}


void
Game::SpellbookControlHovered(uint32 controlID, bool inside)
{
	if (!inside) {
		GUI::Get()->SetHoverTooltip("");
		return;
	}
	res_ref spell;
	auto known = fSpellbookKnown.find(controlID);
	auto memo = fSpellbookMemo.find(controlID);
	if (known != fSpellbookKnown.end())
		spell = known->second;
	else if (memo != fSpellbookMemo.end())
		spell = memo->second;
	else {
		GUI::Get()->SetHoverTooltip("");
		return;
	}

	std::string name = spell.CString();
	SPLResource* spl = gResManager->GetSPL(spell);
	if (spl != NULL) {
		std::string dialog = IDTable::GetDialog(spl->DisplayNameRef());
		if (!dialog.empty())
			name = dialog;
		gResManager->ReleaseResource(spl);
	}
	GUI::Get()->SetHoverTooltip(name);
}


void
Game::SpellbookControlRightClicked(uint32 controlID, uint16 windowID)
{
	if (windowID != 2)
		return;
	auto known = fSpellbookKnown.find(controlID);
	if (known != fSpellbookKnown.end()) {
		_ShowSpellInfo(known->second);
		return;
	}
	auto memo = fSpellbookMemo.find(controlID);
	if (memo != fSpellbookMemo.end())
		_ShowSpellInfo(memo->second);
}


// Populates and shows the spellbook's examine popup (GUIMG/GUIPR window
// 3) for a spell - name + description, same pattern as _ShowItemInfo().
void
Game::_ShowSpellInfo(const res_ref& spellName)
{
	if (fSpellbookCHU.empty())
		return;

	SPLResource* spl = gResManager->GetSPL(spellName);
	if (spl == NULL)
		return;

	GUI::Get()->ShowAuxWindow(fSpellbookCHU.c_str(), 3);
	Window* window = GUI::Get()->GetAuxWindow(fSpellbookCHU.c_str(), 3);
	if (window == NULL) {
		gResManager->ReleaseResource(spl);
		return;
	}

	Label* title = dynamic_cast<Label*>(window->GetControlByID(268435455));
	if (title != NULL) {
		std::string name = IDTable::GetDialog(spl->DisplayNameRef());
		title->SetText(name.empty() ? spellName.CString() : name);
	}

	TextArea* description = dynamic_cast<TextArea*>(window->GetControlByID(3));
	if (description != NULL) {
		description->ClearText();
		std::string text = IDTable::GetDialog(spl->DisplayDescriptionRef());
		description->AddText(text.empty() ? "(No description)" : text.c_str());
		description->ScrollTo(0, 0);
	}

	gResManager->ReleaseResource(spl);
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


// GUIINV.CHU/GUIREC.CHU/GUISAVE.CHU/GUILOAD.CHU control IDs used by the
// label-population methods below, named rather than left as bare
// literals at each call site - all identified/confirmed as described in
// those methods' own comments.
static const uint32 kInvNameLabelID = 268435507;
static const uint32 kInvACLabelID = 268435512;
// Confirmed against GemRB's own GUIINV.py (bg1/bg2, identical control
// IDs in both): current/max hit points and the party gold counter -
// none of the three are CHU-authored static text, all three are set
// from code every time the window refreshes, same as name/AC above.
static const uint32 kInvHPCurrentLabelID = 268435513;
static const uint32 kInvHPMaxLabelID = 268435514;
static const uint32 kInvGoldLabelID = 268435520;
// The paperdoll control itself (128x160, CHU-authored to a fixed
// placeholder bitmap - CIFF4INV, a generic doll unrelated to whichever
// character's inventory is actually open) - see _UpdatePaperdoll().
static const uint32 kInvPaperdollID = 50;
// GUIINV window 5 (background GUIINVHI) is BG2's "examine item" popup,
// opened by right-clicking a slot (confirmed via a real GUIINV.CHU dump):
// id 268435455 = item title, id 7 = the large item-icon button, id 5 =
// the scrollable description text_area. Any button in the window closes
// it (only "Done", id 4, is meaningful here).
static const uint16 kInvInfoWindowID = 5;
static const uint32 kInvInfoTitleID = 268435455;
static const uint32 kInvInfoIconID = 7;
static const uint32 kInvInfoTextID = 5;
// Two CHU-authored labels in that window that otherwise show a literal
// "(No text)" TLK placeholder - blanked rather than left visible (real
// BG2 fills them from code; their exact purpose isn't confirmed here).
static const uint32 kInvInfoBlankLabel1ID = 268435456;
static const uint32 kInvInfoBlankLabel2ID = 268435467;
// GUIREC window 2's top banner - the character name, same role as
// GUIINV's kInvNameLabelID (confirmed via a real dump: id 268435495 at
// (3,9) 505x28, the wide centered label at the very top).
static const uint32 kRecNameLabelID = 268435495;
static const uint32 kRecACLabelID = 268435496;
static const uint32 kRecClassLabelID = 268435471;
static const uint32 kRecRaceLabelID = 268435472;
static const uint32 kRecLevelLabelID = 268435473;
static const uint32 kRecStatsAreaID = 45;
// GUISAVE.CHU and GUILOAD.CHU share the exact same window 0 layout
// (confirmed via a real dump of both - same control ids/positions) -
// only the first of the 4 visible slots is used in this minimal
// version, plus the main confirm button.
static const uint32 kSaveSlot1NameLabelID = 268435464;
static const uint32 kSaveSlot1StatusLabelID = 268435472;
static const uint32 kSaveConfirmButtonID = 34;
// Real BG2 numbers save files per slot (see SAVEGAME(190)'s own
// existing "savegame_slot<N>.gam" convention in RunActionSaveGame(),
// scripting/Actions.cpp) - this minimal single-slot screen just always
// uses slot 0.
static const char* kSavePath = "SAVEGAME";
static const char* kSaveSlotPath = "savegame_slot0.gam";
// GUIJRNL.CHU window 2 (confirmed via a real dump): id 1 is the main
// scrollable entries text_area (with its own scrollbar at id 2, same
// pairing convention as GUIREC's saves/resistances area).
static const uint32 kJournalEntriesAreaID = 1;


// GUIINV window 2 slot-control id -> CRE item-slot index. Both the icon
// refresh (_UpdateInventoryIcons) and the drag/drop click handler
// (InventoryControlInvoked) walk this one table. How each group was
// identified:
//  - general grid (ids 30/32/.../44 then 31/33/.../45): row-major reading
//    order, confirmed against a real GUIINV.CHU dump to be
//    kSlotGeneralFirst..Last (16 slots) in order.
//  - row above the paperdoll (ids 11-14): Armor/Gauntlets/Helmet/Shield,
//    icon-verified on a real character wearing all four.
//  - "Armi rapide" (ids 1-4): Weapon1-4, by count + the same ascending
//    id->ascending slot pattern as the verified row above.
//  - "Faretra" (ids 15-17): the first 3 of the CRE's 4 ammo slots - this
//    CHU layout only has 3 controls (declared deviation).
//  - "Oggetti rapidi" (ids 5-7): QuickItem1-3 (slots 18-20), per the
//    item-order comment in Actor.cpp.
// Still unmapped (deliberately, no empirical confirmation yet):
// rings/amulet/belt/boots/cloak (ids ~21-26).
struct inv_slot_control { uint32 controlID; uint32 creSlot; };
static const inv_slot_control kInvSlotControls[] = {
	{ 30, kSlotGeneralFirst +  0 }, { 32, kSlotGeneralFirst +  1 },
	{ 34, kSlotGeneralFirst +  2 }, { 36, kSlotGeneralFirst +  3 },
	{ 38, kSlotGeneralFirst +  4 }, { 40, kSlotGeneralFirst +  5 },
	{ 42, kSlotGeneralFirst +  6 }, { 44, kSlotGeneralFirst +  7 },
	{ 31, kSlotGeneralFirst +  8 }, { 33, kSlotGeneralFirst +  9 },
	{ 35, kSlotGeneralFirst + 10 }, { 37, kSlotGeneralFirst + 11 },
	{ 39, kSlotGeneralFirst + 12 }, { 41, kSlotGeneralFirst + 13 },
	{ 43, kSlotGeneralFirst + 14 }, { 45, kSlotGeneralFirst + 15 },
	{ 11, kSlotArmor }, { 12, kSlotGauntlets }, { 13, kSlotHelmet }, { 14, kSlotShield },
	{ 1, kSlotWeaponFirst }, { 2, kSlotWeaponFirst + 1 },
	{ 3, kSlotWeaponFirst + 2 }, { 4, kSlotWeaponFirst + 3 },
	{ 15, kSlotAmmoFirst }, { 16, kSlotAmmoFirst + 1 }, { 17, kSlotAmmoFirst + 2 },
	{ 5, 18 }, { 6, 19 }, { 7, 20 },
	// Rings / amulet / belt / boots / cloak (GUIINV window-2 ids 21-26,
	// mapped by on-screen position - to be confirmed empirically).
	{ 22, kSlotRingLeft }, { 23, kSlotRingLeft + 1 }, { 25, kSlotAmulet },
	{ 21, kSlotBelt }, { 24, kSlotBoots }, { 26, kSlotCloak },
};


// Builds the inventory icon (cycle 0 / frame 0 of the ITM's inventory-icon
// BAM, same convention Button uses for its own CHU bitmaps) for an item
// resref. Returns a new reference the caller owns, or NULL.
static Bitmap*
_MakeItemIcon(const res_ref& itemName)
{
	ITMResource* itm = gResManager->GetITM(itemName);
	if (itm == NULL)
		return NULL;
	Bitmap* icon = NULL;
	BAMResource* bam = gResManager->GetBAM(itm->InventoryIcon());
	if (bam != NULL) {
		icon = bam->FrameForCycle(0, 0);
		gResManager->ReleaseResource(bam);
	}
	gResManager->ReleaseResource(itm);
	return icon;
}


// The spellbook-icon frame for a spell resref (SPL 0x3a -> BAM cycle 0
// frame 0). Caller owns the returned reference, or NULL.
static Bitmap*
_MakeSpellIcon(const res_ref& spellName)
{
	SPLResource* spl = gResManager->GetSPL(spellName);
	if (spl == NULL)
		return NULL;
	Bitmap* icon = NULL;
	BAMResource* bam = gResManager->GetBAM(spl->BookIcon());
	if (bam != NULL) {
		icon = bam->FrameForCycle(0, 0);
		gResManager->ReleaseResource(bam);
	}
	gResManager->ReleaseResource(spl);
	return icon;
}


// Human-readable name for an item: its identified name, else its
// unidentified name, else the bare resref.
static std::string
_ItemDisplayName(ITMResource* itm, const res_ref& itemName)
{
	if (itm != NULL) {
		std::string name = IDTable::GetDialog(itm->IdentifiedNameRef());
		if (name.empty())
			name = IDTable::GetDialog(itm->UnidentifiedNameRef());
		if (!name.empty())
			return name;
	}
	return itemName.CString();
}


// Same as above, resolving the ITM resource itself from the resref -
// for call sites (inventory drag/drop logging) that only have the
// resref, not an already-loaded ITMResource*.
static std::string
_ItemDisplayName(const res_ref& itemName)
{
	ITMResource* itm = gResManager->GetITM(itemName);
	std::string name = _ItemDisplayName(itm, itemName);
	if (itm != NULL)
		gResManager->ReleaseResource(itm);
	return name;
}


// CRE item-slot for a GUIINV control id, or -1 if the control isn't a
// mapped inventory slot.
static int32
_CreSlotForControl(uint32 controlID)
{
	for (const auto& entry : kInvSlotControls) {
		if (entry.controlID == controlID)
			return (int32)entry.creSlot;
	}
	return -1;
}


// GUIINV window 2's 5 "ground item" slot buttons (ids 68-72, confirmed
// identical in both games' real CHU data) - the actual click target for
// dropping a held item to the floor: real GemRB GUIScripts
// (InventoryCommon.OnDragItemGround()) call DropDraggedItem(pc, -2) on
// exactly these while an item is being dragged. Only that drop side is
// wired here; the same buttons are also meant to show and pick back up
// whatever's already on the ground at the character's feet (paged by
// the neighboring scrollbar, control id 66) - not implemented yet, so
// they stay visually empty. Ground items are still only picked up by
// clicking their pile in the game world (AreaRoom::PickUpGroundPile()).
static bool
_IsGroundItemSlotControl(uint32 controlID)
{
	return controlID >= 68 && controlID <= 72;
}


Actor*
Game::_ShownActor() const
{
	if (fParty == NULL || fParty->CountActors() == 0)
		return NULL;
	uint16 index = fShownCharacter < fParty->CountActors() ? fShownCharacter : 0;
	return fParty->ActorAt(index);
}


void
Game::ShowCharacter(uint16 partyIndex)
{
	// The portrait column drives the same "which member is current"
	// state the world selection (number keys 1-6) uses.
	SelectPartyMember(partyIndex);
}


// Re-populates whichever of the Inventory / Record screens are open for
// the current fShownCharacter - called after any change of who's shown.
void
Game::_RefreshCharacterScreens()
{
	RefreshHUDPortraits();
	if (GUI::Get()->GetAuxWindow("GUIINV", 2) != NULL) {
		_UpdatePortraitColumn(GUI::Get()->GetAuxWindow("GUIINV", 1), 4);
		_UpdateInventoryIcons();
	}
	if (GUI::Get()->GetAuxWindow("GUIREC", 2) != NULL) {
		_UpdatePortraitColumn(GUI::Get()->GetAuxWindow("GUIREC", 1), 4);
		_UpdateRecordLabels();
	}
	if (!fSpellbookCHU.empty()
		&& GUI::Get()->GetAuxWindow(fSpellbookCHU.c_str(), 2) != NULL) {
		_UpdatePortraitColumn(GUI::Get()->GetAuxWindow(fSpellbookCHU.c_str(), 1), 4);
		_UpdateSpellbookScreen();
	}
}


void
Game::RecordControlInvoked(uint32 controlID, uint16 windowID)
{
	if (windowID == 0) {
		_AuxCommandBarInvoked(controlID);
		return;
	}
	// Window 1 is the portrait column (same layout as GUIINV's).
	if (windowID == 1 && controlID <= 3)
		ShowCharacter((uint16)controlID);
}


// Fills a portrait column's buttons (ids 0..count-1) with the party's
// small portraits, clearing the rest. Same column layout on the HUD
// (GUIW window 1, 6 slots) and the Inventory/Record side panel (GUIINV/
// GUIREC window 1, 4 slots).
void
Game::_UpdatePortraitColumn(Window* window, uint32 count)
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
	_UpdatePortraitColumn(GUI::Get()->GetWindow(GUI::WINDOW_PLAYER_SLOTS), 6);
}


// Populates the inventory-slot buttons in the open GUIINV window 2 with
// the real item icon (ITM's InventoryIcon(), cycle 0/frame 0 of that BAM -
// same convention Button's own constructor uses for its CHU-authored
// bitmaps) for whichever item currently occupies the matching slot in the
// shown character's CREResource, clearing the icon on empty slots.
void
Game::_UpdateInventoryIcons()
{
	if (fParty == NULL || fParty->CountActors() == 0)
		return;

	Actor* actor = _ShownActor();
	if (actor == NULL || actor->CRE() == NULL)
		return;

	Window* window = GUI::Get()->GetAuxWindow("GUIINV", 2);
	if (window == NULL)
		return;

	CREResource* cre = actor->CRE();

	for (const auto& entry : kInvSlotControls)
		_SetSlotIcon(window, cre, entry.controlID, entry.creSlot);

	_UpdatePaperdoll(window, actor);
	_UpdateInventoryLabels(window, actor);
	_UpdateGroundItemSlots(window, actor);
}


// GUI::ControlInvoked() routes clicks on GUIINV slot buttons here (window
// 2). Click-to-pick, click-to-place: the first click on a non-empty slot
// picks the item up (it rides the cursor via GUI::SetDragBitmap()); the
// next click drops it into the clicked slot, swapping with whatever's
// there. A rejected drop (incompatible slot, e.g. armor onto a weapon
// slot) keeps the item on the cursor so the player can try elsewhere;
// clicking the origin slot again puts it back. Operates on whichever
// party member the portrait column currently shows (_ShownActor()).
void
Game::InventoryControlInvoked(uint32 controlID, uint16 windowID)
{
	if (windowID == kInvInfoWindowID) {
		// Any button in the examine popup just closes it.
		GUI::Get()->HideAuxWindow("GUIINV", kInvInfoWindowID);
		return;
	}

	if (windowID == 0) {
		// Navigating away from the Inventory - a half-finished drag
		// doesn't survive it, same as re-toggling this same window does.
		fInvDragSlot = -1;
		GUI::Get()->SetDragBitmap(NULL);
		_AuxCommandBarInvoked(controlID);
		return;
	}

	if (windowID == 1 && controlID <= 3) {
		ShowCharacter((uint16)controlID); // portrait column
		return;
	}

	if (windowID == 2 && _IsGroundItemSlotControl(controlID)) {
		if (GUI::Get()->IsDraggingItem())
			DropHeldItemOnGround();
		return;
	}

	int32 slot = _CreSlotForControl(controlID);
	if (slot < 0)
		return;

	if (fParty == NULL || fParty->CountActors() == 0)
		return;
	Actor* actor = _ShownActor();
	if (actor == NULL || actor->CRE() == NULL)
		return;

	if (!GUI::Get()->IsDraggingItem()) {
		IE::item item;
		if (!actor->CRE()->GetItemAtSlot((uint32)slot, item))
			return; // empty slot - nothing to pick up
		fInvDragSlot = slot;
		GUI::Get()->SetDragBitmap(_MakeItemIcon(item.name));
		std::cout << actor->Name() << " picks up " << _ItemDisplayName(item.name)
			<< std::endl;
		return;
	}

	// Fetched before MoveItemToSlot() - on a swap, fInvDragSlot no longer
	// holds this item afterwards (it holds whatever was in `slot`).
	IE::item draggedItem;
	actor->CRE()->GetItemAtSlot((uint32)fInvDragSlot, draggedItem);
	std::string itemName = _ItemDisplayName(draggedItem.name);

	if (actor->MoveItemToSlot((uint32)fInvDragSlot, (uint32)slot)) {
		fInvDragSlot = -1;
		GUI::Get()->SetDragBitmap(NULL);
		_UpdateInventoryIcons();
		std::cout << actor->Name() << " puts " << itemName << " in slot " << slot
			<< ((uint32)slot == kSlotWeaponFirst ? " (equipped weapon)" : "")
			<< std::endl;
	} else {
		// Drop rejected (incompatible slot) - keep holding the item.
		std::cout << actor->Name() << " can't put " << itemName << " there"
			<< std::endl;
	}
}


// Right-click on a GUIINV window-2 slot: if the player is mid-drag, drop
// the held item back where it came from (BG2's right-click-cancels); if
// the slot holds an item, examine it (opens the GUIINVHI popup).
void
Game::InventoryControlRightClicked(uint32 controlID, uint16 windowID)
{
	if (windowID != 2)
		return;

	if (GUI::Get()->IsDraggingItem()) {
		fInvDragSlot = -1;
		GUI::Get()->SetDragBitmap(NULL);
		return;
	}

	int32 slot = _CreSlotForControl(controlID);
	if (slot < 0 || fParty == NULL || fParty->CountActors() == 0)
		return;
	Actor* actor = _ShownActor();
	if (actor == NULL || actor->CRE() == NULL)
		return;

	IE::item item;
	if (actor->CRE()->GetItemAtSlot((uint32)slot, item))
		_ShowItemInfo(item.name);
}


// Populates and shows GUIINV's examine popup (window 5) for an item.
void
Game::_ShowItemInfo(const res_ref& itemName)
{
	ITMResource* itm = gResManager->GetITM(itemName);
	if (itm == NULL)
		return;

	GUI::Get()->ShowAuxWindow("GUIINV", kInvInfoWindowID);
	Window* window = GUI::Get()->GetAuxWindow("GUIINV", kInvInfoWindowID);
	if (window == NULL) {
		gResManager->ReleaseResource(itm);
		return;
	}

	Label* title = dynamic_cast<Label*>(window->GetControlByID(kInvInfoTitleID));
	if (title != NULL)
		title->SetText(_ItemDisplayName(itm, itemName));

	for (uint32 id : { kInvInfoBlankLabel1ID, kInvInfoBlankLabel2ID }) {
		Label* label = dynamic_cast<Label*>(window->GetControlByID(id));
		if (label != NULL)
			label->SetText("");
	}

	Button* icon = dynamic_cast<Button*>(window->GetControlByID(kInvInfoIconID));
	if (icon != NULL)
		icon->SetIcon(_MakeItemIcon(itemName), true);

	TextArea* description =
		dynamic_cast<TextArea*>(window->GetControlByID(kInvInfoTextID));
	if (description != NULL) {
		description->ClearText();
		std::string text = IDTable::GetDialog(itm->DescriptionRef());
		description->AddText(text.empty() ? "(No description)" : text.c_str());
		description->ScrollTo(0, 0);
	}

	gResManager->ReleaseResource(itm);
}


// Hover enter/leave on a GUIINV window-2 slot: show the item's name in a
// tooltip next to the cursor while the pointer is over a filled slot.
void
Game::InventoryControlHovered(uint32 controlID, uint16 windowID, bool inside)
{
	if (windowID != 2 || !inside) {
		GUI::Get()->SetHoverTooltip("");
		return;
	}

	int32 slot = _CreSlotForControl(controlID);
	if (slot < 0 || fParty == NULL || fParty->CountActors() == 0)
		return;
	Actor* actor = _ShownActor();
	if (actor == NULL || actor->CRE() == NULL)
		return;

	IE::item item;
	if (!actor->CRE()->GetItemAtSlot((uint32)slot, item)) {
		GUI::Get()->SetHoverTooltip("");
		return;
	}

	ITMResource* itm = gResManager->GetITM(item.name);
	std::string name = _ItemDisplayName(itm, item.name);
	if (itm != NULL)
		gResManager->ReleaseResource(itm);
	GUI::Get()->SetHoverTooltip(name);
}


// Dropping an item: a click on the inventory window that didn't land on
// any slot, while an item rides the cursor. The held item leaves the
// shown character's inventory and becomes a loose pile on the area floor
// at that character's feet (picked back up by clicking it in the world -
// see AreaRoom::PickUpGroundPile()). No-op (item stays on the cursor) if
// the current room isn't an explorable area.
void
Game::DropHeldItemOnGround()
{
	if (!GUI::Get()->IsDraggingItem() || fInvDragSlot < 0)
		return;

	Actor* actor = _ShownActor();
	AreaRoom* room = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
	if (actor == NULL || actor->CRE() == NULL || room == NULL)
		return;

	IE::item item;
	if (!actor->TakeItemFromSlot((uint32)fInvDragSlot, item))
		return;

	room->AddGroundItem(item, actor->Position());
	fInvDragSlot = -1;
	GUI::Get()->SetDragBitmap(NULL);
	_UpdateInventoryIcons();
}


// The wearer's body-size letter for WPxxx paperdoll overlay resrefs
// below - real BG2 keys this off the avatar animation id via a table
// hardcoded in the executable (no data file for it), so this is a
// simplification keyed off race instead, covering every playable PC
// race. Halflings get their own "H" variant (helmets are the one
// exception - they fall back to the gnome/"S" files - but this codebase
// doesn't composite helmets).
static const char*
_PaperdollSizeCode(const Actor* actor)
{
	switch (actor->CRE()->Race()) {
		case 5: // HALFLING
			return "H";
		case 6: // GNOME
			return "S";
		default:
			return "M";
	}
}


// Composites one equipped item's paperdoll overlay (a "WP" + size +
// animation-code + suffix BAM, e.g. "WPMS1INV" for a medium character's
// long sword) onto `canvas`. The frame's own stored center offset is
// the target position outright (negated, no separate canvas anchor
// involved) - confirmed against GemRB's AnimationFactory::
// GetPaperdollImage()/Button::DrawSelf(), which blit every paperdoll
// layer at the same button-relative point and let each sprite's own
// stored offset place it.
static void
_CompositePaperdollOverlay(Bitmap* canvas, const char* sizeCode,
		const std::string& animationCode, const char* suffix)
{
	if (animationCode.empty())
		return;

	std::string resRef = std::string("WP") + sizeCode + animationCode + suffix;
	BAMResource* bam = gResManager->GetBAM(resRef.c_str());
	if (bam == nullptr)
		return;

	Bitmap* frame = bam->FrameForCycle(0, 0);
	if (frame != nullptr) {
		GFX::rect frameRect = frame->Frame();
		GFX::point where(-(frameRect.x + frame->Width() / 2),
				-(frameRect.y + frame->Height() / 2));
		frame->BlitTo(canvas, where);
		frame->Release();
	}
	gResManager->ReleaseResource(bam);
}


// Swaps the paperdoll control's fixed CHU-authored placeholder (CIFF4INV,
// a generic doll unrelated to the shown character) for the real thing:
// the actual character's own class/race/gender/armor identity (see
// AnimationFactory::PaperdollName()), rendered from its PLT resource and
// recolored with their own CRE colors (see PLTResource::Image()), with
// the equipped weapon and shield/off-hand item composited on top (see
// _CompositePaperdollOverlay()). Helmet/armor overlays aren't - BG2
// already bakes worn armor into the base doll's own resref, and helmets
// aren't handled yet.
void
Game::_UpdatePaperdoll(Window* window, Actor* actor)
{
	Button* button = dynamic_cast<Button*>(window->GetControlByID(kInvPaperdollID));
	if (button == NULL)
		return;

	Bitmap* icon = nullptr;
	std::string name = actor->PaperdollName();
	if (Core::Get()->Game() == game::GAME_BALDURSGATE) {
		// Baldur's Gate 1 has paperdolls in BAM resources instead
		// TODO: it only shows the upper body for now, and in the wrong color, too
		BAMResource* bam = gResManager->GetBAM(name.c_str());
		if (bam != nullptr) {
			icon = bam->FrameForCycle(0, 0);
			gResManager->ReleaseResource(bam);
		} else {
			std::cerr << "Game::_UpdatePaperdoll(): no BAM resource named "
									<< name << std::endl;
		}
	} else {
		PLTResource* plt = gResManager->GetPLT(name.c_str());
		if (plt == NULL && name.length() >= 5) {
			// Not every class-letter x armor-digit paperdoll exists in every
			// install; fall back to the unarmored (digit 1) doll rather than
			// leaving the paperdoll blank.
			name[4] = '1';
			plt = gResManager->GetPLT(name.c_str());
		}
		if (plt != NULL) {
				icon = plt->Image(actor->CRE()->Colors());
				gResManager->ReleaseResource(plt);
			} else {
				std::cerr << "Game::_UpdatePaperdoll(): no PLT resource named "
						<< name << std::endl;
		}

		if (icon != nullptr) {
			const char* sizeCode = _PaperdollSizeCode(actor);

			ITMResource* weapon = actor->EquippedWeapon();
			if (weapon != nullptr) {
				_CompositePaperdollOverlay(icon, sizeCode, weapon->Animation(), "INV");
				gResManager->ReleaseResource(weapon);
			}

			IE::item shieldItem;
			if (actor->CRE()->GetItemAtSlot(kSlotShield, shieldItem)) {
				ITMResource* shield = gResManager->GetITM(shieldItem.name);
				if (shield != nullptr) {
					// 0x000c: real shield, uses the same "INV" suffix as
					// the weapon; anything else in this slot is an
					// off-hand weapon (dual-wielding), which uses "OIN".
					const char* suffix = shield->ItemType() == 0x000c ? "INV" : "OIN";
					_CompositePaperdollOverlay(icon, sizeCode, shield->Animation(), suffix);
					gResManager->ReleaseResource(shield);
				}
			}
		}
	}

	// coverBackground: the paperdoll control's CHU bitmap is just a
	// generic placeholder doll (CIFF4INV) - hide it so it can't show
	// through the real doll's transparent areas.
	button->SetIcon(icon, true);
}


// Fills in the two GUIINV labels whose CHU-authored text_ref resolves to
// a literal "(No text)" TLK placeholder - real BG2 sets these from code,
// not from static CHU data, same as the item icons above.
void
Game::_UpdateInventoryLabels(Window* window, Actor* actor)
{
	Label* nameLabel = dynamic_cast<Label*>(window->GetControlByID(kInvNameLabelID));
	if (nameLabel != NULL)
		nameLabel->SetText(actor->LongName());

	Label* acLabel = dynamic_cast<Label*>(window->GetControlByID(kInvACLabelID));
	if (acLabel != NULL)
		acLabel->SetText(std::to_string(actor->CRE()->AC().effective));

	Label* hpLabel = dynamic_cast<Label*>(window->GetControlByID(kInvHPCurrentLabelID));
	if (hpLabel != nullptr)
		hpLabel->SetText(std::to_string(actor->CRE()->CurrentHitPoints()));

	Label* hpMaxLabel = dynamic_cast<Label*>(window->GetControlByID(kInvHPMaxLabelID));
	if (hpMaxLabel != nullptr)
		hpMaxLabel->SetText(std::to_string(actor->CRE()->MaxHitPoints()));

	// Party-wide, not per-actor - see Core::AddPartyGold()'s own comment.
	Label* goldLabel = dynamic_cast<Label*>(window->GetControlByID(kInvGoldLabelID));
	if (goldLabel != nullptr)
		goldLabel->SetText(std::to_string(Core::Get()->PartyGold()));
}


// Populates the parts of GUIREC's "General" tab (window 2) identified
// with confidence so far - the 6 ability scores (row-by-row position
// matching against the already-correctly-localized stat name labels next
// to them, e.g. "Forza"/STR) and the AC badge (same shield-shaped-badge
// reasoning as GUIINV's). Not done yet, deliberately (same caution as
// GUIINV's unmapped equipment slots): the name banner (almost certainly
// hits the same REALMS font bug as GUIINV, unverified), the class/race/
// level text block (id 268435470/table around y=452 - "Umano" already
// shows there for a human character, but it's unclear whether that's a
// real dynamic display or just this CHU's static per-field default,
// since no code sets it and a non-human party member wasn't tested), and
// a second AC-adjacent 2-line badge (id 268435497/498, same "unclear
// what it's for" situation as GUIINV's second badge).
void
Game::_UpdateRecordLabels()
{
	if (fParty == NULL || fParty->CountActors() == 0)
		return;

	Actor* actor = _ShownActor();
	if (actor == NULL || actor->CRE() == NULL)
		return;

	Window* window = GUI::Get()->GetAuxWindow("GUIREC", 2);
	if (window == NULL)
		return;

	Label* nameLabel = dynamic_cast<Label*>(window->GetControlByID(kRecNameLabelID));
	if (nameLabel != NULL)
		nameLabel->SetText(actor->LongName());

	_UpdateAbilityScoreLabels(window, actor->CRE());

	Label* acLabel = dynamic_cast<Label*>(window->GetControlByID(kRecACLabelID));
	if (acLabel != NULL)
		acLabel->SetText(std::to_string(actor->CRE()->AC().effective));

	_UpdateClassRaceLevelLabels(window, actor);
	_UpdateSavesAndResistances(window, actor->CRE());
}


// The 6 ability-score value labels, row-by-row (top to bottom) against
// the static "Forza/Destrezza/Costituzione/Intelligenza/Saggezza/
// Carisma" name labels beside them, confirmed via a real GUIREC.CHU
// control dump (each value label sits ~10px below its matching name
// label, same 37px row spacing for both columns).
void
Game::_UpdateAbilityScoreLabels(Window* window, CREResource* cre)
{
	BaseAttributes attrs;
	cre->GetAttributes(attrs);

	static const uint32 kStatLabelIDs[] = {
		268435503, 268435465, 268435466, 268435467, 268435468, 268435469
	};
	const int8 statValues[] = {
		attrs.strength, attrs.dexterity, attrs.constitution,
		attrs.intelligence, attrs.wisdom, attrs.charisma
	};
	for (int i = 0; i < 6; i++) {
		Label* label = dynamic_cast<Label*>(window->GetControlByID(kStatLabelIDs[i]));
		if (label == NULL)
			continue;
		std::string text = std::to_string(statValues[i]);
		// Exceptional strength (18/xx) - only meaningful at STR 18.
		if (i == 0 && statValues[0] == 18 && attrs.strength_bonus > 0)
			text += "/" + std::to_string(attrs.strength_bonus);
		label->SetText(text);
	}
}


// Class/Race/Level 3-line block (ids 471/472/473, y=322/345/368,
// confirmed via a real GUIREC.CHU dump). The CHU's own static default
// for the race line ("Umano"/Human) looked plausible at first for a
// human test character, but showing a second, non-human party member
// (Imoen, a half-elf) still showed "Umano" unchanged - proving it's
// just this control's authored default, not real data, and needs to
// be set from code like everything else here.
// IDTable::RaceAt()/ClassAt() return the raw RACE.IDS/CLASS.IDS
// token (e.g. "HALF_ELF", "FIGHTER_CLERIC") - there's no RACE.2DA in
// this installation to resolve a localized display string from, so
// _TitleCaseIDSName() below just turns "HALF_ELF" into "Half Elf"
// (underscores to spaces, title case) rather than show the raw
// all-caps token. Not localized to Italian like the rest of this
// screen - declared simplification, same spirit as other "real data,
// imperfect presentation" deviations already on the roadmap.
void
Game::_UpdateClassRaceLevelLabels(Window* window, Actor* actor)
{
	Label* classLabel = dynamic_cast<Label*>(window->GetControlByID(kRecClassLabelID));
	if (classLabel != NULL)
		classLabel->SetText(_TitleCaseIDSName(IDTable::ClassAt(actor->CRE()->Class())));

	Label* raceLabel = dynamic_cast<Label*>(window->GetControlByID(kRecRaceLabelID));
	if (raceLabel != NULL)
		raceLabel->SetText(_TitleCaseIDSName(IDTable::RaceAt(actor->CRE()->Race())));

	Label* levelLabel = dynamic_cast<Label*>(window->GetControlByID(kRecLevelLabelID));
	if (levelLabel != NULL)
		levelLabel->SetText("Livello " + std::to_string(actor->CRE()->Level()));
}


// Saving throws + damage resistances (id 45, a scrollable text_area
// with its own scrollbar at id 46, confirmed via a real GUIREC.CHU
// dump) - real BG2 lists these as plain scrollable text rather than
// individual labels, unlike the rest of this tab. Both structs
// (SaveVersus/Resistances) were already fully read by CREResource,
// just never displayed anywhere until now.
void
Game::_UpdateSavesAndResistances(Window* window, CREResource* cre)
{
	TextArea* statsArea = dynamic_cast<TextArea*>(window->GetControlByID(kRecStatsAreaID));
	if (statsArea == NULL)
		return;

	statsArea->ClearText();

	SaveVersus saves = cre->Saves();
	const std::pair<const char*, uint8> saveLines[] = {
		{ "Morte", saves.death },
		{ "Bacchette", saves.wands },
		{ "Polimorfismo", saves.poly },
		{ "Soffio", saves.breath },
		{ "Incantesimi", saves.spell }
	};
	statsArea->AddText("Tiri Salvezza");
	for (const auto& line : saveLines)
		statsArea->AddText((std::string(line.first) + ": " + std::to_string(line.second)).c_str());

	Resistances res = cre->DamageResistances();
	const std::pair<const char*, uint8> resistanceLines[] = {
		{ "Contundente", res.crushing },
		{ "Perforante", res.piercing },
		{ "Tagliente", res.slashing },
		{ "Missili", res.missile },
		{ "Fuoco", res.fire },
		{ "Freddo", res.cold },
		{ "Elettricita", res.electricity },
		{ "Acido", res.acid },
		{ "Magia", res.magic },
		{ "Fuoco magico", res.magic_fire },
		{ "Freddo magico", res.magic_cold }
	};
	statsArea->AddText("Resistenze");
	for (const auto& line : resistanceLines) {
		statsArea->AddText(
			(std::string(line.first) + ": " + std::to_string(line.second) + "%").c_str());
	}

	// AddText() auto-scrolls to the newest line (fine for the
	// dialogue TextArea it was written for) - scroll back to the
	// top so Saving Throws, not the tail of Resistances, is what's
	// visible when the tab first opens.
	statsArea->ScrollTo(0, 0);
}


// "HALF_ELF" -> "Half Elf" - turns a raw *.IDS symbolic name (all caps,
// underscore-separated) into something readable, absent a 2DA to look up
// a real localized display string for it.
std::string
Game::_TitleCaseIDSName(const std::string& idsName)
{
	std::string result = idsName;
	bool startOfWord = true;
	for (char& c : result) {
		if (c == '_') {
			c = ' ';
			startOfWord = true;
		} else if (startOfWord) {
			c = toupper(c);
			startOfWord = false;
		} else {
			c = tolower(c);
		}
	}
	return result;
}


// Looks up controlID's Button in window and sets its icon from whatever
// item (if any) sits in creSlot of cre - shared by the general-grid loop
// above and by individual equipment-slot mappings as they get confirmed.
void
Game::_SetSlotIcon(Window* window, CREResource* cre, uint32 controlID,
	uint32 creSlot)
{
	Button* button = dynamic_cast<Button*>(window->GetControlByID(controlID));
	if (button == NULL)
		return;

	// Lets a real press-drag-release mouse gesture move an item between
	// slots in one motion, not just two separate clicks - see Button::
	// SetDragCapture()'s own comment. Set on every refresh (redundant
	// after the first, harmless) since this is the one place that walks
	// every inventory slot control.
	button->SetDragCapture(true);

	IE::item item;
	Bitmap* icon = NULL;
	int count = 0;
	if (cre->GetItemAtSlot(creSlot, item)) {
		icon = _MakeItemIcon(item.name);
		count = item.quantity1;
	}
	button->SetIcon(icon);
	button->SetIconCount(count);

	// The only visible cue that an equip actually took effect, short of
	// attacking to see the animation change: a highlighted border (same
	// mechanism already used for the selected party member's portrait)
	// on whichever weapon-row slot is the one this engine actually
	// wields (kSlotWeaponFirst - EquippedWeapon()/WeaponAnimation()
	// always read that one slot; the other 3 "Weapon2-4" quickslots
	// this CHU shows can hold a spare weapon, but this engine has no
	// quickslot-switching, so an item sitting there has no effect until
	// it's moved into this one). Without this, dropping a weapon into a
	// different quickslot looked identical to a real equip - same icon
	// update, no way to tell them apart.
	if (creSlot >= kSlotWeaponFirst && creSlot < kSlotWeaponFirst + 4)
		button->SetHighlighted(creSlot == kSlotWeaponFirst);
}


// Mirrors whatever's in the ground pile at the shown character's own
// position into the 5 "ground item" slots (ids 68-72, see
// _IsGroundItemSlotControl()) - the same pile AreaRoom's world-click
// handler picks up via GroundPileAtPoint(). Only display: a click there
// is still handled purely as a drop target (InventoryControlInvoked()),
// not as its own pickup source - picking a specific item back up still
// means clicking the pile in the world. No paging if a pile holds more
// than 5 items (the neighboring scrollbar, control id 66, isn't wired
// yet) - declared simplification, not expected to matter for piles
// built up from drops alone.
void
Game::_UpdateGroundItemSlots(Window* window, Actor* actor)
{
	AreaRoom* room = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
	const std::vector<IE::item>* items = NULL;
	if (room != NULL) {
		int32 pileIndex = room->GroundPileAtPoint(actor->Position());
		if (pileIndex >= 0)
			items = &room->GroundPiles()[(size_t)pileIndex].items;
	}

	for (uint32 i = 0; i < 5; i++) {
		Button* button = dynamic_cast<Button*>(window->GetControlByID(68 + i));
		if (button == NULL)
			continue;

		Bitmap* icon = NULL;
		int count = 0;
		if (items != NULL && i < items->size()) {
			icon = _MakeItemIcon((*items)[i].name);
			count = (*items)[i].quantity1;
		}
		button->SetIcon(icon);
		button->SetIconCount(count);
	}
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


bool
Game::Save(const char* name)
{
	if (fParty == NULL || fParty->CountActors() == 0) {
		std::cerr << "Game::Save(): no party to save" << std::endl;
		return false;
	}

	RoomBase* room = Core::Get()->CurrentRoom();
	res_ref areaName(room != NULL ? room->Name() : "");

	GamResource* gam = new GamResource(res_ref("SAVE"));
	gam->SetCurrentArea(areaName);

	for (uint16 i = 0; i < fParty->CountActors(); i++) {
		Actor* actor = fParty->ActorAt(i);

		gam_party_member member;
		member.creName = res_ref(actor->Name());
		member.name = actor->Name();
		member.position = actor->Position();
		member.orientation = (uint16)actor->Orientation();
		member.areaName = areaName;

		gam->AddPartyMember(member, actor->CRE());
	}

	gam->SetVariables(Core::Get()->Vars().All());

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

	delete fParty;
	fParty = new ::Party();

	uint32 count = gam->PartyMemberCount();
	for (uint32 i = 0; i < count; i++) {
		gam_party_member member = gam->PartyMemberAt(i);

		// Actor()'s normal constructor fetches the character's original,
		// unmodified CRE from the game's own files (ResourceManager) -
		// this reuses all of Actor's existing init logic (animation
		// factory, etc.) safely. The saved CRE state (inventory,
		// spellbook, HP, status - everything Phase 1-4 added write
		// support for) is then applied on top of it.
		Actor* actor = new Actor(member.creName.CString(), member.position,
			member.orientation);

		CREResource* savedCre = gam->PartyMemberCRE(i);
		if (savedCre != NULL) {
			actor->CRE()->CopyDataFrom(savedCre);
			gResManager->ReleaseResource(savedCre);
		}

		fParty->AddActor(actor);
	}

	for (const auto& variable : gam->Variables())
		Core::Get()->Vars().Set(variable.first.c_str(), variable.second);

	res_ref area = gam->CurrentArea();
	gResManager->ReleaseResource(gam);
	return Core::Get()->LoadArea(area, "", "");
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


void
Game::AddJournalEntry(uint32 strref)
{
	fJournalEntries.push_back(strref);
}


void
Game::RemoveJournalEntry(uint32 strref)
{
	auto it = std::find(fJournalEntries.begin(), fJournalEntries.end(), strref);
	if (it != fJournalEntries.end())
		fJournalEntries.erase(it);
}


const std::vector<uint32>&
Game::JournalEntries() const
{
	return fJournalEntries;
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
	if (fParty == NULL || index >= fParty->CountActors())
		return;

	Actor* member = fParty->ActorAt(index);
	if (member == NULL)
		return;

	fShownCharacter = index;

	AreaRoom* room = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
	if (room != NULL)
		room->SelectActor(member);

	_RefreshCharacterScreens();
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


// Fills in slot 1's name/status labels with whatever's actually on disk
// at kSaveSlotPath - "Vuoto" (empty) if there's no file there yet,
// otherwise a generic "available" status. Doesn't parse the GAM file to
// show its real character/area/date (that's the multi-slot browser's
// job, not modeled here) - just enough to tell the two states apart.
void
Game::_UpdateSaveLoadLabels(const res_ref& chuName)
{
	Window* window = GUI::Get()->GetAuxWindow(chuName, 0);
	if (window == NULL)
		return;

	std::error_code checkpointError;
	std::filesystem::create_directories(kSavePath, checkpointError);
	std::string saveSlotPath = std::string(kSavePath) + std::string("/") + std::string(kSaveSlotPath);
	std::ifstream file(saveSlotPath);
	bool exists = file.good();

	Label* nameLabel = dynamic_cast<Label*>(window->GetControlByID(kSaveSlot1NameLabelID));
	if (nameLabel != NULL)
		nameLabel->SetText("Slot 1");

	Label* statusLabel = dynamic_cast<Label*>(window->GetControlByID(kSaveSlot1StatusLabelID));
	if (statusLabel != NULL)
		statusLabel->SetText(exists ? "Salvataggio disponibile" : "Vuoto");
}


void
Game::SaveOrLoadControlInvoked(const res_ref& chuName, uint32 controlID,
	uint16 windowID)
{
	if (windowID != 0 || controlID != kSaveConfirmButtonID)
		return;

	// Copy, not reference: chuName came from the very Window that owns
	// the button just clicked (see Control::Invoke()) - Load() below
	// reloads the area (Core::LoadArea()), which rebuilds the whole GUI
	// from scratch (GUI::Load() calls Clear(), destroying every window,
	// that one included) - a lingering reference into it would dangle.
	std::error_code checkpointError;
	std::filesystem::create_directories(kSavePath, checkpointError);
	res_ref chu = chuName;
	bool isSave = chu == res_ref("GUISAVE");
	std::string saveSlotPath = std::string(kSavePath) + std::string("/") + std::string(kSaveSlotPath);
	if (isSave) {
		bool ok = Save(saveSlotPath.c_str());
		std::cout << "Save " << saveSlotPath << ": " << (ok ? "OK" : "FAILED") << std::endl;
		GUI::Get()->ToggleAuxWindowGroup(chu, {0});
	} else {
		bool ok = Load(saveSlotPath.c_str());
		std::cout << "Load " << saveSlotPath << ": " << (ok ? "OK" : "FAILED") << std::endl;
		// Nothing to close here: Load() already rebuilt the GUI from
		// scratch (see above), so there's no aux window left open to
		// hide - trying to would instead freshly reopen a new one.
	}
}


// Fills the journal's main scrollable text_area with every tracked
// entry (Game::JournalEntries(), already maintained by ADDJOURNALENTRY/
// ERASEJOURNALENTRY/SETQUESTDONE since Fase 10 - this is the first GUI
// to actually display it). No Quest/Story/User section split (that
// distinction isn't modeled - see JournalEntries()'s own comment),
// just the whole list in insertion order.
void
Game::_UpdateJournalLabels()
{
	Window* window = GUI::Get()->GetAuxWindow("GUIJRNL", 2);
	if (window == NULL)
		return;

	TextArea* entriesArea = dynamic_cast<TextArea*>(window->GetControlByID(kJournalEntriesAreaID));
	if (entriesArea == NULL)
		return;

	entriesArea->ClearText();
	if (fJournalEntries.empty())
		entriesArea->AddText("Il diario e' vuoto.");
	else {
		for (uint32 strref : fJournalEntries)
			entriesArea->AddText(IDTable::GetDialog(strref).c_str());
	}
}
