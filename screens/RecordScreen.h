#pragma once

#include "PanelScreen.h"

#include <string>

class Actor;
class CREResource;


// The character sheet (GUIREC): the shown character's identity, stats,
// saves and resistances. Level Up and Information work (the latter opens a
// window with the character's kills, favourite spell and weapon, and from it
// the biography); the other buttons (Dual-Class, ...) only carry their
// captions: the windows they would open don't exist yet.
class RecordScreen : public PanelScreen {
public:
	RecordScreen(Game& game);

	virtual uint32 CommandBarButton() const;
	virtual void OnShownCharacterChanged();

protected:
	virtual void OnClose();
	virtual void RefreshContent();
	virtual void PanelControlInvoked(uint16 windowID, uint32 controlID);

private:
	// The Information window (window 4) and the Biography one it opens (12),
	// both shown over the sheet.
	static const uint16 kInfoWindow = 4;
	static const uint16 kBiographyWindow = 12;

	void _OpenInformation();
	void _CloseInformation();
	void _RefreshInformation();
	void _OpenBiography();
	void _CloseBiography();
	std::string _ClassTitle(Actor* actor) const;
	void _UpdateButtons(Window* window, Actor* actor);
	void _UpdatePortrait(Window* window, Actor* actor);
	void _UpdateAbilityScoreLabels(Window* window, CREResource* cre);
	void _UpdateClassRaceLevelLabels(Window* window, Actor* actor);
	void _UpdateSavesAndResistances(Window* window, CREResource* cre);
	static std::string _TitleCaseIDSName(const std::string& idsName);
};
