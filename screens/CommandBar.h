#pragma once

#include "IETypes.h"


// The command bar's actions - Map, Journal, Inventory, Record, the two
// spellbooks, Save, Pause and Rest. The bar exists twice, with the same
// icons and (but for Pause and Rest) the same control ids: on the HUD (GUIW's
// WINDOW_COMMANDS) and as a copy in window 0 of every full-screen panel, so
// the player can still switch screens or rest while one of them is open.
namespace CommandBar {

// A click on the HUD's bar; false if the control isn't one of its buttons.
bool InvokeOnHUD(uint32 controlID);
// A click on the copy embedded in a panel; false if it isn't one of its buttons.
bool InvokeInPanel(uint32 controlID);

// Queues RESTPARTY(230) on the first party member - the same action
// SETAREARESTFLAG/RunActionRestParty implement, triggered from the Rest
// button instead of a script.
void Rest();

}
