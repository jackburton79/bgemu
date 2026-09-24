#include "LootWindow.h"

#include "Actor.h"
#include "BamResource.h"
#include "Bitmap.h"
#include "Button.h"
#include "Container.h"
#include "Core.h"
#include "CreResource.h"
#include "GUI.h"
#include "Label.h"
#include "ResManager.h"
#include "Scrollbar.h"
#include "ScreenSupport.h"
#include "Window.h"

#include <algorithm>
#include <iostream>
#include <string>

// GUIW window 8 - the loot window (identical layout in BG1 and BG2, confirmed
// by dumping both GUIW.CHU): ids 0-5 are the source's item slots (3
// columns x 2 rows), 10-13 the looter's own carried items (2 columns x 2
// rows), 52/53 their scrollbars (whole rows), 50 the container-type icon,
// 51 "Done", 54 the bag icon the encumbrance labels sit on. Ids and behavior
// follow GemRB's CommonWindow.py OpenContainerWindow().
static const uint32 kContainerSourceSlots = 6;
static const uint32 kContainerSourceColumns = 3;
static const uint32 kContainerOwnFirstID = 10;
static const uint32 kContainerOwnSlots = 4;
static const uint32 kContainerOwnColumns = 2;
static const uint32 kContainerIconID = 50;
static const uint32 kContainerDoneID = 51;
static const uint32 kContainerSourceScrollID = 52;
static const uint32 kContainerOwnScrollID = 53;
static const uint32 kContainerWeightIconID = 54;
static const uint32 kContainerGoldLabelID = 268435510;

// Per ARE container type (index = type; GemRB's own shared containr.2da,
// which real game installs don't ship): open sound, icon BAM, close sound.
// "" = none.
struct container_type_info { const char* openSound; const char* icon; const char* closeSound; };
static const container_type_info kContainerTypes[] = {
	{ "", "", "" },
	{ "GAM_12A1", "CONTSACK", "GAM_12A" },	// bag
	{ "AMB_D05A", "CONTCHST", "AMB_D05B" },	// chest
	{ "AMB_D05A", "CONTDRWR", "AMB_D05B" },	// drawer
	{ "AMB_D18", "CONTGRND", "" },			// pile
	{ "AMB_D08", "CONTTABL", "" },			// table
	{ "AMB_D07", "CONTSHLF", "" },			// shelf
	{ "AMB_D07", "CONTALTR", "" },			// altar
	{ "AMB_D18", "", "" },					// non-visible
	{ "GAM_06", "CONTBOOK", "GAM_05" },		// spellbook
	{ "AMB_D08G", "CONTBODY", "" },			// body
	{ "AMB_D12", "CONTBARL", "AMB_D13" },	// barrel
	{ "AMB_D05A", "CONTCRAT", "AMB_D05B" },	// crate
};
static const uint16 kContainerTypeBody = 10;


static const container_type_info&
_ContainerTypeInfo(Object* source)
{
	uint16 type = kContainerTypeBody; // a corpse
	if (Container* container = dynamic_cast<Container*>(source))
		type = container->Type();
	if (type >= sizeof(kContainerTypes) / sizeof(kContainerTypes[0]))
		type = 0;
	return kContainerTypes[type];
}


// What `source` (a Container, or a dead Actor's CRE slots) currently holds.
static void
_CollectSourceEntries(Object* source, std::vector<ScreenSupport::LootEntry>& entries)
{
	if (Container* container = dynamic_cast<Container*>(source)) {
		for (uint32 i = 0; i < container->ItemCount(); i++)
			entries.push_back({ container->ItemAt(i), (int32)i });
	} else if (Actor* corpse = dynamic_cast<Actor*>(source)) {
		if (corpse->CRE() == NULL)
			return;
		for (uint32 slot = 0; slot < kNumItemSlots; slot++) {
			IE::item item;
			if (corpse->CRE()->GetItemAtSlot(slot, item) && item.name.name[0] != '\0')
				entries.push_back({ item, (int32)slot });
		}
	}
}


static bool
_TakeSourceEntry(Object* source, const ScreenSupport::LootEntry& entry, IE::item& out)
{
	if (Container* container = dynamic_cast<Container*>(source))
		return container->TakeItemAt((uint32)entry.slot, out);
	if (Actor* corpse = dynamic_cast<Actor*>(source))
		return corpse->TakeItemFromSlot((uint32)entry.slot, out);
	return false;
}


static bool
_AddToSource(Object* source, const IE::item& item)
{
	if (Container* container = dynamic_cast<Container*>(source)) {
		container->AddContainerItem(item);
		return true;
	}
	if (Actor* corpse = dynamic_cast<Actor*>(source))
		return corpse->AddItem(item);
	return false;
}


// Loot entry behind a slot button, or NULL. `entries` must outlive the
// returned pointer.
static const ScreenSupport::LootEntry*
_EntryAt(const std::vector<ScreenSupport::LootEntry>& entries, int32 row, uint32 columns,
	uint32 slotIndex)
{
	size_t index = (size_t)row * columns + slotIndex;
	return index < entries.size() ? &entries[index] : NULL;
}


LootWindow::LootWindow()
	:
	fSource(NULL),
	fLooter(NULL),
	fLeftRow(0),
	fRightRow(0)
{
}


void
LootWindow::Open(Actor* looter, Object* source)
{
	if (looter == NULL || looter->CRE() == NULL || source == NULL)
		return;
	if (IsOpen())
		Close();

	GUI* gui = GUI::Get();
	// The loot window takes over the bottom of the screen: the message
	// area and the command bar (same as the original).
	fHiddenWindows.clear();
	for (uint16 id : { (uint16)GUI::WINDOW_CMDS, (uint16)GUI::WINDOW_MESSAGES,
			(uint16)GUI::WINDOW_MESSAGES_LARGE }) {
		if (gui->IsWindowShown(id)) {
			fHiddenWindows.push_back(id);
			gui->HideWindow(id);
		}
	}

	fSource = source;
	fLooter = looter;
	fLeftRow = 0;
	fRightRow = 0;

	gui->ShowWindow(GUI::WINDOW_CONTAINER);
	Window* window = gui->GetWindow(GUI::WINDOW_CONTAINER);
	if (window == NULL) {
		Close();
		return;
	}

	if (Scrollbar* scrollbar = dynamic_cast<Scrollbar*>(
			window->GetControlByID(kContainerSourceScrollID))) {
		scrollbar->SetRowCallback([this](int32 row) {
			fLeftRow = row;
			_Refresh();
		});
	}
	if (Scrollbar* scrollbar = dynamic_cast<Scrollbar*>(
			window->GetControlByID(kContainerOwnScrollID))) {
		scrollbar->SetRowCallback([this](int32 row) {
			fRightRow = row;
			_Refresh();
		});
	}

	const container_type_info& info = _ContainerTypeInfo(source);
	if (Button* icon = dynamic_cast<Button*>(window->GetControlByID(kContainerIconID))) {
		Bitmap* frame = NULL;
		if (info.icon[0] != '\0') {
			if (BAMResource* bam = gResManager->GetBAM(info.icon)) {
				frame = bam->FrameForCycle(0, 0);
				gResManager->ReleaseResource(bam);
			}
		}
		icon->SetIcon(frame, true);
	}
	if (info.openSound[0] != '\0')
		Core::Get()->PlaySound(info.openSound);

	_Refresh();
}


void
LootWindow::Close()
{
	if (!IsOpen())
		return;

	// No GUI left to restore when this runs during shutdown.
	if (GUI* gui = GUI::Get()) {
		const container_type_info& info = _ContainerTypeInfo(fSource);
		gui->HideWindow(GUI::WINDOW_CONTAINER);
		gui->SetHoverTooltip("");
		for (uint16 id : fHiddenWindows)
			gui->ShowWindow(id);
		if (info.closeSound[0] != '\0')
			Core::Get()->PlaySound(info.closeSound);
	}
	fHiddenWindows.clear();

	fSource = NULL;
	fLooter = NULL;
}


bool
LootWindow::IsOpen() const
{
	return fSource != NULL;
}


void
LootWindow::_Refresh()
{
	if (!IsOpen())
		return;
	Window* window = GUI::Get()->GetWindow(GUI::WINDOW_CONTAINER);
	if (window == NULL)
		return;

	std::vector<ScreenSupport::LootEntry> sourceEntries, ownEntries;
	_CollectSourceEntries(fSource, sourceEntries);
	ScreenSupport::CollectOwnEntries(fLooter, ownEntries);

	// Rows the list scrolls by: everything past the visible slots, in
	// whole rows.
	auto maxRow = [](size_t count, uint32 visible, uint32 columns) -> int32 {
		return count > visible ? (int32)((count - visible + columns - 1) / columns) : 0;
	};
	const int32 sourceMaxRow = maxRow(sourceEntries.size(), kContainerSourceSlots,
		kContainerSourceColumns);
	const int32 ownMaxRow = maxRow(ownEntries.size(), kContainerOwnSlots,
		kContainerOwnColumns);
	fLeftRow = std::min(fLeftRow, sourceMaxRow);
	fRightRow = std::min(fRightRow, ownMaxRow);

	auto fill = [&](uint32 firstID, uint32 slots, uint32 columns, int32 row,
			const std::vector<ScreenSupport::LootEntry>& entries) {
		for (uint32 i = 0; i < slots; i++) {
			Button* button = dynamic_cast<Button*>(window->GetControlByID(firstID + i));
			if (button == NULL)
				continue;
			const ScreenSupport::LootEntry* entry = _EntryAt(entries, row, columns, i);
			button->SetIcon(entry != NULL ? ScreenSupport::MakeItemIcon(entry->item.name) : NULL);
			button->SetIconCount(entry != NULL ? entry->item.quantity1 : 0);
		}
	};
	fill(0, kContainerSourceSlots, kContainerSourceColumns, fLeftRow, sourceEntries);
	fill(kContainerOwnFirstID, kContainerOwnSlots, kContainerOwnColumns, fRightRow,
		ownEntries);

	if (Scrollbar* scrollbar = dynamic_cast<Scrollbar*>(
			window->GetControlByID(kContainerSourceScrollID)))
		scrollbar->SetScrollInfo(fLeftRow, sourceMaxRow);
	if (Scrollbar* scrollbar = dynamic_cast<Scrollbar*>(
			window->GetControlByID(kContainerOwnScrollID)))
		scrollbar->SetScrollInfo(fRightRow, ownMaxRow);

	if (Label* gold = dynamic_cast<Label*>(window->GetControlByID(kContainerGoldLabelID)))
		gold->SetText(std::to_string(Core::Get()->PartyGold()));
	ScreenSupport::EnsureWeightLabels(window, kContainerWeightIconID);
	if (Label* label = dynamic_cast<Label*>(window->GetControlByID(ScreenSupport::kWeightCurrentLabelID)))
		label->SetText(std::to_string(ScreenSupport::CarriedWeight(fLooter->CRE())) + ":");
	if (Label* label = dynamic_cast<Label*>(window->GetControlByID(ScreenSupport::kWeightMaxLabelID)))
		label->SetText(std::to_string(ScreenSupport::MaxEncumbrance(fLooter->CRE())) + ":");
}


// Click on a loot window control: a source slot moves that item into the
// looter's inventory, an own-inventory slot moves it into the source, Done
// closes.
void
LootWindow::ControlInvoked(uint32 controlID)
{
	if (!IsOpen())
		return;

	if (controlID == kContainerDoneID) {
		Close();
		return;
	}

	if (controlID < kContainerSourceSlots) {
		std::vector<ScreenSupport::LootEntry> entries;
		_CollectSourceEntries(fSource, entries);
		const ScreenSupport::LootEntry* entry = _EntryAt(entries, fLeftRow,
			kContainerSourceColumns, controlID);
		if (entry == NULL)
			return;
		// Add first, remove only once it fits - a full inventory leaves the
		// item where it is.
		if (!fLooter->AddItem(entry->item)) {
			std::cout << fLooter->Name() << " has no room for "
				<< ScreenSupport::ItemDisplayName(entry->item.name) << std::endl;
			return;
		}
		IE::item taken;
		_TakeSourceEntry(fSource, *entry, taken);
		std::cout << fLooter->Name() << " takes " << ScreenSupport::ItemDisplayName(taken.name)
			<< std::endl;
	} else if (controlID >= kContainerOwnFirstID
			&& controlID < kContainerOwnFirstID + kContainerOwnSlots) {
		std::vector<ScreenSupport::LootEntry> entries;
		ScreenSupport::CollectOwnEntries(fLooter, entries);
		const ScreenSupport::LootEntry* entry = _EntryAt(entries, fRightRow,
			kContainerOwnColumns, controlID - kContainerOwnFirstID);
		if (entry == NULL)
			return;
		IE::item taken;
		if (!fLooter->TakeItemFromSlot((uint32)entry->slot, taken))
			return;
		if (!_AddToSource(fSource, taken)) {
			fLooter->AddItem(taken); // no room over there - put it back
			return;
		}
		std::cout << fLooter->Name() << " puts " << ScreenSupport::ItemDisplayName(taken.name)
			<< " away" << std::endl;
	} else {
		return;
	}

	_Refresh();
}


void
LootWindow::ControlHovered(uint32 controlID, bool inside)
{
	if (!inside || !IsOpen()) {
		GUI::Get()->SetHoverTooltip("");
		return;
	}

	std::vector<ScreenSupport::LootEntry> entries;
	const ScreenSupport::LootEntry* entry = NULL;
	if (controlID < kContainerSourceSlots) {
		_CollectSourceEntries(fSource, entries);
		entry = _EntryAt(entries, fLeftRow, kContainerSourceColumns, controlID);
	} else if (controlID >= kContainerOwnFirstID
			&& controlID < kContainerOwnFirstID + kContainerOwnSlots) {
		ScreenSupport::CollectOwnEntries(fLooter, entries);
		entry = _EntryAt(entries, fRightRow, kContainerOwnColumns,
			controlID - kContainerOwnFirstID);
	}
	GUI::Get()->SetHoverTooltip(entry != NULL ? ScreenSupport::ItemDisplayName(entry->item.name) : "");
}
