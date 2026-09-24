#include "RecordScreen.h"

#include "Actor.h"
#include "Bitmap.h"
#include "BmpResource.h"
#include "Button.h"
#include "Core.h"
#include "CreResource.h"
#include "Game.h"
#include "Label.h"
#include "ResManager.h"
#include "TextArea.h"
#include "Window.h"

#include <ctype.h>

static const uint32 kRecNameLabelID = 268435470;
static const uint32 kRecACLabelID = 268435496;
static const uint32 kRecHPCurrentLabelID = 268435497;
static const uint32 kRecHPMaxLabelID = 268435498;
static const uint32 kRecClassLabelID = 268435504;
static const uint32 kRecRaceLabelID = 268435471;
static const uint32 kRecGenderLabelID = 268435473;
// Alignment (confirmed against GUIREC.py's own 0x10000010) - the CHU's
// own baked-in default text for this control (a real Label always shows
// one, see gui/Label.cpp) happened to be a leftover from whatever this
// control template was cloned from, not alignment; that stale default
// was never visible in a real game because the real engine always
// overwrites it here, same as this code now does.
static const uint32 kRecAlignmentLabelID = 268435472;
// GemRB's own hardcoded strrefs for this pair (real BG1/BG2 don't ship
// a 2DA for two values) - GENDER.IDS' MALE id (1) picks the first.
static const uint32 kRecMaleStrRef = 7198;
static const uint32 kRecFemaleStrRef = 7199;
static const uint32 kRecStatsAreaID = 45;
// The record screen's own buttons (window 2), confirmed against a real
// GUIREC.CHU dump of both games and GemRB's GUIREC.py (which sets each
// one's caption from the same strrefs - the engine's own English text
// renders in whatever language the loaded TLK actually is). Kit Info is
// BG2-only (dual-classing and reforming the party exist in both games);
// control id 2, also confirmed against GUIREC.py, isn't an 8th button at
// all but the large portrait (kRecPortraitButtonID below).
static const uint32 kRecDualClassButtonID = 0;
static const uint32 kRecDualClassStrRef = 7174;
static const uint32 kRecLevelUpButtonID = 37;
static const uint32 kRecLevelUpStrRef = 7175;
static const uint32 kRecInformationButtonID = 1;
static const uint32 kRecInformationStrRef = 11946;
static const uint32 kRecReformPartyButtonID = 51;
static const uint32 kRecReformPartyStrRef = 16559;
static const uint32 kRecCustomizeButtonID = 50;
static const uint32 kRecCustomizeStrRef = 10645;
static const uint32 kRecExportButtonID = 36;
static const uint32 kRecExportStrRef = 13956;
static const uint32 kRecKitInfoButtonID = 52;
static const uint32 kRecKitInfoStrRef = 61265;
// The large portrait button (confirmed as such, not a 7th action button,
// against GemRB's GUIREC.py: "Button = Window.GetControl(2);
// Button.SetPicture(GemRB.GetPlayerPortrait(pc,0), ...)"). BG2 falls back
// to the medium placeholder when a character has no portrait of its own,
// every other game to the large one - both real BMP resources, confirmed
// present in both installs.
static const uint32 kRecPortraitButtonID = 2;

RecordScreen::RecordScreen(Game& game)
	:
	PanelScreen(game, "GUIREC")
{
}


/* virtual */
uint32
RecordScreen::CommandBarButton() const
{
	return 4;
}


// The "General" tab of the Record screen (window 2): name, HP, AC, the six
// ability scores, class/race/alignment/gender, saving throws and
// resistances, the large portrait and the captions of the buttons.
void
RecordScreen::RefreshContent()
{
	Actor* actor = fGame.ShownActor();
	if (actor == NULL || actor->CRE() == NULL)
		return;

	Window* window = GetWindow(kContentWindow);
	if (window == NULL)
		return;

	Label* nameLabel = dynamic_cast<Label*>(window->GetControlByID(kRecNameLabelID));
	if (nameLabel != NULL)
		nameLabel->SetText(actor->LongName());

	Label* hpLabel = dynamic_cast<Label*>(window->GetControlByID(kRecHPCurrentLabelID));
	if (hpLabel != nullptr)
		hpLabel->SetText(std::to_string(actor->CRE()->CurrentHitPoints()));

	Label* hpMaxLabel = dynamic_cast<Label*>(window->GetControlByID(kRecHPMaxLabelID));
	if (hpMaxLabel != nullptr)
		hpMaxLabel->SetText(std::to_string(actor->CRE()->MaxHitPoints()));

	_UpdateAbilityScoreLabels(window, actor->CRE());

	Label* acLabel = dynamic_cast<Label*>(window->GetControlByID(kRecACLabelID));
	if (acLabel != NULL)
		acLabel->SetText(std::to_string(actor->CRE()->AC().effective));

	_UpdateClassRaceLevelLabels(window, actor);
	_UpdateSavesAndResistances(window, actor->CRE());
	_UpdateButtons(window);
	_UpdatePortrait(window, actor);
}


// The character's own large portrait (control 2 - see its own comment).
void
RecordScreen::_UpdatePortrait(Window* window, Actor* actor)
{
	Button* button = dynamic_cast<Button*>(window->GetControlByID(kRecPortraitButtonID));
	if (button == NULL)
		return;

	res_ref portraitRef = actor->CRE()->LargePortrait();
	if (portraitRef.CString()[0] == '\0') {
		portraitRef = Core::Get()->Game() == game::GAME_BALDURSGATE2
			? res_ref("NOPORTMD") : res_ref("NOPORTLG");
	}

	Bitmap* portrait = NULL;
	BMPResource* bmp = gResManager->GetBMP(portraitRef);
	if (bmp != NULL) {
		portrait = bmp->Image();
		gResManager->ReleaseResource(bmp);
	}
	// coverBackground: same reasoning as the paperdoll button's own -
	// the CHU-authored placeholder shouldn't show through/around it.
	button->SetIcon(portrait, true);
}


// The row of buttons under the portrait (Dual-Class/Level Up/Information/
// Reform Party/Customize/Export, plus Kit Info on BG2) - labels only, same
// as GUIREC.py's own SetText() calls; none of the windows they'd open
// (dual-classing, leveling up, ...) exist in this engine yet, so they stay
// unwired (a click is a silent no-op, same as any other unhandled control).
void
RecordScreen::_UpdateButtons(Window* window)
{
	auto setLabel = [window] (uint32 controlID, uint32 strRef) {
		if (Button* button = dynamic_cast<Button*>(window->GetControlByID(controlID)))
			button->SetText(IDTable::GetDialog(strRef));
	};
	setLabel(kRecDualClassButtonID, kRecDualClassStrRef);
	setLabel(kRecLevelUpButtonID, kRecLevelUpStrRef);
	setLabel(kRecInformationButtonID, kRecInformationStrRef);
	setLabel(kRecReformPartyButtonID, kRecReformPartyStrRef);
	setLabel(kRecCustomizeButtonID, kRecCustomizeStrRef);
	setLabel(kRecExportButtonID, kRecExportStrRef);
	if (Core::Get()->Game() == game::GAME_BALDURSGATE2)
		setLabel(kRecKitInfoButtonID, kRecKitInfoStrRef);
}


// The 6 ability-score value labels, row-by-row (top to bottom) against
// the static "Forza/Destrezza/Costituzione/Intelligenza/Saggezza/
// Carisma" name labels beside them, confirmed via a real GUIREC.CHU
// control dump (each value label sits ~10px below its matching name
// label, same 37px row spacing for both columns).
void
RecordScreen::_UpdateAbilityScoreLabels(Window* window, CREResource* cre)
{
	BaseAttributes attrs;
	cre->GetAttributes(attrs);

	static const uint32 kStatLabelIDs[] = {
		268435503, 268435465, 268435466, 268435467, 268435468, 268435469
	};
	const int8 statValues[] = {
		attrs.strength, attrs.dexterity, attrs.constitution,
		attrs.intelligence, attrs.wisdom, attrs.charisma
	};
	for (int i = 0; i < 6; i++) {
		Label* label = dynamic_cast<Label*>(window->GetControlByID(kStatLabelIDs[i]));
		if (label == NULL)
			continue;
		std::string text = std::to_string(statValues[i]);
		// Exceptional strength (18/xx) - only meaningful at STR 18.
		if (i == 0 && statValues[0] == 18 && attrs.strength_bonus > 0)
			text += "/" + std::to_string(attrs.strength_bonus);
		label->SetText(text);
	}
}


// Class/Race/Alignment/Gender block. Prefers the properly localized
// IDTable::*Name() lookups (see their own comment); an id outside their
// small hardcoded table (an exotic/modded creature) falls back to the
// raw, always-English IDS symbol rather than showing nothing.
void
RecordScreen::_UpdateClassRaceLevelLabels(Window* window, Actor* actor)
{
	Label* classLabel = dynamic_cast<Label*>(window->GetControlByID(kRecClassLabelID));
	if (classLabel != NULL) {
		std::string text = IDTable::ClassName(actor->CRE()->Class());
		if (text.empty())
			text = _TitleCaseIDSName(IDTable::ClassAt(actor->CRE()->Class()));
		classLabel->SetText(text);
	}

	Label* raceLabel = dynamic_cast<Label*>(window->GetControlByID(kRecRaceLabelID));
	if (raceLabel != NULL) {
		std::string text = IDTable::RaceName(actor->CRE()->Race());
		if (text.empty())
			text = _TitleCaseIDSName(IDTable::RaceAt(actor->CRE()->Race()));
		raceLabel->SetText(text);
	}

	Label* alignmentLabel = dynamic_cast<Label*>(window->GetControlByID(kRecAlignmentLabelID));
	if (alignmentLabel != NULL) {
		std::string text = IDTable::AlignmentName(actor->CRE()->Alignment());
		if (text.empty())
			text = _TitleCaseIDSName(IDTable::AlignmentAt(actor->CRE()->Alignment()));
		alignmentLabel->SetText(text);
	}

	Label* genderLabel = dynamic_cast<Label*>(window->GetControlByID(kRecGenderLabelID));
	if (genderLabel != NULL) {
		genderLabel->SetText(IDTable::GetDialog(actor->CRE()->Gender() == 1
			? kRecMaleStrRef : kRecFemaleStrRef));
	}
}


// Saving throws + damage resistances (id 45, a scrollable text_area
// with its own scrollbar at id 46, confirmed via a real GUIREC.CHU
// dump) - real BG2 lists these as plain scrollable text rather than
// individual labels, unlike the rest of this tab. Both structs
// (SaveVersus/Resistances) were already fully read by CREResource,
// just never displayed anywhere until now.
void
RecordScreen::_UpdateSavesAndResistances(Window* window, CREResource* cre)
{
	TextArea* statsArea = dynamic_cast<TextArea*>(window->GetControlByID(kRecStatsAreaID));
	if (statsArea == NULL)
		return;

	statsArea->ClearText();

	// name + level
	statsArea->AddText((IDTable::ClassAt(cre->Class())
		+ std::string(": Level ") + std::to_string(cre->Level())).c_str());

	// Experience
	statsArea->AddText((std::string("Experience: ") + std::to_string(cre->Experience())).c_str());

	// TODO: Next level

	SaveVersus saves = cre->Saves();
	const std::pair<const char*, uint8> saveLines[] = {
		{ "Morte", saves.death },
		{ "Bacchette", saves.wands },
		{ "Polimorfismo", saves.poly },
		{ "Soffio", saves.breath },
		{ "Incantesimi", saves.spell }
	};
	statsArea->AddText("Tiri Salvezza");
	for (const auto& line : saveLines)
		statsArea->AddText((std::string(line.first) + ": " + std::to_string(line.second)).c_str());

	Resistances res = cre->DamageResistances();
	const std::pair<const char*, uint8> resistanceLines[] = {
		{ "Contundente", res.crushing },
		{ "Perforante", res.piercing },
		{ "Tagliente", res.slashing },
		{ "Missili", res.missile },
		{ "Fuoco", res.fire },
		{ "Freddo", res.cold },
		{ "Elettricita", res.electricity },
		{ "Acido", res.acid },
		{ "Magia", res.magic },
		{ "Fuoco magico", res.magic_fire },
		{ "Freddo magico", res.magic_cold }
	};
	statsArea->AddText("Resistenze");
	for (const auto& line : resistanceLines) {
		statsArea->AddText(
			(std::string(line.first) + ": " + std::to_string(line.second) + "%").c_str());
	}

	// AddText() auto-scrolls to the newest line (fine for the
	// dialogue TextArea it was written for) - scroll back to the
	// top so Saving Throws, not the tail of Resistances, is what's
	// visible when the tab first opens.
	statsArea->ScrollTo(0, 0);
}


// "HALF_ELF" -> "Half Elf" - turns a raw *.IDS symbolic name (all caps,
// underscore-separated) into something readable, absent a 2DA to look up
// a real localized display string for it.
std::string
RecordScreen::_TitleCaseIDSName(const std::string& idsName)
{
	std::string result = idsName;
	bool startOfWord = true;
	for (char& c : result) {
		if (c == '_') {
			c = ' ';
			startOfWord = true;
		} else if (startOfWord) {
			c = toupper(c);
			startOfWord = false;
		} else {
			c = tolower(c);
		}
	}
	return result;
}
