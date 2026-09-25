#pragma once

#include "GameScreen.h"

#include <string>
#include <vector>

class Button;
class CharacterBuilder;
class Label;
class TextArea;
class Window;


// Character creation (GUICG.CHU): an overview window (window 0) that lists the
// stages, with the summary of what was chosen, and one window per choice shown
// over it: gender, portrait, race, class (and multiclass), alignment,
// abilities, skills. Each stage is the button of the overview that is due; a choice
// window has its own Done and Back. The choices go into the game's
// CharacterBuilder. Appearance and name (the stages after these) aren't there yet:
// once the skills are set, Accept finishes with the defaults.
// Specialist mages (the Specialist button) and the custom portrait aren't done
// either. BG1's windows only.
class CharGenScreen : public GameScreen {
public:
	enum Outcome { OUTCOME_NONE, OUTCOME_DONE, OUTCOME_CANCELLED };

	CharGenScreen(Game& game);

	// Starts creating a character from nothing.
	void Begin();
	Outcome Result() const;
	// The stage due (gender, race, class, alignment, abilities, skills, accept), for tests.
	const char* StepName() const;
	// The points still to give in the window that is open (moved between the
	// abilities, or the thief skills' or weapon proficiencies'), for tests.
	int PointsLeft() const {
		if (fOpenWindow == kProficienciesWindow)
			return fProficiencyPoints;
		return fOpenWindow == kSkillsWindow ? fSkillPoints : fPointsLeft;
	}

	virtual void Refresh();
	virtual bool ControlInvoked(uint16 windowID, uint32 controlID);

private:
	enum Step {
		STEP_GENDER, STEP_RACE, STEP_CLASS, STEP_ALIGNMENT, STEP_ABILITIES, STEP_SKILLS, STEP_ACCEPT
	};

	// GUICG.CHU's windows.
	static const uint16 kOverviewWindow = 0;
	static const uint16 kGenderWindow = 1;
	static const uint16 kClassWindow = 2;
	static const uint16 kAlignmentWindow = 3;
	static const uint16 kAbilitiesWindow = 4;
	static const uint16 kSkillsWindow = 6;
	static const uint16 kRacialEnemyWindow = 15;
	static const uint16 kProficienciesWindow = 9;
	static const uint16 kRaceWindow = 8;
	static const uint16 kMultiClassWindow = 10;
	static const uint16 kPortraitWindow = 11;

	CharacterBuilder& _Builder() const;
	Window* _Window(uint16 windowID) const;
	Button* _Button(uint16 windowID, uint32 controlID) const;
	Label* _Label(uint16 windowID, uint32 controlID) const;
	TextArea* _TextArea(uint16 windowID, uint32 controlID) const;
	void _SetText(uint16 windowID, uint32 controlID, uint32 strRef);
	void _SetDescription(uint16 windowID, uint32 controlID, uint32 strRef);

	void _RefreshOverview();
	void _OpenStage(Step step);
	void _CloseChoice();
	void _StepBack();

	// The choice windows.
	void _MakeHotSpots(uint16 windowID, uint32 first, size_t count);
	void _ShowGender();
	void _ShowPortrait();
	void _ShowRace();
	void _ShowClass();
	void _ShowMultiClass();
	void _ShowAlignment();
	void _ShowAbilities();
	// Marks button `chosen` of `first`..`first + count - 1` as the selected one.
	void _Latch(uint16 windowID, uint32 first, size_t count, int chosen);
	bool _HandleChoice(uint16 windowID, uint32 controlID);
	void _ShowPortraitPicture();
	void _SelectPortrait(int step);
	// The skills stage: a run of windows (racial enemy, thief skills, proficiencies so far) one after the
	// other, Done going on to the next and Back to the one before.
	enum SkillPage { PAGE_RACIAL_ENEMY, PAGE_THIEF_SKILLS, PAGE_PROFICIENCIES };
	void _ShowSkills();
	void _ShowSkillPage();
	void _NextSkillPage();
	void _PreviousSkillPage();
	void _ShowRacialEnemy();
	void _ShowRacialEnemyList();
	void _ChooseRacialEnemy(int row);
	void _ShowThiefSkills();
	void _MoveThiefSkill(int skill, int step);
	void _DescribeThiefSkill(int skill);
	void _ShowThiefSkillValues();
	void _ShowProficiencies();
	void _MoveProficiency(int proficiency, int step);
	void _DescribeProficiency(int proficiency);
	void _ShowProficiencyValues();
	bool _HandleSkillWindow(uint16 windowID, uint32 controlID);
	// The abilities window: the roll, the points moved between the scores.
	void _RollAbilities();
	void _MovePoint(int ability, int step);
	void _StoreAbilities();
	void _RecallAbilities();
	void _ShowAbilityValues();
	void _DescribeAbility(int ability);
	std::string _AbilityText(int ability) const;
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
	// The abilities window: points taken off a score and not given to another,
	// and the roll kept by Store (with its own points, and the 18/xx).
	int fPointsLeft;
	int fStoredAbilities[6];
	int fStoredPoints;
	int fStoredExtra;
	// The skills stage: its windows in order, the one shown, and the proficiency
	// and thief skill points still to give.
	std::vector<SkillPage> fSkillPages;
	size_t fSkillPage;
	int fProficiencyPoints;
	int fSkillPoints;
	int fEnemy;			// index in CharGenData::HatedRaces(), -1 none
	int fEnemyRow;			// the first one the list shows
	int fOpenWindow;		// the choice window shown over the overview, -1 none
};
