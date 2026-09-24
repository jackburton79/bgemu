#pragma once

#include "PanelScreen.h"


// The journal (GUIJRNL): the notes made so far, one chapter at a time (in
// BG2 also one section at a time), oldest or newest first. The notes
// themselves are Game's (Game::Journal()).
class JournalScreen : public PanelScreen {
public:
	JournalScreen(Game& game);

	virtual uint32 CommandBarButton() const;

protected:
	virtual void OnOpen();
	virtual void RefreshContent();
	virtual void PanelControlInvoked(uint16 windowID, uint32 controlID);

private:
	// What is shown: the chapter, the section (BG2 only) and whether the
	// entries are listed newest first.
	int32 fChapter;
	uint8 fSection;
	bool fReverse;
};
