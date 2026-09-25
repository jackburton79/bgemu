#include "CharGenScreen.h"

#include "Bitmap.h"
#include "BmpResource.h"
#include "Button.h"
#include "CharGenData.h"
#include "CharacterBuilder.h"
#include "Game.h"
#include "GUI.h"
#include "ResManager.h"
#include "StartingParty.h"
#include "TextArea.h"
#include "Window.h"

#include <algorithm>

// Strrefs of the captions and texts, as GemRB's GUICG scripts set them (the
// strrefs of the loaded TLK, so in its language).
static const uint32 kDoneStrRef = 11973;
static const uint32 kBackStrRef = 15416;
static const uint32 kCancelStrRef = 13727;
static const uint32 kImportStrRef = 13955;
static const uint32 kMaleTextStrRef = 13083;
static const uint32 kFemaleTextStrRef = 13084;
static const uint32 kGenderPromptStrRef = 17236;
static const uint32 kRacePromptStrRef = 17237;
static const uint32 kClassPromptStrRef = 17242;
static const uint32 kMultiClassPromptStrRef = 17244;
static const uint32 kAlignmentPromptStrRef = 9602;
static const uint32 kStartStrRef = 16575;	// the overview's text at the start
static const uint32 kMultiClassButtonStrRef = 11993;
static const uint32 kSpecialistButtonStrRef = 11994;
static const uint32 kGeneralistStrRef = 18039;	// MAGESCH.2DA's generalist
// The summary's labels.
static const uint32 kGenderLabelStrRef = 12135;
static const uint32 kRaceLabelStrRef = 1048;
static const uint32 kClassLabelStrRef = 12136;
static const uint32 kAlignmentLabelStrRef = 1049;
static const uint32 kMaleStrRef = 1050;
static const uint32 kFemaleStrRef = 1051;

// The overview's controls.
static const uint32 kOverviewText = 9;
static const uint32 kOverviewBack = 11;
static const uint32 kOverviewPortrait = 12;
static const uint32 kOverviewImport = 13;
static const uint32 kOverviewCancel = 15;
static const uint32 kOverviewAccept = 8;
// Stage buttons in the order of the stages, and their captions.
struct stage_button { uint32 control; uint32 strRef; };
static const stage_button kStageButtons[] = {
	{ 0, 11956 }, { 1, 11957 }, { 2, 11959 }, { 3, 11958 },
	{ 4, 11960 }, { 5, 17372 }, { 6, 11961 }, { 7, 11963 }, { 8, 11962 }
};

// The controls every choice window has (Done is 0 in all of them); the others
// differ per window.
static const uint32 kDoneControl = 0;
static const uint32 kGenderBack = 6, kGenderText = 5, kMaleControl = 2, kFemaleControl = 3;
static const uint32 kRaceBack = 10, kRaceText = 8, kRaceFirst = 2;
static const uint32 kClassBack = 14, kClassText = 13, kClassFirst = 2;
static const uint32 kClassMulti = 10, kClassSpecialist = 11;
static const uint32 kMultiBack = 14, kMultiText = 12, kMultiFirst = 2;
static const uint32 kAlignmentBack = 13, kAlignmentText = 11, kAlignmentFirst = 2;
static const uint32 kPortraitPicture = 1, kPortraitLeft = 2, kPortraitRight = 3;
static const uint32 kPortraitBack = 5, kPortraitCustom = 6;


static std::vector<int>
_ClassesOfKind(bool multi)
{
	size_t count = 0;
	const CharGenClass* classes = CharGenData::Classes(count);
	std::vector<int> found;
	for (size_t i = 0; i < count; i++) {
		if (classes[i].multi == multi)
			found.push_back((int)i);
	}
	return found;
}


// The text of a strref with the mage's school filled in: the class names of a
// mage read "<MAGESCHOOL>" (a specialist's school; the generalist here, which is
// the only mage the creation makes yet).
static std::string
_ClassText(uint32 strRef)
{
	std::string text = IDTable::GetDialog(strRef);
	const std::string token = "<MAGESCHOOL>";
	const size_t at = text.find(token);
	if (at != std::string::npos)
		text.replace(at, token.size(), IDTable::GetDialog(kGeneralistStrRef));
	return text;
}


static Bitmap*
_LoadImage(const std::string& name)
{
	BMPResource* bmp = gResManager->GetBMP(res_ref(name.c_str()));
	if (bmp == NULL)
		return NULL;
	Bitmap* image = bmp->Image();
	gResManager->ReleaseResource(bmp);
	return image;
}


CharGenScreen::CharGenScreen(Game& game)
	:
	GameScreen(game, "GUICG", { kOverviewWindow }),
	fStep(STEP_GENDER),
	fOutcome(OUTCOME_NONE),
	fGender(0),
	fRace(-1),
	fClass(-1),
	fAlignment(-1),
	fPortrait(-1),
	fOpenWindow(-1)
{
}


CharacterBuilder&
CharGenScreen::_Builder() const
{
	return fGame.Starting().Builder();
}


Window*
CharGenScreen::_Window(uint16 windowID) const
{
	return GUI::Get()->GetAuxWindow(CHUName(), windowID);
}


Button*
CharGenScreen::_Button(uint16 windowID, uint32 controlID) const
{
	Window* window = _Window(windowID);
	return window != NULL ? dynamic_cast<Button*>(window->GetControlByID(controlID)) : NULL;
}


TextArea*
CharGenScreen::_TextArea(uint16 windowID, uint32 controlID) const
{
	Window* window = _Window(windowID);
	return window != NULL ? dynamic_cast<TextArea*>(window->GetControlByID(controlID)) : NULL;
}


void
CharGenScreen::_SetText(uint16 windowID, uint32 controlID, uint32 strRef)
{
	if (Button* button = _Button(windowID, controlID))
		button->SetText(IDTable::GetDialog(strRef));
}


void
CharGenScreen::_SetDescription(uint16 windowID, uint32 controlID, uint32 strRef)
{
	if (TextArea* text = _TextArea(windowID, controlID)) {
		text->ClearText();
		text->AddText(_ClassText(strRef).c_str());
		text->ScrollTo(0, 0);
	}
}


void
CharGenScreen::Begin()
{
	_Builder().Reset();
	fGame.Starting().UseBuilder(false);
	fStep = STEP_GENDER;
	fOutcome = OUTCOME_NONE;
	fGender = 0;
	fRace = fClass = fAlignment = fPortrait = -1;
	fOpenWindow = -1;
	Open();
}


CharGenScreen::Outcome
CharGenScreen::Result() const
{
	return fOutcome;
}


const char*
CharGenScreen::StepName() const
{
	static const char* kNames[] = { "gender", "race", "class", "alignment", "accept" };
	return kNames[fStep];
}


/* virtual */
void
CharGenScreen::Refresh()
{
	_RefreshOverview();
}


std::string
CharGenScreen::_SummaryLine(uint32 labelRef, const std::string& value) const
{
	return IDTable::GetDialog(labelRef) + ": " + value;
}


// The overview: the button of the stage that is due (the others off), the
// summary of what was chosen so far, Back if there is something to go back
// from.
void
CharGenScreen::_RefreshOverview()
{
	for (size_t i = 0; i < sizeof(kStageButtons) / sizeof(kStageButtons[0]); i++) {
		Button* button = _Button(kOverviewWindow, kStageButtons[i].control);
		if (button == NULL)
			continue;
		button->SetText(IDTable::GetDialog(kStageButtons[i].strRef));
		// Abilities to name (between the alignment and Accept) aren't there yet.
		button->SetEnabled((int)i == (int)fStep + (fStep == STEP_ACCEPT ? 4 : 0));
	}
	_SetText(kOverviewWindow, kOverviewImport, kImportStrRef);
	if (Button* import = _Button(kOverviewWindow, kOverviewImport))
		import->SetEnabled(false);
	_SetText(kOverviewWindow, kOverviewCancel, kCancelStrRef);
	if (Button* back = _Button(kOverviewWindow, kOverviewBack))
		back->SetEnabled(fStep != STEP_GENDER);

	if (Button* portrait = _Button(kOverviewWindow, kOverviewPortrait)) {
		Bitmap* image = NULL;
		if (fPortrait >= 0) {
			size_t count = 0;
			const CharGenPortrait* portraits = CharGenData::Portraits(count);
			image = _LoadImage(std::string(portraits[fPortrait].name) + "L");
		}
		if (image == NULL)
			image = _LoadImage("NOPORTLG");
		portrait->SetIcon(image, true);
	}

	if (TextArea* text = _TextArea(kOverviewWindow, kOverviewText)) {
		text->ClearText();
		if (fStep == STEP_GENDER) {
			text->AddText(IDTable::GetDialog(kStartStrRef).c_str());
		} else {
			size_t count = 0;
			if (fGender != 0) {
				text->AddText(_SummaryLine(kGenderLabelStrRef,
					IDTable::GetDialog(fGender == 1 ? kMaleStrRef : kFemaleStrRef)).c_str());
			}
			if (fRace >= 0) {
				const CharGenRace* races = CharGenData::Races(count);
				text->AddText(_SummaryLine(kRaceLabelStrRef,
					IDTable::GetDialog(races[fRace].capRef)).c_str());
			}
			if (fClass >= 0) {
				const CharGenClass* classes = CharGenData::Classes(count);
				text->AddText(_SummaryLine(kClassLabelStrRef,
					_ClassText(classes[fClass].capRef)).c_str());
			}
			if (fAlignment >= 0) {
				const CharGenAlignment* aligns = CharGenData::Alignments(count);
				text->AddText(_SummaryLine(kAlignmentLabelStrRef,
					IDTable::GetDialog(aligns[fAlignment].capRef)).c_str());
			}
		}
		text->ScrollTo(0, 0);
	}
}


void
CharGenScreen::_OpenStage(Step step)
{
	switch (step) {
		case STEP_GENDER:
			_ShowGender();
			break;
		case STEP_RACE:
			_ShowRace();
			break;
		case STEP_CLASS:
			_ShowClass();
			break;
		case STEP_ALIGNMENT:
			_ShowAlignment();
			break;
		case STEP_ACCEPT:
			_Finish();
			break;
	}
}


void
CharGenScreen::_CloseChoice()
{
	if (fOpenWindow >= 0)
		GUI::Get()->HideAuxWindow(CHUName(), (uint16)fOpenWindow);
	fOpenWindow = -1;
}


// Marks the chosen one of a run of radio buttons (control ids `first` on) and
// leaves the others off.
void
CharGenScreen::_Latch(uint16 windowID, uint32 first, size_t count, int chosen)
{
	for (size_t i = 0; i < count; i++) {
		if (Button* button = _Button(windowID, first + (uint32)i))
			button->SetLatched((int)i == chosen);
	}
}


// The header of a choice window: Done (off until something is chosen) and Back.
static void
_ChoiceWindowButtons(CharGenScreen* /*screen*/, Button* done, Button* back)
{
	if (done != NULL) {
		done->SetText(IDTable::GetDialog(kDoneStrRef));
		done->SetEnabled(false);
	}
	if (back != NULL)
		back->SetText(IDTable::GetDialog(kBackStrRef));
}


void
CharGenScreen::_ShowGender()
{
	_CloseChoice();
	GUI::Get()->ShowAuxWindow(CHUName(), kGenderWindow);
	fOpenWindow = kGenderWindow;

	_ChoiceWindowButtons(this, _Button(kGenderWindow, kDoneControl),
		_Button(kGenderWindow, kGenderBack));
	_SetDescription(kGenderWindow, kGenderText, kGenderPromptStrRef);
	_Latch(kGenderWindow, kMaleControl, 2, fGender - 1);
	if (fGender != 0) {
		if (Button* done = _Button(kGenderWindow, kDoneControl))
			done->SetEnabled(true);
	}
}


void
CharGenScreen::_ShowPortrait()
{
	_CloseChoice();
	GUI::Get()->ShowAuxWindow(CHUName(), kPortraitWindow);
	fOpenWindow = kPortraitWindow;

	_ChoiceWindowButtons(this, _Button(kPortraitWindow, kDoneControl),
		_Button(kPortraitWindow, kPortraitBack));
	if (Button* done = _Button(kPortraitWindow, kDoneControl))
		done->SetEnabled(true);
	// The custom portrait (a list of the portrait files) isn't there.
	if (Button* custom = _Button(kPortraitWindow, kPortraitCustom)) {
		custom->SetText(IDTable::GetDialog(17545));
		custom->SetEnabled(false);
	}

	// Starts on the first portrait of this gender.
	size_t count = 0;
	const CharGenPortrait* portraits = CharGenData::Portraits(count);
	if (fPortrait < 0 || portraits[fPortrait].gender != fGender) {
		fPortrait = -1;
		_SelectPortrait(1);
	} else
		_ShowPortraitPicture();
}


// The next (+1) or previous (-1) portrait of the chosen gender, round the list.
void
CharGenScreen::_SelectPortrait(int step)
{
	size_t count = 0;
	const CharGenPortrait* portraits = CharGenData::Portraits(count);
	int index = fPortrait;
	for (size_t tried = 0; tried < count; tried++) {
		index = (index + step + (int)count) % (int)count;
		if (portraits[index].gender == fGender) {
			fPortrait = index;
			break;
		}
	}
	_ShowPortraitPicture();
}


void
CharGenScreen::_ShowPortraitPicture()
{
	Button* picture = _Button(kPortraitWindow, kPortraitPicture);
	if (picture == NULL || fPortrait < 0)
		return;
	size_t count = 0;
	const CharGenPortrait* portraits = CharGenData::Portraits(count);
	picture->SetIcon(_LoadImage(std::string(portraits[fPortrait].name) + "G"), true);
}


void
CharGenScreen::_ShowRace()
{
	_CloseChoice();
	GUI::Get()->ShowAuxWindow(CHUName(), kRaceWindow);
	fOpenWindow = kRaceWindow;

	_ChoiceWindowButtons(this, _Button(kRaceWindow, kDoneControl),
		_Button(kRaceWindow, kRaceBack));
	size_t count = 0;
	const CharGenRace* races = CharGenData::Races(count);
	for (size_t i = 0; i < count; i++) {
		if (Button* button = _Button(kRaceWindow, kRaceFirst + (uint32)i)) {
			button->SetText(IDTable::GetDialog(races[i].nameRef));
			button->SetEnabled(true);
		}
	}
	_Latch(kRaceWindow, kRaceFirst, count, fRace);
	if (fRace >= 0) {
		_SetDescription(kRaceWindow, kRaceText, races[fRace].descRef);
		if (Button* done = _Button(kRaceWindow, kDoneControl))
			done->SetEnabled(true);
	} else
		_SetDescription(kRaceWindow, kRaceText, kRacePromptStrRef);
}


void
CharGenScreen::_ShowClass()
{
	_CloseChoice();
	GUI::Get()->ShowAuxWindow(CHUName(), kClassWindow);
	fOpenWindow = kClassWindow;

	_ChoiceWindowButtons(this, _Button(kClassWindow, kDoneControl),
		_Button(kClassWindow, kClassBack));

	size_t count = 0;
	const CharGenClass* classes = CharGenData::Classes(count);
	size_t raceCount = 0;
	const CharGenRace* races = CharGenData::Races(raceCount);
	const int column = fRace >= 0 ? CharGenData::RaceColumn(races[fRace].name) : -1;

	const std::vector<int> singles = _ClassesOfKind(false);
	for (size_t i = 0; i < singles.size(); i++) {
		Button* button = _Button(kClassWindow, kClassFirst + (uint32)i);
		if (button == NULL)
			continue;
		const CharGenClass& entry = classes[singles[i]];
		button->SetText(_ClassText(entry.nameRef));
		// Only a plain yes: a gnome mage (2) is an illusionist, the Specialist
		// window's business.
		button->SetEnabled(column >= 0 && entry.allowed[column] == 1);
	}
	_Latch(kClassWindow, kClassFirst, singles.size(),
		(int)(std::find(singles.begin(), singles.end(), fClass) - singles.begin())
			% (int)std::max<size_t>(singles.size() + 1, 1));

	// Multi Class: on if the race may be any of them.
	bool anyMulti = false;
	for (int index : _ClassesOfKind(true))
		anyMulti = anyMulti || (column >= 0 && classes[index].allowed[column] != 0);
	if (Button* multi = _Button(kClassWindow, kClassMulti)) {
		multi->SetText(IDTable::GetDialog(kMultiClassButtonStrRef));
		multi->SetEnabled(anyMulti);
	}
	// Specialist mages aren't there yet.
	if (Button* specialist = _Button(kClassWindow, kClassSpecialist)) {
		specialist->SetText(IDTable::GetDialog(kSpecialistButtonStrRef));
		specialist->SetEnabled(false);
	}

	if (fClass >= 0 && !classes[fClass].multi) {
		_SetDescription(kClassWindow, kClassText, classes[fClass].descRef);
		if (Button* done = _Button(kClassWindow, kDoneControl))
			done->SetEnabled(true);
	} else
		_SetDescription(kClassWindow, kClassText, kClassPromptStrRef);
}


void
CharGenScreen::_ShowMultiClass()
{
	_CloseChoice();
	GUI::Get()->ShowAuxWindow(CHUName(), kMultiClassWindow);
	fOpenWindow = kMultiClassWindow;

	_ChoiceWindowButtons(this, _Button(kMultiClassWindow, kDoneControl),
		_Button(kMultiClassWindow, kMultiBack));

	size_t count = 0;
	const CharGenClass* classes = CharGenData::Classes(count);
	size_t raceCount = 0;
	const CharGenRace* races = CharGenData::Races(raceCount);
	const int column = fRace >= 0 ? CharGenData::RaceColumn(races[fRace].name) : -1;

	const std::vector<int> multis = _ClassesOfKind(true);
	for (size_t i = 0; i < multis.size(); i++) {
		Button* button = _Button(kMultiClassWindow, kMultiFirst + (uint32)i);
		if (button == NULL)
			continue;
		const CharGenClass& entry = classes[multis[i]];
		button->SetText(_ClassText(entry.nameRef));
		button->SetEnabled(column >= 0 && entry.allowed[column] != 0);
	}
	_Latch(kMultiClassWindow, kMultiFirst, multis.size(),
		(int)(std::find(multis.begin(), multis.end(), fClass) - multis.begin())
			% (int)std::max<size_t>(multis.size() + 1, 1));
	if (fClass >= 0 && classes[fClass].multi) {
		_SetDescription(kMultiClassWindow, kMultiText, classes[fClass].descRef);
		if (Button* done = _Button(kMultiClassWindow, kDoneControl))
			done->SetEnabled(true);
	} else
		_SetDescription(kMultiClassWindow, kMultiText, kMultiClassPromptStrRef);
}


void
CharGenScreen::_ShowAlignment()
{
	_CloseChoice();
	GUI::Get()->ShowAuxWindow(CHUName(), kAlignmentWindow);
	fOpenWindow = kAlignmentWindow;

	_ChoiceWindowButtons(this, _Button(kAlignmentWindow, kDoneControl),
		_Button(kAlignmentWindow, kAlignmentBack));

	size_t count = 0;
	const CharGenAlignment* aligns = CharGenData::Alignments(count);
	for (size_t i = 0; i < count; i++) {
		if (Button* button = _Button(kAlignmentWindow, kAlignmentFirst + (uint32)i)) {
			button->SetText(IDTable::GetDialog(aligns[i].nameRef));
			// What the class allows (ALIGNMNT.2DA).
			button->SetEnabled(_Builder().IsAlignmentAllowed(aligns[i].name));
		}
	}
	_Latch(kAlignmentWindow, kAlignmentFirst, count, fAlignment);
	if (fAlignment >= 0) {
		_SetDescription(kAlignmentWindow, kAlignmentText, aligns[fAlignment].descRef);
		if (Button* done = _Button(kAlignmentWindow, kDoneControl))
			done->SetEnabled(true);
	} else
		_SetDescription(kAlignmentWindow, kAlignmentText, kAlignmentPromptStrRef);
}


// Back from the overview: to the previous stage, its choice forgotten (and
// everything after it).
void
CharGenScreen::_StepBack()
{
	if (fStep == STEP_GENDER)
		return;
	fStep = (Step)((int)fStep - 1);

	CharacterBuilder& builder = _Builder();
	builder.Reset();
	if (fStep <= STEP_ALIGNMENT)
		fAlignment = -1;
	if (fStep <= STEP_CLASS)
		fClass = -1;
	if (fStep <= STEP_RACE)
		fRace = -1;
	if (fStep <= STEP_GENDER) {
		fGender = 0;
		fPortrait = -1;
	}

	// The choices before that stage go back into the builder.
	size_t count = 0;
	if (fGender != 0)
		builder.SetGender(fGender == 1 ? "MALE" : "FEMALE");
	if (fPortrait >= 0) {
		const CharGenPortrait* portraits = CharGenData::Portraits(count);
		builder.SetPortraits(std::string(portraits[fPortrait].name) + "S",
			std::string(portraits[fPortrait].name) + "L");
	}
	if (fRace >= 0)
		builder.SetRace(CharGenData::Races(count)[fRace].name);
	if (fClass >= 0)
		builder.SetClass(CharGenData::Classes(count)[fClass].name);
	_RefreshOverview();
}


// Accept: what the stages after the alignment would have asked is the default
// for now (abilities rolled, the portrait's own colors, no name).
void
CharGenScreen::_Finish()
{
	CharacterBuilder& builder = _Builder();
	if (builder.RollAbilities() == 0)
		return;
	// The name is asked for by a later stage.
	if (builder.Name().empty())
		builder.SetName("Player");

	std::vector<std::string> problems;
	if (!builder.IsComplete(problems))
		return;

	fGame.Starting().UseBuilder(true);
	fOutcome = OUTCOME_DONE;
}


bool
CharGenScreen::_HandleChoice(uint16 windowID, uint32 controlID)
{
	size_t count = 0;
	CharacterBuilder& builder = _Builder();

	if (windowID == kGenderWindow) {
		if (controlID == kMaleControl || controlID == kFemaleControl) {
			fGender = controlID == kMaleControl ? 1 : 2;
			_Latch(kGenderWindow, kMaleControl, 2, fGender - 1);
			_SetDescription(kGenderWindow, kGenderText,
				fGender == 1 ? kMaleTextStrRef : kFemaleTextStrRef);
			if (Button* done = _Button(kGenderWindow, kDoneControl))
				done->SetEnabled(true);
		} else if (controlID == kDoneControl) {
			builder.SetGender(fGender == 1 ? "MALE" : "FEMALE");
			_ShowPortrait();
		} else if (controlID == kGenderBack) {
			_CloseChoice();
			_RefreshOverview();
		}
		return true;
	}

	if (windowID == kPortraitWindow) {
		if (controlID == kPortraitLeft)
			_SelectPortrait(-1);
		else if (controlID == kPortraitRight)
			_SelectPortrait(1);
		else if (controlID == kDoneControl && fPortrait >= 0) {
			const CharGenPortrait* portraits = CharGenData::Portraits(count);
			builder.SetPortraits(std::string(portraits[fPortrait].name) + "S",
				std::string(portraits[fPortrait].name) + "L");
			fStep = STEP_RACE;
			_CloseChoice();
			_RefreshOverview();
		} else if (controlID == kPortraitBack)
			_ShowGender();
		return true;
	}

	if (windowID == kRaceWindow) {
		const CharGenRace* races = CharGenData::Races(count);
		if (controlID >= kRaceFirst && controlID < kRaceFirst + count) {
			fRace = (int)(controlID - kRaceFirst);
			_Latch(kRaceWindow, kRaceFirst, count, fRace);
			_SetDescription(kRaceWindow, kRaceText, races[fRace].descRef);
			if (Button* done = _Button(kRaceWindow, kDoneControl))
				done->SetEnabled(true);
		} else if (controlID == kDoneControl && fRace >= 0) {
			builder.SetRace(races[fRace].name);
			fStep = STEP_CLASS;
			_CloseChoice();
			_RefreshOverview();
		} else if (controlID == kRaceBack) {
			fRace = -1;
			_CloseChoice();
			_RefreshOverview();
		}
		return true;
	}

	if (windowID == kClassWindow || windowID == kMultiClassWindow) {
		const CharGenClass* classes = CharGenData::Classes(count);
		const bool multi = windowID == kMultiClassWindow;
		const std::vector<int> list = _ClassesOfKind(multi);
		const uint32 first = multi ? kMultiFirst : kClassFirst;
		const uint32 back = multi ? kMultiBack : kClassBack;
		const uint32 textControl = multi ? kMultiText : kClassText;
		if (controlID >= first && controlID < first + list.size()) {
			fClass = list[controlID - first];
			_Latch(windowID, first, list.size(), (int)(controlID - first));
			_SetDescription(windowID, textControl, classes[fClass].descRef);
			if (Button* done = _Button(windowID, kDoneControl))
				done->SetEnabled(true);
		} else if (!multi && controlID == kClassMulti) {
			fClass = -1;
			_ShowMultiClass();
		} else if (controlID == kDoneControl && fClass >= 0
				&& classes[fClass].multi == multi) {
			builder.SetClass(classes[fClass].name);
			fStep = STEP_ALIGNMENT;
			_CloseChoice();
			_RefreshOverview();
		} else if (controlID == back) {
			fClass = -1;
			_CloseChoice();
			_RefreshOverview();
		}
		return true;
	}

	if (windowID == kAlignmentWindow) {
		const CharGenAlignment* aligns = CharGenData::Alignments(count);
		if (controlID >= kAlignmentFirst && controlID < kAlignmentFirst + count) {
			fAlignment = (int)(controlID - kAlignmentFirst);
			_Latch(kAlignmentWindow, kAlignmentFirst, count, fAlignment);
			_SetDescription(kAlignmentWindow, kAlignmentText, aligns[fAlignment].descRef);
			if (Button* done = _Button(kAlignmentWindow, kDoneControl))
				done->SetEnabled(true);
		} else if (controlID == kDoneControl && fAlignment >= 0) {
			builder.SetAlignment(aligns[fAlignment].name);
			fStep = STEP_ACCEPT;
			_CloseChoice();
			_RefreshOverview();
		} else if (controlID == kAlignmentBack) {
			fAlignment = -1;
			_CloseChoice();
			_RefreshOverview();
		}
		return true;
	}
	return false;
}


/* virtual */
bool
CharGenScreen::ControlInvoked(uint16 windowID, uint32 controlID)
{
	// A button that is off (a class the race can't be, an alignment the class
	// forbids, a stage not due) does nothing.
	if (Button* button = _Button(windowID, controlID)) {
		if (!button->Enabled())
			return true;
	}

	if (windowID != kOverviewWindow)
		return _HandleChoice(windowID, controlID);

	if (controlID == kOverviewCancel) {
		_CloseChoice();
		fOutcome = OUTCOME_CANCELLED;
	} else if (controlID == kOverviewBack) {
		_StepBack();
	} else {
		for (size_t i = 0; i < sizeof(kStageButtons) / sizeof(kStageButtons[0]); i++) {
			if (kStageButtons[i].control != controlID)
				continue;
			// Only the button of the stage due does anything.
			if ((int)i == (int)fStep + (fStep == STEP_ACCEPT ? 4 : 0))
				_OpenStage(fStep);
			break;
		}
	}
	return true;
}
