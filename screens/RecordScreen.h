#pragma once

#include "PanelScreen.h"

#include <string>

class Actor;
class CREResource;


// The character sheet (GUIREC): the shown character's identity, stats,
// saves and resistances. Its buttons (Dual-Class, Level Up, ...) only carry
// their captions: the windows they would open don't exist yet.
class RecordScreen : public PanelScreen {
public:
	RecordScreen(Game& game);

	virtual uint32 CommandBarButton() const;

protected:
	virtual void RefreshContent();

private:
	void _UpdateButtons(Window* window);
	void _UpdatePortrait(Window* window, Actor* actor);
	void _UpdateAbilityScoreLabels(Window* window, CREResource* cre);
	void _UpdateClassRaceLevelLabels(Window* window, Actor* actor);
	void _UpdateSavesAndResistances(Window* window, CREResource* cre);
	static std::string _TitleCaseIDSName(const std::string& idsName);
};
