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
#include "NPCRoster.h"
#include "GameJournal.h"
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
#include "SavedGame.h"
#include "StartingParty.h"
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
	fNPCs(NULL),
	fTempState(NULL),
	// 15 Hz, standard Infinity Engine pace (AI_UPDATE_FREQ)
	// IESDP's documented
	// clock model (docs/iesdp-gh-pages/appendices/timers.htm).
	fDelay(67),
	fTestMode(false),
	fJournal(NULL),
	fLoot(NULL),
	fBar(NULL),
	fShownCharacter(0),
	fScreens(NULL),
	fSaves(NULL)
{
	fNPCs = new NPCRoster;
	fJournal = new GameJournal;
	fScreens = new ScreenManager;
	fLoot = new LootWindow;
	fBar = new ActionBar(*this);
	fSaves = new SavedGame(*this);
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
	fStartingParty = new StartingParty;
}


Game::~Game()
{
	TerminateDialog();
	delete fParty;
	delete fNPCs;
	delete fJournal;
	delete fTempState;

	// Same convention as everything else this session is careful to
	// balance, even though it only matters for the ASan leak report at
	// this specific point (the whole process is about to go away
	// regardless).
	ClearAreaCache();
	delete fAreaCache;
	delete fStartingParty;
	delete fScreens;
	delete fLoot;
	delete fBar;
	delete fSaves;
}


void
Game::ClearAreaCache()
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
			assert(fParty == NULL);
			fParty = fStartingParty->Create();
		} catch (...) {
			throw std::runtime_error("Error creating player!");
		}
		fNPCs->LoadStarting(fParty);
		if (!fStartingArea.empty())
			Core::Get()->LoadArea(fStartingArea.c_str(), "", "");
		else if (noNewGame)
			Core::Get()->LoadWorldMap();
		else
			LoadStartingArea();

		if (!fExecFile.empty()) {
			inputConsole->RunFile(fExecFile);
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


NPCRoster&
Game::NPCs()
{
	return *fNPCs;
}


StartingParty&
Game::Starting()
{
	return *fStartingParty;
}


SavedGame&
Game::Saves()
{
	return *fSaves;
}


::Party*
Game::ResetParty()
{
	delete fParty;
	fParty = new ::Party();
	return fParty;
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


void
Game::InitiateDialog(Actor* actor, Actor* target)
{
	assert(fDialog == NULL);

	// One that is leaving the game (EscapeArea) has nothing more to say.
	if (actor->ToBeDestroyed() || target->ToBeDestroyed())
		return;

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
Game::ForgetActor(Actor* actor)
{
	if (fDialog == NULL || !fDialog->Involves(actor))
		return;
	delete fDialog;
	fDialog = NULL;
	GUI::Get()->EnsureShowNormalMessageArea();
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


GameJournal&
Game::Journal()
{
	return *fJournal;
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
Game::MakeNPC(Actor* actor)
{
	if (fParty->HasActor(actor) || fNPCs->Contains(actor))
		return;

	fNPCs->Adopt(actor);
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
	if (fNPCs->Contains(actor)) {
		fNPCs->Remove(actor);
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
	fNPCs->Add(actor);
	if (fShownCharacter >= fParty->CountActors())
		fShownCharacter = 0;
	RefreshHUDPortraits();
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


