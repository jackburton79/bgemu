#include "CharGenScreen.h"

#include "Bitmap.h"
#include "BmpResource.h"
#include "Button.h"
#include "CharGenData.h"
#include "CharacterBuilder.h"
#include "Game.h"
#include "GUI.h"
#include "Label.h"
#include "TextEdit.h"
#include "Scrollbar.h"
#include "ScreenSupport.h"
#include "SPLResource.h"
#include "ResManager.h"
#include "StartingParty.h"
#include "TextArea.h"
#include "Window.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

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
static const uint32 kFighterTypeStrRef = 10174;	// what <FIGHTERTYPE> stands for
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

// The overview's button of each stage (the order of Step): the appearance button
// (6) isn't a stage yet.
static const uint32 kStepButton[] = { 0, 1, 2, 3, 4, 5, 7, 8 };

// The stage an overview button opens, -1 for one that isn't a stage yet.
static int
_StepOfButton(uint32 controlID)
{
	for (size_t i = 0; i < sizeof(kStepButton) / sizeof(kStepButton[0]); i++) {
		if (kStepButton[i] == controlID)
			return (int)i;
	}
	return -1;
}


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
// The abilities window: Reroll, Store and Recall, the text of the ability
// picked, a button (the name) and two arrows for each of the six, and the labels
// with the points left, the scores and the names.
static const uint32 kAbilitiesBack = 36, kAbilitiesText = 29;
static const uint32 kAbilitiesReroll = 2, kAbilitiesStore = 37, kAbilitiesRecall = 38;
static const uint32 kAbilitiesSelectFirst = 30, kAbilitiesArrowFirst = 16;
static const uint32 kAbilitiesPointsLabel = 0x10000002;
static const uint32 kAbilitiesValueLabelFirst = 0x10000003;
static const uint32 kAbilitiesNameLabelFirst = 0x10000009;
static const uint32 kRerollStrRef = 11982, kStoreStrRef = 17373, kRecallStrRef = 17374;
static const uint32 kAbilitiesPromptStrRef = 17247;
// ability.2da of GemRB's unhardcoded tables: the name and the description of
// each ability (Strength, Dexterity, Constitution, Intelligence, Wisdom,
// Charisma), whose text has the range in <MINIMUM> and <MAXIMUM>.
static const uint32 kAbilityNameStrRefs[6] = { 11975, 11977, 11978, 11979, 11980, 11981 };
static const uint32 kAbilityDescStrRefs[6] = { 9582, 9584, 9583, 9585, 9586, 9587 };
// The names as the summary writes them (the table's CAP_REF).
static const uint32 kAbilityCapStrRefs[6] = { 1145, 1151, 1178, 1179, 1180, 1181 };
static const int kNumAbilities = 6;

// The racial enemy window: a scrollbar, six buttons for the list (its art is
// the CHU's), the text area, Back and Done.
static const uint32 kEnemyScroll = 1, kEnemyText = 8, kEnemyBack = 10, kEnemyDone = 11;
static const uint32 kEnemyFirst = 2;
static const int kEnemyRows = 6;
static const uint32 kEnemyPromptStrRef = 17256;
static const uint32 kEnemyLabelStrRef = 15982;

// The spells window: Back and Done, a title, the label with the points to give,
// 24 buttons (six a row) for the spells, and the text area.
static const uint32 kSpellsBack = 29, kSpellsText = 27;
static const uint32 kSpellsFirst = 2;
static const int kSpellButtons = 24;
static const uint32 kSpellsTitleLabel = 0x10000000;
static const uint32 kSpellsPointsLabel = 0x1000001b;
static const uint32 kSpellsLearnStrRef = 17250, kSpellsMemorizeStrRef = 17253;
static const uint32 kSpellsLearnTitleStrRef = 10345, kSpellsMemorizeTitleStrRef = 17189;
static const uint32 kMageSpellsLabelStrRef = 11027, kPriestSpellsLabelStrRef = 11028;

// The name window: Back, Done and the text field.
static const uint32 kNameBack = 3, kNameField = 2;

// The thief skills window: Back, the text area, a line for each of the four skills
// with the button that adds a point and the one that takes it off, a button over
// the name that shows what the skill is, the label with the points still to give
// and the ones with the points of each skill and its name.
static const uint32 kSkillsBack = 25, kSkillsText = 19;
static const uint32 kSkillPlusFirst = 11;		// then minus, +2 a line
static const uint32 kSkillInfoFirst = 21;
static const uint32 kSkillPointsLabel = 0x10000005;
static const uint32 kSkillValueLabelFirst = 0x10000001;
static const uint32 kSkillNameLabelFirst = 0x10000006;
static const uint32 kSkillsPromptStrRef = 17248;
static const uint32 kSkillsLabelStrRef = 8442;

// The proficiencies window: Back, the text area, a line for each of the eight
// weapons with the button that adds a star, the one that takes it off, five stars
// and a button over the name that shows what the weapon is; the label with the
// points left and the ones with the names.
static const uint32 kProficienciesBack = 77, kProficienciesText = 68;
static const uint32 kProficiencyPlusFirst = 11;		// then minus, +2 a line
static const uint32 kProficiencyStarFirst = 27;		// five a line
static const uint32 kProficiencyInfoFirst = 69;
static const uint32 kProficiencyPointsLabel = 0x10000009;
static const uint32 kProficiencyNameLabelFirst = 0x10000001;
static const int kMaxStars = 5;
static const uint32 kProficienciesPromptStrRef = 9588;
static const uint32 kProficienciesLabelStrRef = 9466;


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


// The text of a strref with the class tokens filled in: a mage's name reads
// "<MAGESCHOOL>" (a specialist's school; the generalist here, which is the only
// mage the creation makes yet) and a fighter's "<FIGHTERTYPE>" (the plain
// fighter, as GemRB's TLK importer has it).
static std::string
_ClassText(uint32 strRef)
{
	std::string text = IDTable::GetDialog(strRef);
	const std::pair<const char*, uint32> tokens[] = {
		{ "<MAGESCHOOL>", kGeneralistStrRef }, { "<FIGHTERTYPE>", kFighterTypeStrRef }
	};
	for (const auto& token : tokens) {
		const size_t at = text.find(token.first);
		if (at != std::string::npos)
			text.replace(at, strlen(token.first), IDTable::GetDialog(token.second));
	}
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
	fPointsLeft(0),
	fStoredPoints(0),
	fStoredExtra(0),
	fSkillPage(0),
	fProficiencyPoints(0),
	fSkillPoints(0),
	fEnemy(-1),
	fMemorizing(false),
	fSpellPoints(0),
	fEnemyRow(0),
	fOpenWindow(-1)
{
	std::fill(fStoredAbilities, fStoredAbilities + kNumAbilities, 0);
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


Label*
CharGenScreen::_Label(uint16 windowID, uint32 controlID) const
{
	Window* window = _Window(windowID);
	return window != NULL ? dynamic_cast<Label*>(window->GetControlByID(controlID)) : NULL;
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
	static const char* kNames[] = { "gender", "race", "class", "alignment", "abilities",
		"skills", "name", "accept" };
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
		button->SetEnabled(_StepOfButton(kStageButtons[i].control) == (int)fStep);
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
			if (fStep >= STEP_SKILLS) {
				for (int i = 0; i < kNumAbilities; i++) {
					text->AddText(_SummaryLine(kAbilityCapStrRefs[i],
						_AbilityText(i)).c_str());
				}
			}
			if (fStep == STEP_ACCEPT && !_Builder().Name().empty())
				text->AddText(_Builder().Name().c_str());
			if (fStep == STEP_ACCEPT) {
				// The spells the character knows, the mage's and the priest's.
				for (bool divine : { false, true }) {
					std::string names;
					for (const std::string& spell : _Builder().KnownSpells(divine)) {
						if (SPLResource* spl = gResManager->GetSPL(res_ref(spell.c_str()))) {
							names += "\n" + IDTable::GetDialog(spl->DisplayNameRef());
							gResManager->ReleaseResource(spl);
						}
					}
					if (!names.empty()) {
						text->AddText((IDTable::GetDialog(divine ? kPriestSpellsLabelStrRef
							: kMageSpellsLabelStrRef) + names).c_str());
					}
				}
			}
			if (fStep == STEP_ACCEPT && fEnemy >= 0 && _Builder().HasRacialEnemy()) {
				const CharGenHatedRace* races = CharGenData::HatedRaces(count);
				text->AddText(_SummaryLine(kEnemyLabelStrRef,
					IDTable::GetDialog(races[fEnemy].nameRef)).c_str());
			}
			if (fStep == STEP_ACCEPT && _Builder().ThiefSkillPoints() > 0) {
				text->AddText(IDTable::GetDialog(kSkillsLabelStrRef).c_str());
				const CharGenSkill* skills = CharGenData::Skills(count);
				const CharacterBuilder& builder = _Builder();
				for (int i = 0; i < CharacterBuilder::kNumThiefSkills; i++) {
					text->AddText(_SummaryLine(skills[i].capRef, std::to_string(
						std::max(builder.ThiefSkill(i) + builder.ThiefSkillBonus(i), 0))).c_str());
				}
			}
			if (fStep == STEP_ACCEPT && _Builder().ProficienciesSpent() > 0) {
				text->AddText(IDTable::GetDialog(kProficienciesLabelStrRef).c_str());
				const CharGenProficiency* profs = CharGenData::Proficiencies(count);
				for (size_t i = 0; i < count; i++) {
					const int stars = _Builder().Proficiency((int)i);
					if (stars > 0) {
						text->AddText((IDTable::GetDialog(profs[i].nameRef) + " "
							+ std::string((size_t)stars, '+')).c_str());
					}
				}
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
		case STEP_ABILITIES:
			_ShowAbilities();
			break;
		case STEP_SKILLS:
			_ShowSkills();
			break;
		case STEP_NAME:
			_ShowName();
			break;
		case STEP_ACCEPT:
			_Finish();
			break;
	}
}


void
CharGenScreen::_CloseChoice()
{
	GUI::Get()->SetTextFocus(NULL);
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


// The buttons laid over the names of a list (to show what each is) have a
// stone slab for a frame that would hide the name; only their click counts.
void
CharGenScreen::_MakeHotSpots(uint16 windowID, uint32 first, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		if (Button* button = _Button(windowID, first + (uint32)i))
			button->SetFrameless(true);
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


// The abilities window: rolls the scores (Reroll does it again), the arrows move
// a point from a score to another, Store keeps the roll and Recall brings it back.
void
CharGenScreen::_ShowAbilities()
{
	_CloseChoice();
	GUI::Get()->ShowAuxWindow(CHUName(), kAbilitiesWindow);
	fOpenWindow = kAbilitiesWindow;

	_ChoiceWindowButtons(this, _Button(kAbilitiesWindow, kDoneControl),
		_Button(kAbilitiesWindow, kAbilitiesBack));
	if (Button* done = _Button(kAbilitiesWindow, kDoneControl))
		done->SetEnabled(true);
	_SetText(kAbilitiesWindow, kAbilitiesReroll, kRerollStrRef);
	_SetText(kAbilitiesWindow, kAbilitiesStore, kStoreStrRef);
	_SetText(kAbilitiesWindow, kAbilitiesRecall, kRecallStrRef);
	for (int i = 0; i < kNumAbilities; i++) {
		if (Label* name = _Label(kAbilitiesWindow, kAbilitiesNameLabelFirst + (uint32)i))
			name->SetText(IDTable::GetDialog(kAbilityNameStrRefs[i]));
	}
	_SetDescription(kAbilitiesWindow, kAbilitiesText, kAbilitiesPromptStrRef);
	_MakeHotSpots(kAbilitiesWindow, kAbilitiesSelectFirst, kNumAbilities);

	// The first roll is kept, so Recall has something to bring back.
	_RollAbilities();
	_StoreAbilities();
}


void
CharGenScreen::_RollAbilities()
{
	_Builder().RollAbilities();
	fPointsLeft = 0;
	_ShowAbilityValues();
}


// The score as it is written: 18/xx for a warrior's exceptional strength.
std::string
CharGenScreen::_AbilityText(int ability) const
{
	const CharacterBuilder& builder = _Builder();
	std::string text = std::to_string(builder.Ability(ability));
	if (ability == 0 && builder.Ability(0) == 18 && builder.HasExceptionalStrength()) {
		char extra[8];
		snprintf(extra, sizeof(extra), "/%02d", builder.StrengthExtra());
		text += extra;
	}
	return text;
}


void
CharGenScreen::_ShowAbilityValues()
{
	if (Label* points = _Label(kAbilitiesWindow, kAbilitiesPointsLabel))
		points->SetText(std::to_string(fPointsLeft));
	for (int i = 0; i < kNumAbilities; i++) {
		if (Label* value = _Label(kAbilitiesWindow, kAbilitiesValueLabelFirst + (uint32)i))
			value->SetText(_AbilityText(i));
	}
}


// The text of the ability picked, with the range its race and class allow.
void
CharGenScreen::_DescribeAbility(int ability)
{
	const CharacterBuilder& builder = _Builder();
	std::string text = IDTable::GetDialog(kAbilityDescStrRefs[ability]);
	const std::pair<const char*, int> tokens[] = {
		{ "<MINIMUM>", builder.AbilityMin(ability) },
		{ "<MAXIMUM>", builder.AbilityMax(ability) }
	};
	for (const auto& token : tokens) {
		const std::string value = std::to_string(token.second);
		for (size_t at = text.find(token.first); at != std::string::npos;
				at = text.find(token.first, at + value.size()))
			text.replace(at, strlen(token.first), value);
	}
	if (TextArea* area = _TextArea(kAbilitiesWindow, kAbilitiesText)) {
		area->ClearText();
		area->AddText(text.c_str());
		area->ScrollTo(0, 0);
	}
}


// One point off (`step` -1, which frees it) or on (+1, which spends a free one)
// a score, within what the race and class allow.
void
CharGenScreen::_MovePoint(int ability, int step)
{
	CharacterBuilder& builder = _Builder();
	_DescribeAbility(ability);
	const int value = builder.Ability(ability);
	if (step < 0 ? value <= builder.AbilityMin(ability)
			: fPointsLeft == 0 || value >= builder.AbilityMax(ability))
		return;
	builder.SetAbility(ability, value + step);
	fPointsLeft -= step;
	_ShowAbilityValues();
}


void
CharGenScreen::_StoreAbilities()
{
	const CharacterBuilder& builder = _Builder();
	for (int i = 0; i < kNumAbilities; i++)
		fStoredAbilities[i] = builder.Ability(i);
	fStoredPoints = fPointsLeft;
	fStoredExtra = builder.StrengthExtra();
}


void
CharGenScreen::_RecallAbilities()
{
	CharacterBuilder& builder = _Builder();
	for (int i = 0; i < kNumAbilities; i++)
		builder.SetAbility(i, fStoredAbilities[i]);
	builder.SetStrengthExtra(fStoredExtra);
	fPointsLeft = fStoredPoints;
	_ShowAbilityValues();
}


// The skills stage: the windows the class has, one after the other.
void
CharGenScreen::_ShowSkills()
{
	// A cleric or a druid knows the priest spells of the first level from the start.
	_Builder().ClearSpells();
	_Builder().LearnDivineSpells();

	fSkillPages.clear();
	if (_Builder().HasRacialEnemy())
		fSkillPages.push_back(PAGE_RACIAL_ENEMY);
	if (_Builder().MageSpellsToLearn() > 0)
		fSkillPages.push_back(PAGE_MAGE_SPELLS);
	if (_Builder().ThiefSkillPoints() > 0)
		fSkillPages.push_back(PAGE_THIEF_SKILLS);
	fSkillPages.push_back(PAGE_PROFICIENCIES);
	fSkillPage = 0;
	_ShowSkillPage();
}


void
CharGenScreen::_ShowSkillPage()
{
	switch (fSkillPages[fSkillPage]) {
		case PAGE_RACIAL_ENEMY:
			_ShowRacialEnemy();
			break;
		case PAGE_MAGE_SPELLS:
			_ShowMageSpells();
			break;
		case PAGE_THIEF_SKILLS:
			_ShowThiefSkills();
			break;
		case PAGE_PROFICIENCIES:
			_ShowProficiencies();
			break;
	}
}


// Done: the next window of the stage, or the end of it.
void
CharGenScreen::_NextSkillPage()
{
	if (fSkillPage + 1 < fSkillPages.size()) {
		fSkillPage++;
		_ShowSkillPage();
		return;
	}
	fStep = STEP_NAME;
	_CloseChoice();
	_RefreshOverview();
}


// Back: the window before, or the overview from the first.
void
CharGenScreen::_PreviousSkillPage()
{
	if (fSkillPage > 0) {
		fSkillPage--;
		_ShowSkillPage();
		return;
	}
	_CloseChoice();
	_RefreshOverview();
}


// The racial enemy window: a list of the creatures a ranger fights best, six
// of them in view, scrolled with the bar; one is chosen and Done takes it.
void
CharGenScreen::_ShowRacialEnemy()
{
	_CloseChoice();
	GUI::Get()->ShowAuxWindow(CHUName(), kRacialEnemyWindow);
	fOpenWindow = kRacialEnemyWindow;

	_ChoiceWindowButtons(this, _Button(kRacialEnemyWindow, kEnemyDone),
		_Button(kRacialEnemyWindow, kEnemyBack));
	_SetDescription(kRacialEnemyWindow, kEnemyText, kEnemyPromptStrRef);

	fEnemy = -1;
	fEnemyRow = 0;
	size_t count = 0;
	CharGenData::HatedRaces(count);
	if (Scrollbar* scrollbar = dynamic_cast<Scrollbar*>(
			_Window(kRacialEnemyWindow)->GetControlByID(kEnemyScroll))) {
		scrollbar->SetRowCallback([this](int32 row) {
			fEnemyRow = row;
			_ShowRacialEnemyList();
		});
		scrollbar->SetScrollInfo(0, (int32)count - kEnemyRows);
	}
	_ShowRacialEnemyList();
}


// The six lines of the list from the row on, the chosen one latched.
void
CharGenScreen::_ShowRacialEnemyList()
{
	size_t count = 0;
	const CharGenHatedRace* races = CharGenData::HatedRaces(count);
	for (int i = 0; i < kEnemyRows; i++) {
		Button* button = _Button(kRacialEnemyWindow, kEnemyFirst + (uint32)i);
		if (button == NULL)
			continue;
		const int entry = fEnemyRow + i;
		const bool exists = entry >= 0 && (size_t)entry < count;
		button->SetText(exists ? IDTable::GetDialog(races[entry].nameRef) : "");
		button->SetEnabled(exists);
		button->SetLatched(exists && entry == fEnemy);
	}
	if (Button* done = _Button(kRacialEnemyWindow, kEnemyDone))
		done->SetEnabled(fEnemy >= 0);
}


// One of the six lines chosen: its text, and Done can go on.
void
CharGenScreen::_ChooseRacialEnemy(int row)
{
	size_t count = 0;
	const CharGenHatedRace* races = CharGenData::HatedRaces(count);
	const int entry = fEnemyRow + row;
	if (entry < 0 || (size_t)entry >= count)
		return;
	fEnemy = entry;
	_SetDescription(kRacialEnemyWindow, kEnemyText, races[entry].helpRef);
	_ShowRacialEnemyList();
}


// The spells window of a mage: the level-1 wizard spells its alignment allows,
// the ones to learn picked among them, then the one to memorize among those.
void
CharGenScreen::_ShowMageSpells()
{
	_CloseChoice();
	GUI::Get()->ShowAuxWindow(CHUName(), kSpellsWindow);
	fOpenWindow = kSpellsWindow;

	CharacterBuilder& builder = _Builder();
	_ChoiceWindowButtons(this, _Button(kSpellsWindow, kDoneControl),
		_Button(kSpellsWindow, kSpellsBack));
	builder.ClearSpells();
	builder.LearnDivineSpells();

	fSpellChoices.clear();
	for (const CharacterBuilder::spell_choice& choice : builder.MageSpellChoices())
		fSpellChoices.push_back({ choice.resref, choice.learnable, false });
	fMemorizing = false;
	fSpellPoints = builder.MageSpellsToLearn();
	_ShowSpellList();
}


// The buttons, from the choices (in the memorizing phase only the ones learned),
// with the picked ones latched and the points left.
void
CharGenScreen::_ShowSpellList()
{
	if (Label* title = _Label(kSpellsWindow, kSpellsTitleLabel)) {
		// Learning: "spells of level <SPELLLEVEL>", the first level.
		std::string text = IDTable::GetDialog(fMemorizing ? kSpellsMemorizeTitleStrRef
			: kSpellsLearnTitleStrRef);
		const size_t at = text.find("<SPELLLEVEL>");
		if (at != std::string::npos)
			text.replace(at, strlen("<SPELLLEVEL>"), "1");
		title->SetText(text);
	}
	if (Label* points = _Label(kSpellsWindow, kSpellsPointsLabel))
		points->SetText(std::to_string(fSpellPoints));

	// The learned spells, when memorizing: which of them are shown is stored as the
	// slot's spell.
	std::vector<size_t> shown;
	for (size_t i = 0; i < fSpellChoices.size(); i++) {
		if (!fMemorizing || _Builder().IsSpellKnown(fSpellChoices[i].resref))
			shown.push_back(i);
	}
	for (int slot = 0; slot < kSpellButtons; slot++) {
		Button* button = _Button(kSpellsWindow, kSpellsFirst + (uint32)slot);
		if (button == NULL)
			continue;
		if ((size_t)slot >= shown.size()) {
			button->SetIcon(NULL);
			button->SetEnabled(false);
			button->SetFrameless(true);
			button->SetLatched(false);
			continue;
		}
		const SpellChoice& choice = fSpellChoices[shown[(size_t)slot]];
		button->SetFrameless(false);
		button->SetIcon(ScreenSupport::MakeSpellIcon(res_ref(choice.resref.c_str())));
		button->SetEnabled(choice.learnable);
		button->SetLatched(choice.picked);
	}

	std::string text = IDTable::GetDialog(fMemorizing ? kSpellsMemorizeStrRef
		: kSpellsLearnStrRef);
	const size_t at = text.find("<number>");
	if (at != std::string::npos)
		text.replace(at, strlen("<number>"), std::to_string(fSpellPoints));
	if (TextArea* area = _TextArea(kSpellsWindow, kSpellsText)) {
		area->ClearText();
		area->AddText(text.c_str());
		area->ScrollTo(0, 0);
	}
	if (Button* done = _Button(kSpellsWindow, kDoneControl))
		done->SetEnabled(fSpellPoints == 0);
}


// A spell clicked: what it is, and it is picked (or not, if the points are gone).
void
CharGenScreen::_PickSpell(int slot)
{
	std::vector<size_t> shown;
	for (size_t i = 0; i < fSpellChoices.size(); i++) {
		if (!fMemorizing || _Builder().IsSpellKnown(fSpellChoices[i].resref))
			shown.push_back(i);
	}
	if (slot < 0 || (size_t)slot >= shown.size())
		return;
	SpellChoice& choice = fSpellChoices[shown[(size_t)slot]];
	if (SPLResource* spell = gResManager->GetSPL(res_ref(choice.resref.c_str()))) {
		if (TextArea* area = _TextArea(kSpellsWindow, kSpellsText)) {
			area->ClearText();
			area->AddText(IDTable::GetDialog(spell->DisplayDescriptionRef()).c_str());
			area->ScrollTo(0, 0);
		}
		gResManager->ReleaseResource(spell);
	}
	if (!choice.learnable)
		return;
	if (choice.picked) {
		choice.picked = false;
		fSpellPoints++;
	} else if (fSpellPoints > 0) {
		choice.picked = true;
		fSpellPoints--;
	}
	// The description stays; the buttons and the points are drawn again.
	if (Label* points = _Label(kSpellsWindow, kSpellsPointsLabel))
		points->SetText(std::to_string(fSpellPoints));
	for (int i = 0; i < kSpellButtons && (size_t)i < shown.size(); i++) {
		if (Button* button = _Button(kSpellsWindow, kSpellsFirst + (uint32)i))
			button->SetLatched(fSpellChoices[shown[(size_t)i]].picked);
	}
	if (Button* done = _Button(kSpellsWindow, kDoneControl))
		done->SetEnabled(fSpellPoints == 0);
}


// Done: the picked spells are learned; then (for the mage) the one to memorize is
// picked among them, and after that the window is done.
void
CharGenScreen::_FinishSpellPhase()
{
	CharacterBuilder& builder = _Builder();
	if (!fMemorizing) {
		for (SpellChoice& choice : fSpellChoices) {
			if (choice.picked)
				builder.LearnSpell(choice.resref, false);
			choice.picked = false;
		}
		fMemorizing = true;
		fSpellPoints = builder.MageSpellsToMemorize();
		if (fSpellPoints > 0) {
			_ShowSpellList();
			return;
		}
	} else {
		for (const SpellChoice& choice : fSpellChoices) {
			if (choice.picked)
				builder.LearnSpell(choice.resref, true);
		}
	}
	_NextSkillPage();
}


// The thief skills window: the points of the class to give to the four skills,
// which show with what the race and the Dexterity add; Done once none is left.
void
CharGenScreen::_ShowThiefSkills()
{
	_CloseChoice();
	GUI::Get()->ShowAuxWindow(CHUName(), kSkillsWindow);
	fOpenWindow = kSkillsWindow;

	_ChoiceWindowButtons(this, _Button(kSkillsWindow, kDoneControl),
		_Button(kSkillsWindow, kSkillsBack));
	_SetDescription(kSkillsWindow, kSkillsText, kSkillsPromptStrRef);
	_MakeHotSpots(kSkillsWindow, kSkillInfoFirst, CharacterBuilder::kNumThiefSkills);

	CharacterBuilder& builder = _Builder();
	for (int i = 0; i < CharacterBuilder::kNumThiefSkills; i++)
		builder.SetThiefSkill(i, 0);
	fSkillPoints = builder.ThiefSkillPoints();

	size_t count = 0;
	const CharGenSkill* skills = CharGenData::Skills(count);
	for (size_t i = 0; i < count; i++) {
		if (Label* name = _Label(kSkillsWindow, kSkillNameLabelFirst + (uint32)i))
			name->SetText(IDTable::GetDialog(skills[i].capRef));
	}
	_ShowThiefSkillValues();
}


void
CharGenScreen::_ShowThiefSkillValues()
{
	const CharacterBuilder& builder = _Builder();
	if (Label* points = _Label(kSkillsWindow, kSkillPointsLabel))
		points->SetText(std::to_string(fSkillPoints));
	for (int i = 0; i < CharacterBuilder::kNumThiefSkills; i++) {
		// What the character has: the points and the race's and the Dexterity's share.
		if (Label* value = _Label(kSkillsWindow, kSkillValueLabelFirst + (uint32)i)) {
			value->SetText(std::to_string(
				std::max(builder.ThiefSkill(i) + builder.ThiefSkillBonus(i), 0)));
		}
	}
	if (Button* done = _Button(kSkillsWindow, kDoneControl))
		done->SetEnabled(fSkillPoints == 0);
}


void
CharGenScreen::_DescribeThiefSkill(int skill)
{
	size_t count = 0;
	const CharGenSkill* skills = CharGenData::Skills(count);
	_SetDescription(kSkillsWindow, kSkillsText, skills[skill].descRef);
}


// One point more (`step` +1, from the points left) or less (-1) for a skill.
void
CharGenScreen::_MoveThiefSkill(int skill, int step)
{
	CharacterBuilder& builder = _Builder();
	_DescribeThiefSkill(skill);
	const int points = builder.ThiefSkill(skill);
	if (step > 0 ? fSkillPoints == 0 || points >= 250 : points == 0)
		return;
	builder.SetThiefSkill(skill, points + step);
	fSkillPoints -= step;
	_ShowThiefSkillValues();
}


// The proficiencies window: the class's points to give to the weapons it may
// use, up to the stars it can have at the start; Done once none is left.
void
CharGenScreen::_ShowProficiencies()
{
	_CloseChoice();
	GUI::Get()->ShowAuxWindow(CHUName(), kProficienciesWindow);
	fOpenWindow = kProficienciesWindow;

	_ChoiceWindowButtons(this, _Button(kProficienciesWindow, kDoneControl),
		_Button(kProficienciesWindow, kProficienciesBack));
	_SetDescription(kProficienciesWindow, kProficienciesText, kProficienciesPromptStrRef);
	_MakeHotSpots(kProficienciesWindow, kProficiencyInfoFirst, CharacterBuilder::kNumProficiencies);

	CharacterBuilder& builder = _Builder();
	for (int i = 0; i < CharacterBuilder::kNumProficiencies; i++)
		builder.SetProficiency(i, 0);
	fProficiencyPoints = builder.ProficiencyPoints();

	size_t count = 0;
	const CharGenProficiency* profs = CharGenData::Proficiencies(count);
	for (size_t i = 0; i < count; i++) {
		if (Label* name = _Label(kProficienciesWindow, kProficiencyNameLabelFirst + (uint32)i))
			name->SetText(IDTable::GetDialog(profs[i].nameRef));
	}
	_ShowProficiencyValues();
}


// The points left, the stars of each weapon, and which weapons can be changed.
void
CharGenScreen::_ShowProficiencyValues()
{
	const CharacterBuilder& builder = _Builder();
	if (Label* points = _Label(kProficienciesWindow, kProficiencyPointsLabel))
		points->SetText(std::to_string(fProficiencyPoints));
	for (int i = 0; i < CharacterBuilder::kNumProficiencies; i++) {
		// A weapon the class can't use has no buttons.
		const bool usable = builder.ProficiencyLimit(i) > 0;
		for (uint32 button = 0; button < 2; button++) {
			if (Button* arrow = _Button(kProficienciesWindow,
					kProficiencyPlusFirst + (uint32)i * 2 + button)) {
				arrow->SetEnabled(usable);
				arrow->SetFrameless(!usable);
			}
		}
		for (int star = 0; star < kMaxStars; star++) {
			if (Button* mark = _Button(kProficienciesWindow,
					kProficiencyStarFirst + (uint32)(i * kMaxStars + star)))
				mark->SetFrameless(star >= builder.Proficiency(i));
		}
	}
	if (Button* done = _Button(kProficienciesWindow, kDoneControl))
		done->SetEnabled(fProficiencyPoints == 0);
}


void
CharGenScreen::_DescribeProficiency(int proficiency)
{
	size_t count = 0;
	const CharGenProficiency* profs = CharGenData::Proficiencies(count);
	_SetDescription(kProficienciesWindow, kProficienciesText, profs[proficiency].descRef);
}


// One star more (`step` +1, from the points left) or less (-1) for a weapon.
void
CharGenScreen::_MoveProficiency(int proficiency, int step)
{
	CharacterBuilder& builder = _Builder();
	_DescribeProficiency(proficiency);
	if (step > 0 && fProficiencyPoints == 0)
		return;
	if (!builder.SetProficiency(proficiency, builder.Proficiency(proficiency) + step))
		return;
	fProficiencyPoints -= step;
	_ShowProficiencyValues();
}


// The name window: a text field, Done once something is typed (Return does it).
void
CharGenScreen::_ShowName()
{
	_CloseChoice();
	GUI::Get()->ShowAuxWindow(CHUName(), kNameWindow);
	fOpenWindow = kNameWindow;

	_ChoiceWindowButtons(this, _Button(kNameWindow, kDoneControl), _Button(kNameWindow, kNameBack));
	TextEdit* field = dynamic_cast<TextEdit*>(_Window(kNameWindow)->GetControlByID(kNameField));
	if (field == NULL)
		return;
	auto changed = [this, field] () {
		if (Button* done = _Button(kNameWindow, kDoneControl))
			done->SetEnabled(!field->Text().empty());
	};
	field->SetChangeCallback(changed);
	field->SetEnterCallback([this, field] () {
		if (!field->Text().empty())
			ControlInvoked(kNameWindow, kDoneControl);
	});
	field->SetText(_Builder().Name() == "Player" ? "" : _Builder().Name());
	GUI::Get()->SetTextFocus(field);
}


// Back from the overview: to the previous stage, its choice forgotten (and
// everything after it).
void
CharGenScreen::_StepBack()
{
	if (fStep == STEP_GENDER)
		return;
	fStep = (Step)((int)fStep - 1);

	// The stages from the abilities on take back only their own choice; the ones
	// before start over from the builder's beginning.
	CharacterBuilder& builder = _Builder();
	switch (fStep) {
		case STEP_NAME:
			builder.SetName("");
			_RefreshOverview();
			return;
		case STEP_SKILLS:
			builder.ClearSkills();
			_RefreshOverview();
			return;
		case STEP_ABILITIES:
			builder.ClearAbilities();
			_RefreshOverview();
			return;
		default:
			break;
	}
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
	if (fAlignment >= 0)
		builder.SetAlignment(CharGenData::Alignments(count)[fAlignment].name);
	_RefreshOverview();
}


// Accept: what the stages after the skills would have asked is the default for
// now (no name); the colors are the portrait's, then the finish of the creation.
void
CharGenScreen::_Finish()
{
	CharacterBuilder& builder = _Builder();
	if (fPortrait >= 0) {
		size_t count = 0;
		const CharGenPortrait& portrait = CharGenData::Portraits(count)[fPortrait];
		builder.SetColor("hair", portrait.hair);
		builder.SetColor("skin", portrait.skin);
		builder.SetColor("major", portrait.major);
		builder.SetColor("minor", portrait.minor);
	}
	// The metal, leather and armor colors of a new character.
	builder.SetColor("metal", 0x1b);
	builder.SetColor("leather", 0x16);
	builder.SetColor("armor", 0x17);
	builder.ApplyStartingKit();

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
			fStep = STEP_ABILITIES;
			_CloseChoice();
			_RefreshOverview();
		} else if (controlID == kAlignmentBack) {
			fAlignment = -1;
			_CloseChoice();
			_RefreshOverview();
		}
		return true;
	}

	if (windowID == kNameWindow) {
		if (controlID == kDoneControl) {
			TextEdit* field = dynamic_cast<TextEdit*>(_Window(kNameWindow)->GetControlByID(kNameField));
			if (field != NULL && !field->Text().empty()) {
				builder.SetName(field->Text());
				fStep = STEP_ACCEPT;
				_CloseChoice();
				_RefreshOverview();
			}
		} else if (controlID == kNameBack) {
			_CloseChoice();
			_RefreshOverview();
		}
		return true;
	}

	if (windowID == kSpellsWindow || windowID == kRacialEnemyWindow
			|| windowID == kSkillsWindow || windowID == kProficienciesWindow)
		return _HandleSkillWindow(windowID, controlID);

	if (windowID == kAbilitiesWindow) {
		if (controlID >= kAbilitiesSelectFirst
				&& controlID < kAbilitiesSelectFirst + kNumAbilities) {
			_DescribeAbility((int)(controlID - kAbilitiesSelectFirst));
		} else if (controlID >= kAbilitiesArrowFirst
				&& controlID < kAbilitiesArrowFirst + 2 * kNumAbilities) {
			// Two arrows a score: the first (the left one) adds a point, the
			// second takes one off, as GemRB's GUICG4 has them.
			const uint32 arrow = controlID - kAbilitiesArrowFirst;
			_MovePoint((int)(arrow / 2), arrow % 2 == 0 ? 1 : -1);
		} else if (controlID == kAbilitiesReroll) {
			_RollAbilities();
		} else if (controlID == kAbilitiesStore) {
			_StoreAbilities();
		} else if (controlID == kAbilitiesRecall) {
			_RecallAbilities();
		} else if (controlID == kDoneControl) {
			fStep = STEP_SKILLS;
			_CloseChoice();
			_RefreshOverview();
		} else if (controlID == kAbilitiesBack) {
			_CloseChoice();
			_RefreshOverview();
		}
		return true;
	}
	return false;
}


// Events of the windows of the skills stage.
bool
CharGenScreen::_HandleSkillWindow(uint16 windowID, uint32 controlID)
{
	if (windowID == kSpellsWindow) {
		if (controlID >= kSpellsFirst && controlID < kSpellsFirst + (uint32)kSpellButtons) {
			_PickSpell((int)(controlID - kSpellsFirst));
		} else if (controlID == kDoneControl) {
			_FinishSpellPhase();
		} else if (controlID == kSpellsBack) {
			_Builder().ClearSpells();
			_Builder().LearnDivineSpells();
			_PreviousSkillPage();
		}
	}
	if (windowID == kRacialEnemyWindow) {
		if (controlID >= kEnemyFirst && controlID < kEnemyFirst + (uint32)kEnemyRows) {
			_ChooseRacialEnemy((int)(controlID - kEnemyFirst));
		} else if (controlID == kEnemyDone && fEnemy >= 0) {
			size_t count = 0;
			_Builder().SetRacialEnemy(CharGenData::HatedRaces(count)[fEnemy].id);
			_NextSkillPage();
		} else if (controlID == kEnemyBack) {
			_PreviousSkillPage();
		}
	}
	if (windowID == kSkillsWindow) {
		const int lines = CharacterBuilder::kNumThiefSkills;
		if (controlID >= kSkillPlusFirst && controlID < kSkillPlusFirst + (uint32)(2 * lines)) {
			const uint32 arrow = controlID - kSkillPlusFirst;
			_MoveThiefSkill((int)(arrow / 2), arrow % 2 == 0 ? 1 : -1);
		} else if (controlID >= kSkillInfoFirst && controlID < kSkillInfoFirst + (uint32)lines) {
			_DescribeThiefSkill((int)(controlID - kSkillInfoFirst));
		} else if (controlID == kDoneControl) {
			_NextSkillPage();
		} else if (controlID == kSkillsBack) {
			_PreviousSkillPage();
		}
	}
	if (windowID == kProficienciesWindow) {
		const int lines = CharacterBuilder::kNumProficiencies;
		if (controlID >= kProficiencyPlusFirst
				&& controlID < kProficiencyPlusFirst + (uint32)(2 * lines)) {
			const uint32 arrow = controlID - kProficiencyPlusFirst;
			_MoveProficiency((int)(arrow / 2), arrow % 2 == 0 ? 1 : -1);
		} else if (controlID >= kProficiencyInfoFirst
				&& controlID < kProficiencyInfoFirst + (uint32)lines) {
			_DescribeProficiency((int)(controlID - kProficiencyInfoFirst));
		} else if (controlID == kDoneControl) {
			_NextSkillPage();
		} else if (controlID == kProficienciesBack) {
			_PreviousSkillPage();
		}
	}
	return true;
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
			if (_StepOfButton(controlID) == (int)fStep)
				_OpenStage(fStep);
			break;
		}
	}
	return true;
}
