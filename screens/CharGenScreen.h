#pragma once

#include "GameScreen.h"

#include <string>
#include <vector>

class Button;
class CharacterBuilder;
class TextArea;
class Window;


// Character creation (GUICG.CHU): an overview window (window 0) that lists the
// stages, with the summary of what was chosen, and one window per choice shown
// over it: gender, portrait, race, class (and multiclass), alignment. Each
// stage is the button of the overview that is due; a choice window has its own
// Done and Back. The choices go into the game's CharacterBuilder. Abilities,
// skills, appearance and name (the stages after these) aren't there yet: once
// the alignment is chosen, Accept finishes with rolled abilities and the
// defaults. Specialist mages (the Specialist button) and the custom portrait
// aren't done either. BG1's windows only.
class CharGenScreen : public GameScreen {
public:
	enum Outcome { OUTCOME_NONE, OUTCOME_DONE, OUTCOME_CANCELLED };

	CharGenScreen(Game& game);

	// Starts creating a character from nothing.
	void Begin();
	Outcome Result() const;
	// The stage due (gender, race, class, alignment, accept), for tests.
	const char* StepName() const;

	virtual void Refresh();
	virtual bool ControlInvoked(uint16 windowID, uint32 controlID);

private:
	enum Step {
		STEP_GENDER, STEP_RACE, STEP_CLASS, STEP_ALIGNMENT, STEP_ACCEPT
	};

	// GUICG.CHU's windows.
	static const uint16 kOverviewWindow = 0;
	static const uint16 kGenderWindow = 1;
	static const uint16 kClassWindow = 2;
	static const uint16 kAlignmentWindow = 3;
	static const uint16 kRaceWindow = 8;
	static const uint16 kMultiClassWindow = 10;
	static const uint16 kPortraitWindow = 11;

	CharacterBuilder& _Builder() const;
	Window* _Window(uint16 windowID) const;
	Button* _Button(uint16 windowID, uint32 controlID) const;
	TextArea* _TextArea(uint16 windowID, uint32 controlID) const;
	void _SetText(uint16 windowID, uint32 controlID, uint32 strRef);
	void _SetDescription(uint16 windowID, uint32 controlID, uint32 strRef);

	void _RefreshOverview();
	void _OpenStage(Step step);
	void _CloseChoice();
	void _StepBack();

	// The choice windows.
	void _ShowGender();
	void _ShowPortrait();
	void _ShowRace();
	void _ShowClass();
	void _ShowMultiClass();
	void _ShowAlignment();
	// Marks button `chosen` of `first`..`first + count - 1` as the selected one.
	void _Latch(uint16 windowID, uint32 first, size_t count, int chosen);
	bool _HandleChoice(uint16 windowID, uint32 controlID);
	void _ShowPortraitPicture();
	void _SelectPortrait(int step);
	std::string _SummaryLine(uint32 labelRef, const std::string& value) const;
	void _Finish();

	Step fStep;
	Outcome fOutcome;
	// The choice being made in the window that is open, before Done.
	int fGender;			// 1 male, 2 female, 0 none
	int fRace;			// index in CharGenData::Races(), -1 none
	int fClass;			// index in CharGenData::Classes(), -1 none
	int fAlignment;			// index in CharGenData::Alignments(), -1 none
	int fPortrait;			// index in CharGenData::Portraits(), -1 none
	int fOpenWindow;		// the choice window shown over the overview, -1 none
};
