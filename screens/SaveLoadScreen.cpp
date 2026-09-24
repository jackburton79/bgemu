#include "SaveLoadScreen.h"

#include "Button.h"
#include "Game.h"
#include "Label.h"
#include "Window.h"

#include <ctime>
#include <filesystem>
#include <iostream>
#include <sys/stat.h>

// GUISAVE.CHU and GUILOAD.CHU window 0 share the same control-id layout
// (confirmed via a real dump of both games' CHUs) - 4 fixed slot rows,
// each: a name label, a date label, a Save-or-Load button and a Delete
// button. Ids follow GemRB's own GUISAVE.py ctrl_offset table, which is
// also how it's known id 34 is Cancel - this code used to wrongly treat
// it as a shared "confirm" button.
static const uint32 kSaveSlotCount = 4;
static const uint32 kSaveSlotNameLabelID[kSaveSlotCount] =
	{ 268435464, 268435465, 268435466, 268435467 };
static const uint32 kSaveSlotDateLabelID[kSaveSlotCount] =
	{ 268435472, 268435473, 268435474, 268435475 };
static const uint32 kSaveSlotActionButtonID[kSaveSlotCount] = { 26, 27, 28, 29 };
static const uint32 kSaveSlotDeleteButtonID[kSaveSlotCount] = { 30, 31, 32, 33 };
static const uint32 kSaveCancelButtonID = 34;


SaveLoadScreen::SaveLoadScreen(Game& game, bool save)
	:
	GameScreen(game, save ? "GUISAVE" : "GUILOAD", { kWindow }),
	fIsSave(save)
{
}


/* virtual */
uint32
SaveLoadScreen::CommandBarButton() const
{
	return 7;
}


// Fills in every slot row's name/date labels and Save-or-Load/Delete
// button state from whatever's actually on disk at SaveSlotPath(i) -
// "Vuoto" (empty) if there's no file there yet, otherwise the file's own
// last-modified time (this engine's own saves don't carry an in-game
// date of their own to show - see GamResource's header comment - so a
// real-world timestamp is the closest available substitute, same as
// what real BG2's own save browser shows for GUISAVE, if not GUILOAD).
/* virtual */
void
SaveLoadScreen::Refresh()
{
	Window* window = GetWindow(kWindow);
	if (window == NULL)
		return;

	std::error_code checkpointError;
	std::filesystem::create_directories(fGame.SaveDirectory(), checkpointError);

	for (uint32 i = 0; i < kSaveSlotCount; i++) {
		std::string path = fGame.SaveSlotPath(i);
		struct stat info;
		bool exists = ::stat(path.c_str(), &info) == 0;

		Label* nameLabel = dynamic_cast<Label*>(
			window->GetControlByID(kSaveSlotNameLabelID[i]));
		if (nameLabel != NULL)
			nameLabel->SetText("Slot " + std::to_string(i + 1));

		Label* dateLabel = dynamic_cast<Label*>(
			window->GetControlByID(kSaveSlotDateLabelID[i]));
		if (dateLabel != NULL) {
			if (exists) {
				char buffer[64];
				std::time_t modTime = info.st_mtime;
				std::strftime(buffer, sizeof(buffer), "%c", std::localtime(&modTime));
				dateLabel->SetText(buffer);
			} else {
				dateLabel->SetText("Vuoto");
			}
		}

		Button* actionButton = dynamic_cast<Button*>(
			window->GetControlByID(kSaveSlotActionButtonID[i]));
		if (actionButton != NULL) {
			actionButton->SetText(fIsSave ? "Salva" : "Carica");
			// A Save button stays enabled on an empty row too - saving
			// into one is how a new save gets made; a Load button can't
			// do anything useful with a slot that doesn't exist yet.
			actionButton->SetEnabled(fIsSave || exists);
		}

		Button* deleteButton = dynamic_cast<Button*>(
			window->GetControlByID(kSaveSlotDeleteButtonID[i]));
		if (deleteButton != NULL) {
			deleteButton->SetText("Elimina");
			deleteButton->SetEnabled(exists);
		}
	}

	Button* cancelButton = dynamic_cast<Button*>(window->GetControlByID(kSaveCancelButtonID));
	if (cancelButton != NULL)
		cancelButton->SetText("Annulla");
}


/* virtual */
bool
SaveLoadScreen::ControlInvoked(uint16 windowID, uint32 controlID)
{
	if (windowID != kWindow)
		return false;

	// Nothing here may touch the windows after a Load(): it reloads the
	// area (Core::LoadArea()), which rebuilds the whole GUI from scratch
	// (GUI::Load() calls Clear(), destroying every window, the one that
	// owns the button just clicked included).
	if (controlID == kSaveCancelButtonID) {
		Close();
		return true;
	}

	for (uint32 i = 0; i < kSaveSlotCount; i++) {
		if (controlID == kSaveSlotActionButtonID[i]) {
			std::error_code checkpointError;
			std::filesystem::create_directories(fGame.SaveDirectory(), checkpointError);
			std::string path = fGame.SaveSlotPath(i);
			if (fIsSave) {
				bool ok = fGame.Save(path.c_str());
				std::cout << "Save " << path << ": " << (ok ? "OK" : "FAILED") << std::endl;
				Refresh();
			} else {
				bool ok = fGame.Load(path.c_str());
				std::cout << "Load " << path << ": " << (ok ? "OK" : "FAILED") << std::endl;
				// Nothing to refresh here: Load() already rebuilt the GUI
				// from scratch (see above), so there's no aux window left
				// open to update - trying to would instead freshly reopen
				// a new one.
			}
			return true;
		}
		if (controlID == kSaveSlotDeleteButtonID[i]) {
			std::string path = fGame.SaveSlotPath(i);
			std::error_code error;
			std::filesystem::remove(path, error);
			// Its own area-checkpoint archive directory (see Game::
			// Save()'s own comment) goes with it - otherwise a later
			// save reusing this same slot path would inherit whatever
			// this deleted save last left there.
			std::filesystem::remove_all(path + ".arecache", error);
			Refresh();
			return true;
		}
	}
	return true;
}
