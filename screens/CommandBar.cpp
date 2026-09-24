#include "CommandBar.h"

#include "Core.h"
#include "Game.h"
#include "GameScreen.h"
#include "InventoryScreen.h"
#include "JournalScreen.h"
#include "Object.h"
#include "Party.h"
#include "RecordScreen.h"
#include "Actor.h"
#include "ScreenManager.h"
#include "Script.h"


namespace {

struct Button { uint32 controlID; void (*action)(); };

// Shared by both bars.
const Button kSharedButtons[] = {
	{ 1, [] { Core::Get()->LoadWorldMap(); } },
	{ 2, [] { Game::Get()->Screens().Toggle<JournalScreen>(); } },
	{ 3, [] { Game::Get()->Screens().Toggle<InventoryScreen>(); } },
	{ 4, [] { Game::Get()->Screens().Toggle<RecordScreen>(); } },
	{ 5, [] { Game::Get()->Screens().Toggle("GUIMG"); } },
	{ 6, [] { Game::Get()->Screens().Toggle("GUIPR"); } },
	{ 7, [] { Game::Get()->Screens().Toggle("GUISAVE"); } },
};

// Only on the HUD: Pause (9) and Rest (11).
const Button kHUDButtons[] = {
	{ 9, [] { Core::Get()->TogglePause(); } },
	{ 11, [] { CommandBar::Rest(); } },
};

// Only on a panel's copy: Rest, under a different id (9, GUIRSBUT).
const Button kPanelButtons[] = {
	{ 9, [] { CommandBar::Rest(); } },
};


template <size_t N>
bool
Invoke(const Button (&buttons)[N], uint32 controlID)
{
	for (const Button& button : buttons) {
		if (button.controlID == controlID) {
			button.action();
			return true;
		}
	}
	return false;
}

}


bool
CommandBar::InvokeOnHUD(uint32 controlID)
{
	return Invoke(kSharedButtons, controlID) || Invoke(kHUDButtons, controlID);
}


bool
CommandBar::InvokeInPanel(uint32 controlID)
{
	return Invoke(kSharedButtons, controlID) || Invoke(kPanelButtons, controlID);
}


void
CommandBar::Rest()
{
	Party* party = Game::Get()->Party();
	if (party == NULL || party->CountActors() == 0)
		return;

	action_params* params = new action_params;
	params->id = 230; // RESTPARTY
	party->ActorAt(0)->AddAction(params);
	params->Release();
}
