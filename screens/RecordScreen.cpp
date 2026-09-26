#include "RecordScreen.h"

#include "Actor.h"
#include "Bitmap.h"
#include "BmpResource.h"
#include "Button.h"
#include "Core.h"
#include "CreResource.h"
#include "Game.h"
#include "GUI.h"
#include "GameTimer.h"
#include "Label.h"
#include "Party.h"
#include "ResManager.h"
#include "ScreenSupport.h"
#include "SPLResource.h"
#include "TextArea.h"
#include "Window.h"

#include <ctype.h>
#include <algorithm>
#include <cstring>

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

// The Information window (window 4, the same in both games; ids as GemRB's
// GUIREC.py has them): Done and Biography, and the labels that get the name, the
// class, the most powerful vanquished, the time in the party, the favourite spell
// and weapon, and the kills of the chapter and of the game (the share of the
// party's XP and kills, then the XP and the number).
static const uint32 kInfoDoneButtonID = 24;
static const uint32 kInfoBiographyButtonID = 26;
static const uint32 kInfoDoneStrRef = 11973;
static const uint32 kInfoBiographyStrRef = 18003;
static const uint32 kInfoNameLabelID = 0x10000000;
static const uint32 kInfoClassLabelID = 0x10000018;
static const uint32 kInfoBestKilledLabelID = 0x10000005;
static const uint32 kInfoTimeLabelID = 0x10000006;
static const uint32 kInfoSpellLabelID = 0x10000007;
static const uint32 kInfoWeaponLabelID = 0x10000008;
static const uint32 kInfoChapterXPShareLabelID = 0x1000000f;
static const uint32 kInfoChapterKillsShareLabelID = 0x10000010;
static const uint32 kInfoChapterXPLabelID = 0x10000011;
static const uint32 kInfoChapterKillsLabelID = 0x10000012;
static const uint32 kInfoTotalXPShareLabelID = 0x10000013;
static const uint32 kInfoTotalKillsShareLabelID = 0x10000014;
static const uint32 kInfoTotalXPLabelID = 0x10000015;
static const uint32 kInfoTotalKillsLabelID = 0x10000016;
// Time in the party: "<GAMEDAYS> days and <HOUR> hours" (only the hours when
// it is less than a day), each with a singular for one.
static const uint32 kInfoDaysStrRef = 10697;
static const uint32 kInfoDayStrRef = 10698;
static const uint32 kInfoAndStrRef = 10699;
static const uint32 kInfoHoursStrRef = 10700;
static const uint32 kInfoHourStrRef = 10701;
// A game day is 7200 game seconds, an hour 300.
static const uint32 kSecondsPerDay = 7200;
static const uint32 kSecondsPerHour = 300;
// The biography window: its text area and Done, and the CRE's string slot that
// holds the biography.
static const uint32 kBiographyTextID = 0;
static const uint32 kBiographyDoneButtonID = 2;
static const uint32 kBiographySoundSlot = 74;

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
		acLabel->SetText(std::to_string(actor->ArmorClass()));

	_UpdateClassRaceLevelLabels(window, actor);
	_UpdateSavesAndResistances(window, actor->CRE());
	_UpdateButtons(window, actor);
	_UpdatePortrait(window, actor);
}


// Level Up: the shown character gains the levels its experience allows and the
// screen shows the new numbers. (The original also opens windows to pick
// proficiencies, spells and thief skills; the points to spend aren't modeled.)
/* virtual */
void
RecordScreen::PanelControlInvoked(uint16 windowID, uint32 controlID)
{
	if (windowID == kInfoWindow) {
		if (controlID == kInfoDoneButtonID)
			_CloseInformation();
		else if (controlID == kInfoBiographyButtonID)
			_OpenBiography();
		return;
	}
	if (windowID == kBiographyWindow) {
		if (controlID == kBiographyDoneButtonID)
			_CloseBiography();
		return;
	}
	if (windowID != kContentWindow)
		return;

	if (controlID == kRecInformationButtonID) {
		_OpenInformation();
		return;
	}
	if (controlID != kRecLevelUpButtonID)
		return;

	Actor* actor = fGame.ShownActor();
	if (actor == NULL || actor->CRE() == NULL || !actor->LevelUp())
		return;

	RefreshContent();
}


// The sheet of another character: the window that is open follows.
/* virtual */
void
RecordScreen::OnShownCharacterChanged()
{
	if (GUI::Get()->IsAuxWindowShown(CHUName(), kInfoWindow))
		_RefreshInformation();
	if (GUI::Get()->IsAuxWindowShown(CHUName(), kBiographyWindow)) {
		_CloseBiography();
		_OpenBiography();
	}
}


// The windows the sheet opened go with it.
/* virtual */
void
RecordScreen::OnClose()
{
	_CloseBiography();
	_CloseInformation();
}


void
RecordScreen::_OpenInformation()
{
	GUI::Get()->ShowAuxWindow(CHUName(), kInfoWindow);
	if (Window* window = GetWindow(kInfoWindow)) {
		auto caption = [window] (uint32 controlID, uint32 strRef) {
			if (Button* button = dynamic_cast<Button*>(window->GetControlByID(controlID)))
				button->SetText(IDTable::GetDialog(strRef));
		};
		caption(kInfoDoneButtonID, kInfoDoneStrRef);
		caption(kInfoBiographyButtonID, kInfoBiographyStrRef);
	}
	_RefreshInformation();
}


void
RecordScreen::_CloseInformation()
{
	_CloseBiography();
	GUI::Get()->HideAuxWindow(CHUName(), kInfoWindow);
}


// The class as the sheet writes it.
std::string
RecordScreen::_ClassTitle(Actor* actor) const
{
	std::string text = IDTable::ClassName(actor->CRE()->Class());
	if (text.empty())
		text = _TitleCaseIDSName(IDTable::ClassAt(actor->CRE()->Class()));
	return text;
}


// A share of `part` in `total`, as a percentage.
static std::string
_Percentage(uint32 part, uint32 total)
{
	return std::to_string(total != 0 ? (uint64)part * 100 / total : 0) + "%";
}


// "<days> days and <hours> hours" of the game time the character has been in the
// party (only the hours under a day).
static std::string
_TimeInParty(uint32 joinTime)
{
	const uint32 now = GameTimer::GameTime();
	const uint32 seconds = now > joinTime ? now - joinTime : 0;
	const uint32 days = seconds / kSecondsPerDay;
	const uint32 hours = (seconds % kSecondsPerDay) / kSecondsPerHour;

	std::string text;
	if (days != 0) {
		text = IDTable::GetDialog(days == 1 ? kInfoDayStrRef : kInfoDaysStrRef)
			+ " " + IDTable::GetDialog(kInfoAndStrRef) + " ";
	}
	text += IDTable::GetDialog(hours == 1 ? kInfoHourStrRef : kInfoHoursStrRef);

	const std::pair<const char*, uint32> tokens[] = {
		{ "<GAMEDAYS>", days }, { "<HOUR>", hours }
	};
	for (const auto& token : tokens) {
		const std::string value = std::to_string(token.second);
		for (size_t at = text.find(token.first); at != std::string::npos;
				at = text.find(token.first, at + value.size()))
			text.replace(at, strlen(token.first), value);
	}
	return text;
}


// The statistics of the shown character: what it killed, the time in the party,
// the favourite spell and weapon, and its share of the party's kills.
void
RecordScreen::_RefreshInformation()
{
	Actor* actor = fGame.ShownActor();
	Window* window = GetWindow(kInfoWindow);
	if (actor == NULL || actor->CRE() == NULL || window == NULL)
		return;

	auto setLabel = [window] (uint32 controlID, const std::string& text) {
		if (Label* label = dynamic_cast<Label*>(window->GetControlByID(controlID)))
			label->SetText(text);
	};

	const PCStats& stats = actor->Stats();
	setLabel(kInfoNameLabelID, actor->LongName());
	setLabel(kInfoClassLabelID, _ClassTitle(actor));
	setLabel(kInfoBestKilledLabelID, stats.bestKilledName != PCStats::kNoName
		? IDTable::GetDialog(stats.bestKilledName) : "");
	setLabel(kInfoTimeLabelID, _TimeInParty(stats.joinTime));

	std::string spellName;
	const res_ref favouriteSpell = stats.FavouriteSpell();
	if (favouriteSpell.CString()[0] != '\0') {
		if (SPLResource* spell = gResManager->GetSPL(favouriteSpell)) {
			spellName = IDTable::GetDialog(spell->DisplayNameRef());
			gResManager->ReleaseResource(spell);
		}
	}
	setLabel(kInfoSpellLabelID, spellName);

	std::string weaponName;
	const res_ref favouriteWeapon = stats.FavouriteWeapon();
	if (favouriteWeapon.CString()[0] != '\0')
		weaponName = ScreenSupport::ItemDisplayName(favouriteWeapon);
	setLabel(kInfoWeaponLabelID, weaponName);

	uint32 partyChapterXP = 0, partyChapterKills = 0, partyTotalXP = 0, partyTotalKills = 0;
	if (::Party* party = fGame.Party()) {
		for (uint16 i = 0; i < party->CountActors(); i++) {
			const PCStats& member = party->ActorAt(i)->Stats();
			partyChapterXP += member.chapterXP;
			partyChapterKills += member.chapterKills;
			partyTotalXP += member.totalXP;
			partyTotalKills += member.totalKills;
		}
	}
	setLabel(kInfoChapterXPShareLabelID, _Percentage(stats.chapterXP, partyChapterXP));
	setLabel(kInfoChapterKillsShareLabelID, _Percentage(stats.chapterKills, partyChapterKills));
	setLabel(kInfoChapterXPLabelID, std::to_string(stats.chapterXP));
	setLabel(kInfoChapterKillsLabelID, std::to_string(stats.chapterKills));
	setLabel(kInfoTotalXPShareLabelID, _Percentage(stats.totalXP, partyTotalXP));
	setLabel(kInfoTotalKillsShareLabelID, _Percentage(stats.totalKills, partyTotalKills));
	setLabel(kInfoTotalXPLabelID, std::to_string(stats.totalXP));
	setLabel(kInfoTotalKillsLabelID, std::to_string(stats.totalKills));
}


// The biography of the shown character: the string its CRE holds in slot 74.
void
RecordScreen::_OpenBiography()
{
	Actor* actor = fGame.ShownActor();
	if (actor == NULL || actor->CRE() == NULL)
		return;

	GUI::Get()->ShowAuxWindow(CHUName(), kBiographyWindow);
	Window* window = GetWindow(kBiographyWindow);
	if (window == NULL)
		return;

	if (Button* done = dynamic_cast<Button*>(window->GetControlByID(kBiographyDoneButtonID)))
		done->SetText(IDTable::GetDialog(kInfoDoneStrRef));
	if (TextArea* text = dynamic_cast<TextArea*>(window->GetControlByID(kBiographyTextID))) {
		text->ClearText();
		const uint32 strRef = actor->CRE()->SoundSetStringRef(kBiographySoundSlot);
		if (strRef != 0xffffffff)
			text->AddText(IDTable::GetDialog(strRef).c_str());
		text->ScrollTo(0, 0);
	}
}


void
RecordScreen::_CloseBiography()
{
	GUI::Get()->HideAuxWindow(CHUName(), kBiographyWindow);
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
// Reform Party/Customize/Export, plus Kit Info on BG2): their captions, same
// as GUIREC.py's own SetText() calls. Only Level Up does anything (enabled
// while the character's experience allows a level, as in GemRB's GUIREC.py);
// the windows of the others (dual-classing, ...) don't exist in this engine
// yet, so a click on them is a silent no-op.
void
RecordScreen::_UpdateButtons(Window* window, Actor* actor)
{
	auto setLabel = [window] (uint32 controlID, uint32 strRef) {
		if (Button* button = dynamic_cast<Button*>(window->GetControlByID(controlID)))
			button->SetText(IDTable::GetDialog(strRef));
	};
	setLabel(kRecDualClassButtonID, kRecDualClassStrRef);
	setLabel(kRecLevelUpButtonID, kRecLevelUpStrRef);
	if (Button* levelUp = dynamic_cast<Button*>(window->GetControlByID(kRecLevelUpButtonID)))
		levelUp->SetEnabled(actor->CanLevelUp());
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
	if (classLabel != NULL)
		classLabel->SetText(_ClassTitle(actor));

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
