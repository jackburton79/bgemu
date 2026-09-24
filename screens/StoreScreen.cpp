#include "StoreScreen.h"

#include "Actor.h"
#include "Bitmap.h"
#include "Button.h"
#include "Core.h"
#include "CreResource.h"
#include "Game.h"
#include "GUI.h"
#include "ITMResource.h"
#include "Label.h"
#include "LootWindow.h"
#include "ResManager.h"
#include "ScreenSupport.h"
#include "Scrollbar.h"
#include "STOResource.h"
#include "Store.h"
#include "TextArea.h"
#include "Window.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>

// GUISTORE.CHU (BG1 and BG2 share this layout; confirmed by dumping both).
// Window 2 is the Buy/Sell page: 4 shelf slots (5-8) with a scrollbar (11)
// on the left, 4 slots (13-16) with a scrollbar (12) for the shown party
// member's items on the right, each slot with a name/price label, sums and
// buttons below; window 3 is the bottom bar with "Done"; 0 and 1 are the
// side columns (in a store the left one is just decoration, the right one
// picks who shops). Control ids follow GemRB's GUISTORE.py.
static const uint16 kStoreShopWindow = 2;
static const uint16 kStoreBarWindow = 3;
static const uint32 kStoreTitleLabelID = 268435459;
static const uint32 kStoreGoldLabelID = 268435498;
static const uint32 kStoreBuySumLabelID = 268435499;
static const uint32 kStoreSellSumLabelID = 268435500;
static const uint32 kStoreCustomerLabelID = 268435502;
static const uint32 kStoreBuyButtonID = 2;
static const uint32 kStoreSellButtonID = 3;
static const uint32 kStoreShelfFirstID = 5;
static const uint32 kStoreOwnFirstID = 13;
static const uint32 kStoreShelfNameFirstID = 268435474;
static const uint32 kStoreOwnNameFirstID = 268435486;
static const uint32 kStoreShelfScrollID = 11;
static const uint32 kStoreOwnScrollID = 12;
static const uint32 kStoreBagIconID = 44;
static const uint32 kStoreDoneButtonID = 0;
static const uint32 kStoreSlots = 4;
// Window 16, BG2 only: the quantity picker for a shelf item - the item's
// icon (0), Cancel (1), Done (2), the two arrows (3 raises, 4 lowers - per
// GemRB's GUISTORE.py) and a text box (6) this engine has no widget for,
// so the amount is drawn on a label over it instead.
static const uint16 kStoreAmountWindow = 16;
static const uint32 kStoreAmountIconID = 0;
static const uint32 kStoreAmountCancelID = 1;
static const uint32 kStoreAmountDoneID = 2;
static const uint32 kStoreAmountRaiseID = 3;
static const uint32 kStoreAmountLowerID = 4;
static const uint32 kStoreAmountBoxID = 6;
static const uint32 kStoreAmountNameLabelID = 268435460;
static const uint32 kStoreAmountValueLabelID = 268435525;
static const uint32 kStoreCancelStrRef = 13727;
// What GemRB caps a picker at for an infinite supply.
static const uint32 kStoreAmountInfiniteMax = 999;
// Window 4, the Identify page: the store name (title), gold, customer name,
// cost-so-far labels, the Identify button (5), a scrollbar (7) for the 4 item
// slots (8-11) with their name/price labels, and a text area (23) where what
// identifying revealed is written.
static const uint16 kStoreIdentifyWindow = 4;
static const uint32 kStoreIdTitleLabelID = 268435456;
static const uint32 kStoreIdGoldLabelID = 268435457;
static const uint32 kStoreIdCostLabelID = 268435459;
static const uint32 kStoreIdCustomerLabelID = 268435461;
static const uint32 kStoreIdentifyButtonID = 5;
static const uint32 kStoreIdScrollID = 7;
static const uint32 kStoreIdSlotFirstID = 8;
static const uint32 kStoreIdNameFirstID = 268435468;
static const uint32 kStoreIdTextAreaID = 23;
// Store pages, and the action numbers GemRB's GUISTORE.py gives them (the
// tab buttons' art cycle is the action number).
enum store_page { STORE_PAGE_SHOP = 0, STORE_PAGE_IDENTIFY = 1 };
static const int32 kStoreTabNone = -1;
static const uint32 kStoreTabButtonFirstID = 1;
static const uint32 kStoreTabCount = 4;
// TLK strings: the Buy/Sell button captions, "Done", the name-and-price
// line template ("<ITEMNAME>" / "<ITEMCOST>" tokens) and the "can't
// afford it" message.
static const uint32 kStoreBuyStrRef = 13703;
static const uint32 kStoreSellStrRef = 13704;
static const uint32 kStoreDoneStrRef = 11973;
static const uint32 kStoreNameAndCostStrRef = 10162;
static const uint32 kStoreTooCostlyStrRef = 11047;
static const uint32 kStoreIdentifyStrRef = 14133;
static const uint32 kStoreIdTooCostlyStrRef = 11050;


// "<name>" then "<price>" on the line the game's own template asks for.
static std::string
_StoreItemLine(const std::string& name, int32 price)
{
	std::string line = IDTable::GetDialog(kStoreNameAndCostStrRef);
	auto replace = [&line](const std::string& token, const std::string& value) {
		size_t at = line.find(token);
		if (at != std::string::npos)
			line.replace(at, token.length(), value);
	};
	if (line.find("<ITEMNAME>") == std::string::npos)
		return name + "\n" + std::to_string(price);
	replace("<ITEMNAME>", name);
	replace("<ITEMCOST>", std::to_string(price));
	return line;
}


static std::string
_StoreItemName(const IE::item& item, bool identified)
{
	ITMResource* itm = gResManager->GetITM(item.name);
	std::string name;
	if (itm != NULL) {
		name = IDTable::GetDialog(identified ? itm->IdentifiedNameRef()
			: itm->UnidentifiedNameRef());
		gResManager->ReleaseResource(itm);
	}
	return name.empty() ? std::string(item.name.CString()) : name;
}


static int32
_StoreStackSize(const IE::item& item)
{
	ITMResource* itm = gResManager->GetITM(item.name);
	int32 size = 0;
	if (itm != NULL) {
		if (itm->StackAmount() > 1)
			size = item.quantity1;
		gResManager->ReleaseResource(itm);
	}
	return size;
}


static std::string
_UpperCase(std::string text)
{
	std::transform(text.begin(), text.end(), text.begin(),
		[](unsigned char c) { return (char)std::toupper(c); });
	return text;
}


bool
StoreScreen::OpenStore(Actor* customer, const res_ref& storeName)
{
	if (customer == NULL || customer->CRE() == NULL)
		return false;

	auto found = fStores.find(storeName.CString());
	if (found == fStores.end()) {
		Store* loaded = Store::Load(storeName);
		if (loaded == NULL)
			return false;
		found = fStores.insert({ storeName.CString(), loaded }).first;
	}
	if (!found->second->IsShop())
		return false;

	fGame.Loot().Close();

	fStore = found->second;
	fCustomer = NULL;
	fSellSlots.clear();
	fLeftRow = 0;
	fRightRow = 0;
	for (store_entry& entry : fStore->Items()) {
		entry.selected = false;
		entry.purchased = 0;
	}

	// Whoever is shopping is the character the side column highlights.
	fGame.SetShownActor(customer);

	// The game stands still while shopping.
	if (!Core::Get()->IsPaused()) {
		Core::Get()->TogglePause();
		fUnpause = true;
	}

	Open();
	return true;
}


// Everything the window shows that isn't the page itself.
/* virtual */
void
StoreScreen::Refresh()
{
	fGame.UpdatePortraitColumn(GetWindow(1), 6);
	if (Window* bar = GetWindow(kStoreBarWindow)) {
		if (Button* done = dynamic_cast<Button*>(bar->GetControlByID(kStoreDoneButtonID)))
			done->SetText(IDTable::GetDialog(kStoreDoneStrRef));
	}

	_SetupTabs();
	fPage = -1;
	_ShowPage(STORE_PAGE_SHOP);
}


/* virtual */
void
StoreScreen::OnShownCharacterChanged()
{
	_UpdateWindow();
}


// Which pages the tab buttons offer, in order - GemRB's table by store type
// (GUIScript.cpp's storebuttons): the store's main function first, then the
// optional ones its flags switch on. Numbers are the action ids the tab
// art (GUISTBBC.BAM's cycles) is indexed by: 0 buy/sell, 1 identify, 2
// steal, 3 cure, 4 donate, 5 drink, 6 rent.
static std::vector<int32>
_StoreTabsFor(const Store* store)
{
	static const int32 kOptional = 0x80;
	// Store flag bit each action needs (0 = always).
	static const uint32 kFlagFor[7] = { 0, 4, 8, 32, 16, 64, 128 };
	static const int32 kTypeTabs[4][7] = {
		{ 0, 1 | kOptional, 2 | kOptional, 4 | kOptional, 3 | kOptional, 5 | kOptional, 6 | kOptional },
		{ 5, 0 | kOptional, 1 | kOptional, 2 | kOptional, 4 | kOptional, 3 | kOptional, 6 | kOptional },
		{ 6, 0 | kOptional, 5 | kOptional, 2 | kOptional, 1 | kOptional, 4 | kOptional, 3 | kOptional },
		{ 3, 4 | kOptional, 0 | kOptional, 1 | kOptional, 2 | kOptional, 5 | kOptional, 6 | kOptional },
	};
	std::vector<int32> tabs;
	const uint32 type = std::min<uint32>(store->Type(), 3);
	for (int32 entry : kTypeTabs[type]) {
		int32 action = entry & ~kOptional;
		if ((entry & kOptional)
				&& (kFlagFor[action] == 0 || !(store->Flags() & kFlagFor[action])) && action != 0)
			continue;
		// A buy/sell tab needs the store to buy or sell something.
		if (action == 0 && !(store->Flags() & (STO_CAN_BUY | STO_CAN_SELL)))
			continue;
		tabs.push_back(action);
	}
	tabs.resize(kStoreTabCount, kStoreTabNone);
	return tabs;
}


// Lays the four tab buttons of the bottom bar out for this store: each gets
// the art of its page, pages this window doesn't implement yet are shown
// disabled, unused slots are blank.
void
StoreScreen::_SetupTabs()
{
	fTabs = _StoreTabsFor(fStore);
	Window* bar = GetWindow(kStoreBarWindow);
	if (bar == NULL)
		return;
	for (uint32 i = 0; i < kStoreTabCount; i++) {
		Button* tab = dynamic_cast<Button*>(bar->GetControlByID(kStoreTabButtonFirstID + i));
		if (tab == NULL)
			continue;
		const int32 action = fTabs[i];
		if (action == kStoreTabNone) {
			tab->SetFrameless(true);
			tab->SetEnabled(false);
			continue;
		}
		tab->SetFrameless(false);
		tab->SetCycle((uint16)action);
		tab->SetEnabled(action == STORE_PAGE_SHOP || action == STORE_PAGE_IDENTIFY);
	}
}


// Switches to a page: its window replaces the other's, selections start
// blank and the tab of the page is marked.
void
StoreScreen::_ShowPage(int32 page)
{
	GUI* gui = GUI::Get();
	if (page == fPage || fStore == NULL)
		return;

	_CloseAmountWindow(false);
	for (store_entry& entry : fStore->Items()) {
		entry.selected = false;
		entry.purchased = 0;
	}
	fSellSlots.clear();
	fIdentifySlots.clear();
	fIdentifyRow = 0;
	fPage = page;

	gui->HideAuxWindow(CHUName(), page == STORE_PAGE_SHOP ? kStoreIdentifyWindow : kStoreShopWindow);
	gui->ShowAuxWindow(CHUName(), page == STORE_PAGE_SHOP ? kStoreShopWindow : kStoreIdentifyWindow);

	if (page == STORE_PAGE_SHOP) {
		if (Window* window = GetWindow(kStoreShopWindow)) {
			if (Scrollbar* scrollbar = dynamic_cast<Scrollbar*>(
					window->GetControlByID(kStoreShelfScrollID))) {
				scrollbar->SetRowCallback([this](int32 row) {
					fLeftRow = row;
					_UpdateWindow();
				});
			}
			if (Scrollbar* scrollbar = dynamic_cast<Scrollbar*>(
					window->GetControlByID(kStoreOwnScrollID))) {
				scrollbar->SetRowCallback([this](int32 row) {
					fRightRow = row;
					_UpdateWindow();
				});
			}
			if (Button* buy = dynamic_cast<Button*>(window->GetControlByID(kStoreBuyButtonID)))
				buy->SetText(IDTable::GetDialog(kStoreBuyStrRef));
			if (Button* sell = dynamic_cast<Button*>(window->GetControlByID(kStoreSellButtonID)))
				sell->SetText(IDTable::GetDialog(kStoreSellStrRef));
		}
	} else if (Window* window = GetWindow(kStoreIdentifyWindow)) {
		if (Scrollbar* scrollbar = dynamic_cast<Scrollbar*>(
				window->GetControlByID(kStoreIdScrollID))) {
			scrollbar->SetRowCallback([this](int32 row) {
				fIdentifyRow = row;
				_UpdateWindow();
			});
		}
		if (Button* identify = dynamic_cast<Button*>(
				window->GetControlByID(kStoreIdentifyButtonID)))
			identify->SetText(IDTable::GetDialog(kStoreIdentifyStrRef));
		if (TextArea* text = dynamic_cast<TextArea*>(window->GetControlByID(kStoreIdTextAreaID)))
			text->ClearText();
	}

	if (Window* bar = GetWindow(kStoreBarWindow)) {
		for (uint32 i = 0; i < kStoreTabCount; i++) {
			if (Button* tab = dynamic_cast<Button*>(bar->GetControlByID(kStoreTabButtonFirstID + i)))
				tab->SetToggled(fTabs[i] == page);
		}
	}
	_UpdateWindow();
}


/* virtual */
void
StoreScreen::OnClose()
{
	_CloseAmountWindow(false);
	GUI* gui = GUI::Get();
	for (uint16 id : { kStoreShopWindow, kStoreIdentifyWindow })
		gui->HideAuxWindow(CHUName(), id);
	gui->SetHoverTooltip("");
	if (fUnpause) {
		fUnpause = false;
		if (Core::Get()->IsPaused())
			Core::Get()->TogglePause();
	}
	if (fStore != NULL) {
		for (store_entry& entry : fStore->Items()) {
			entry.selected = false;
			entry.purchased = 0;
		}
	}
	fStore = NULL;
	fCustomer = NULL;
	fSellSlots.clear();
	fIdentifySlots.clear();
	fPage = -1;
}


/* virtual */
bool
StoreScreen::IsOpen() const
{
	return fStore != NULL && GameScreen::IsOpen();
}


Store*
StoreScreen::LoadedStore(const char* name) const
{
	auto found = fStores.find(name);
	return found != fStores.end() ? found->second : NULL;
}


void
StoreScreen::ClearStores()
{
	fStore = NULL;
	for (auto& store : fStores)
		delete store.second;
	fStores.clear();
}


// Re-draws the whole Buy/Sell page from the store's stock and the shown
// party member's inventory.
void
StoreScreen::_UpdateWindow()
{
	if (!IsOpen())
		return;
	Actor* customer = fGame.ShownActor();
	if (customer == NULL || customer->CRE() == NULL)
		return;

	// Someone else's turn at the counter: their selection starts blank.
	if (customer != fCustomer) {
		fCustomer = customer;
		fSellSlots.clear();
		fIdentifySlots.clear();
		fRightRow = 0;
		fIdentifyRow = 0;
	}

	if (fPage == STORE_PAGE_IDENTIFY)
		_UpdateIdentifyPage();
	else
		_UpdateShopPage();
}


// The Buy/Sell page (window 2).
void
StoreScreen::_UpdateShopPage()
{
	Window* window = GetWindow(kStoreShopWindow);
	Actor* customer = fGame.ShownActor();
	if (window == NULL || customer == NULL || customer->CRE() == NULL)
		return;

	std::vector<store_entry>& shelf = fStore->Items();
	std::vector<ScreenSupport::LootEntry> own;
	ScreenSupport::CollectOwnEntries(customer, own);

	auto maxRow = [](size_t count) -> int32 {
		return count > kStoreSlots ? (int32)(count - kStoreSlots) : 0;
	};
	fLeftRow = std::min(fLeftRow, maxRow(shelf.size()));
	fRightRow = std::min(fRightRow, maxRow(own.size()));

	// What the selections come to.
	int32 buySum = 0;
	for (const store_entry& entry : shelf) {
		if (!entry.selected)
			continue;
		int32 price = fStore->PriceToBuy(entry, customer) * (int32)entry.purchased;
		buySum += price > 0 ? price : (int32)entry.purchased;
	}
	int32 sellSum = 0;
	for (const ScreenSupport::LootEntry& entry : own) {
		if (fSellSlots.count((uint32)entry.slot) == 0)
			continue;
		bool identified = (Store::SlotFlags(entry.item) & STORE_ITEM_IDENTIFIED) != 0;
		sellSum += identified ? fStore->PriceToSell(entry.item, customer) : 1;
	}

	auto setLabel = [window](uint32 id, const std::string& text) {
		if (Label* label = dynamic_cast<Label*>(window->GetControlByID(id)))
			label->SetText(text);
	};
	setLabel(kStoreTitleLabelID, _UpperCase(IDTable::GetDialog(fStore->NameRef())));
	setLabel(kStoreCustomerLabelID, customer->LongName());
	setLabel(kStoreGoldLabelID, std::to_string(Core::Get()->PartyGold()));
	setLabel(kStoreBuySumLabelID, std::to_string(buySum));
	setLabel(kStoreSellSumLabelID, std::to_string(sellSum));

	if (Button* buy = dynamic_cast<Button*>(window->GetControlByID(kStoreBuyButtonID)))
		buy->SetEnabled(buySum > 0);
	if (Button* sell = dynamic_cast<Button*>(window->GetControlByID(kStoreSellButtonID)))
		sell->SetEnabled(sellSum > 0);

	for (uint32 i = 0; i < kStoreSlots; i++) {
		// Shelf.
		Button* slot = dynamic_cast<Button*>(window->GetControlByID(kStoreShelfFirstID + i));
		size_t index = (size_t)fLeftRow + i;
		if (slot != NULL) {
			if (index < shelf.size()) {
				const store_entry& entry = shelf[index];
				bool buyable = (fStore->Actions(entry.item, false) & STORE_ACT_BUY) != 0;
				slot->SetIcon(ScreenSupport::MakeItemIcon(entry.item.name));
				slot->SetIconCount(_StoreStackSize(entry.item));
				slot->SetHighlighted(entry.selected);
				slot->SetEnabled(buyable);
			} else {
				slot->SetIcon(NULL);
				slot->SetIconCount(0);
				slot->SetHighlighted(false);
			}
		}
		std::string line;
		if (index < shelf.size()) {
			const store_entry& entry = shelf[index];
			bool identified = (entry.item.flags & STORE_ITEM_IDENTIFIED) != 0;
			int32 price = fStore->PriceToBuy(entry, customer);
			line = _StoreItemLine(_StoreItemName(entry.item, identified), price);
			if (entry.amount >= 0)
				line += " (" + std::to_string(entry.amount) + ")";
		}
		setLabel(kStoreShelfNameFirstID + i, line);

		// The party member's own things.
		slot = dynamic_cast<Button*>(window->GetControlByID(kStoreOwnFirstID + i));
		index = (size_t)fRightRow + i;
		line.clear();
		if (index < own.size()) {
			const ScreenSupport::LootEntry& entry = own[index];
			uint32 flags = Store::SlotFlags(entry.item);
			bool identified = (flags & STORE_ITEM_IDENTIFIED) != 0;
			bool sellable = (fStore->Actions(entry.item, true) & STORE_ACT_SELL) != 0;
			int32 price = identified ? fStore->PriceToSell(entry.item, customer) : 1;
			if (slot != NULL) {
				slot->SetIcon(ScreenSupport::MakeItemIcon(entry.item.name));
				slot->SetIconCount(_StoreStackSize(entry.item));
				slot->SetHighlighted(fSellSlots.count((uint32)entry.slot) != 0);
				slot->SetEnabled(sellable);
			}
			line = _StoreItemLine(_StoreItemName(entry.item, identified), price);
		} else if (slot != NULL) {
			slot->SetIcon(NULL);
			slot->SetIconCount(0);
			slot->SetHighlighted(false);
		}
		setLabel(kStoreOwnNameFirstID + i, line);
	}

	if (Scrollbar* scrollbar = dynamic_cast<Scrollbar*>(
			window->GetControlByID(kStoreShelfScrollID)))
		scrollbar->SetScrollInfo(fLeftRow, maxRow(shelf.size()));
	if (Scrollbar* scrollbar = dynamic_cast<Scrollbar*>(
			window->GetControlByID(kStoreOwnScrollID)))
		scrollbar->SetScrollInfo(fRightRow, maxRow(own.size()));

	ScreenSupport::EnsureWeightLabels(window, kStoreBagIconID);
	if (Label* label = dynamic_cast<Label*>(window->GetControlByID(ScreenSupport::kWeightCurrentLabelID)))
		label->SetText(std::to_string(ScreenSupport::CarriedWeight(customer->CRE())) + ":");
	if (Label* label = dynamic_cast<Label*>(window->GetControlByID(ScreenSupport::kWeightMaxLabelID)))
		label->SetText(std::to_string(ScreenSupport::MaxEncumbrance(customer->CRE())) + ":");

	fGame.UpdatePortraitColumn(GetWindow(1), 6);
}


// The Identify page (window 4): the shown party member's items, the
// unidentified ones selectable, at the store's price each.
void
StoreScreen::_UpdateIdentifyPage()
{
	Window* window = GetWindow(kStoreIdentifyWindow);
	Actor* customer = fGame.ShownActor();
	if (window == NULL || customer == NULL || customer->CRE() == NULL)
		return;

	std::vector<ScreenSupport::LootEntry> own;
	ScreenSupport::CollectOwnEntries(customer, own);
	const int32 maxRow = own.size() > kStoreSlots ? (int32)(own.size() - kStoreSlots) : 0;
	fIdentifyRow = std::min(fIdentifyRow, maxRow);

	const int32 price = (int32)fStore->IdentifyPrice();
	int32 selected = 0;
	for (const ScreenSupport::LootEntry& entry : own) {
		if (fIdentifySlots.count((uint32)entry.slot))
			selected++;
	}

	auto setLabel = [window](uint32 id, const std::string& text) {
		if (Label* label = dynamic_cast<Label*>(window->GetControlByID(id)))
			label->SetText(text);
	};
	setLabel(kStoreIdTitleLabelID, _UpperCase(IDTable::GetDialog(fStore->NameRef())));
	setLabel(kStoreIdCustomerLabelID, customer->LongName());
	setLabel(kStoreIdGoldLabelID, std::to_string(Core::Get()->PartyGold()));
	setLabel(kStoreIdCostLabelID, std::to_string(price * selected));
	if (Button* identify = dynamic_cast<Button*>(window->GetControlByID(kStoreIdentifyButtonID)))
		identify->SetEnabled(selected > 0);

	for (uint32 i = 0; i < kStoreSlots; i++) {
		Button* slot = dynamic_cast<Button*>(window->GetControlByID(kStoreIdSlotFirstID + i));
		const size_t index = (size_t)fIdentifyRow + i;
		std::string line;
		if (index < own.size()) {
			const ScreenSupport::LootEntry& entry = own[index];
			const bool identified = (Store::SlotFlags(entry.item) & STORE_ITEM_IDENTIFIED) != 0;
			if (slot != NULL) {
				slot->SetIcon(ScreenSupport::MakeItemIcon(entry.item.name));
				slot->SetIconCount(_StoreStackSize(entry.item));
				slot->SetHighlighted(fIdentifySlots.count((uint32)entry.slot) != 0);
				slot->SetEnabled(!identified);
			}
			line = _StoreItemLine(_StoreItemName(entry.item, identified),
				identified ? 0 : price);
		} else if (slot != NULL) {
			slot->SetIcon(NULL);
			slot->SetIconCount(0);
			slot->SetHighlighted(false);
		}
		setLabel(kStoreIdNameFirstID + i, line);
	}

	if (Scrollbar* scrollbar = dynamic_cast<Scrollbar*>(window->GetControlByID(kStoreIdScrollID)))
		scrollbar->SetScrollInfo(fIdentifyRow, maxRow);
	fGame.UpdatePortraitColumn(GetWindow(1), 6);
}


void
StoreScreen::_IdentifySelected()
{
	Actor* customer = fGame.ShownActor();
	Window* window = GetWindow(kStoreIdentifyWindow);
	if (customer == NULL || window == NULL || fStore == NULL)
		return;

	const std::set<uint32> slots = fIdentifySlots;
	const int32 total = (int32)fStore->IdentifyPrice() * (int32)slots.size();
	if (total > Core::Get()->PartyGold()) {
		GUI::Get()->DisplayStringCentered(IDTable::GetDialog(kStoreIdTooCostlyStrRef),
			320, 200, 4000);
		return;
	}

	TextArea* text = dynamic_cast<TextArea*>(window->GetControlByID(kStoreIdTextAreaID));
	for (uint32 slot : slots) {
		IE::item item;
		if (!customer->CRE()->GetItemAtSlot(slot, item) || !customer->IdentifyItemInSlot(slot))
			continue;
		std::string name = _StoreItemName(item, true);
		std::cout << customer->Name() << " has " << name << " identified" << std::endl;
		// What identifying revealed: the item's real name and description.
		if (text != NULL) {
			std::string description;
			if (ITMResource* itm = gResManager->GetITM(item.name)) {
				description = IDTable::GetDialog(itm->DescriptionRef());
				gResManager->ReleaseResource(itm);
			}
			text->AddText((name + "\n\n" + description + "\n\n\n").c_str());
		}
	}
	Core::Get()->AddPartyGold(-total);
	fIdentifySlots.clear();
	_UpdateWindow();
}


void
StoreScreen::_BuySelected()
{
	Actor* customer = fGame.ShownActor();
	if (customer == NULL || fStore == NULL)
		return;

	std::vector<store_entry>& shelf = fStore->Items();
	int32 sum = 0;
	for (const store_entry& entry : shelf) {
		if (!entry.selected)
			continue;
		int32 price = fStore->PriceToBuy(entry, customer) * (int32)entry.purchased;
		sum += price > 0 ? price : (int32)entry.purchased;
	}
	if (sum > Core::Get()->PartyGold()) {
		GUI::Get()->DisplayStringCentered(IDTable::GetDialog(kStoreTooCostlyStrRef),
			320, 200, 4000);
		return;
	}

	// Backwards: bought-out entries leave the shelf and shift the rest.
	for (size_t i = shelf.size(); i > 0; i--) {
		const store_entry& entry = shelf[i - 1];
		if (!entry.selected)
			continue;
		int32 price = fStore->PriceToBuy(entry, customer) * (int32)entry.purchased;
		if (price <= 0)
			price = (int32)entry.purchased;
		std::string itemName = ScreenSupport::ItemDisplayName(entry.item.name);
		if (fStore->Buy(i - 1, customer)) {
			Core::Get()->AddPartyGold(-price);
			std::cout << customer->Name() << " buys " << itemName << " for "
				<< price << std::endl;
		} else {
			std::cout << customer->Name() << " has no room for " << itemName
				<< std::endl;
		}
	}
	_UpdateWindow();
}


void
StoreScreen::_SellSelected()
{
	Actor* customer = fGame.ShownActor();
	if (customer == NULL || fStore == NULL)
		return;

	const std::set<uint32> slots = fSellSlots;
	fSellSlots.clear();
	for (uint32 slot : slots) {
		IE::item item;
		if (!customer->CRE()->GetItemAtSlot(slot, item))
			continue;
		if (fStore->IsFull()) {
			std::cout << fStore->Name().CString() << " is full" << std::endl;
			break;
		}
		bool identified = (Store::SlotFlags(item) & STORE_ITEM_IDENTIFIED) != 0;
		int32 price = identified ? fStore->PriceToSell(item, customer) : 1;
		std::string itemName = ScreenSupport::ItemDisplayName(item.name);
		IE::item sold;
		if (!customer->TakeItemFromSlot(slot, sold))
			continue;
		fStore->Accept(sold);
		Core::Get()->AddPartyGold(price);
		std::cout << customer->Name() << " sells " << itemName << " for "
			<< price << std::endl;
	}
	_UpdateWindow();
}


/* virtual */
bool
StoreScreen::ControlInvoked(uint16 windowID, uint32 controlID)
{
	if (!IsOpen())
		return false;

	// The picker is modal: nothing else answers while it's up.
	if (fAmountIndex >= 0) {
		if (windowID != kStoreAmountWindow)
			return true;
		if (controlID == kStoreAmountRaiseID)
			fAmountValue = std::min(fAmountValue + 1, fAmountMax);
		else if (controlID == kStoreAmountLowerID)
			fAmountValue = fAmountValue > 0 ? fAmountValue - 1 : 0;
		else if (controlID == kStoreAmountDoneID)
			_CloseAmountWindow(true);
		else if (controlID == kStoreAmountCancelID)
			_CloseAmountWindow(false);
		_UpdateAmountWindow();
		return true;
	}

	if (windowID == kStoreBarWindow) {
		if (controlID == kStoreDoneButtonID) {
			Close();
		} else if (controlID >= kStoreTabButtonFirstID
				&& controlID < kStoreTabButtonFirstID + kStoreTabCount) {
			const int32 action = fTabs[controlID - kStoreTabButtonFirstID];
			if (action == STORE_PAGE_SHOP || action == STORE_PAGE_IDENTIFY)
				_ShowPage(action);
		}
		return true;
	}
	if (windowID == 1) {
		if (controlID <= 5)
			fGame.ShowCharacter((uint16)controlID);
		return true;
	}
	Actor* customer = fGame.ShownActor();
	if (customer == NULL)
		return true;

	if (windowID == kStoreIdentifyWindow && fPage == STORE_PAGE_IDENTIFY) {
		if (controlID == kStoreIdentifyButtonID) {
			_IdentifySelected();
		} else if (controlID >= kStoreIdSlotFirstID
				&& controlID < kStoreIdSlotFirstID + kStoreSlots) {
			std::vector<ScreenSupport::LootEntry> own;
			ScreenSupport::CollectOwnEntries(customer, own);
			size_t index = (size_t)fIdentifyRow + (controlID - kStoreIdSlotFirstID);
			if (index >= own.size()
					|| !(fStore->Actions(own[index].item, true) & STORE_ACT_IDENTIFY))
				return true;
			uint32 slot = (uint32)own[index].slot;
			if (!fIdentifySlots.erase(slot))
				fIdentifySlots.insert(slot);
			_UpdateWindow();
		}
		return true;
	}
	if (windowID != kStoreShopWindow || fPage != STORE_PAGE_SHOP)
		return true;

	if (controlID == kStoreBuyButtonID) {
		_BuySelected();
	} else if (controlID == kStoreSellButtonID) {
		_SellSelected();
	} else if (controlID >= kStoreShelfFirstID && controlID < kStoreShelfFirstID + kStoreSlots) {
		size_t index = (size_t)fLeftRow + (controlID - kStoreShelfFirstID);
		std::vector<store_entry>& shelf = fStore->Items();
		if (index >= shelf.size()
				|| !(fStore->Actions(shelf[index].item, false) & STORE_ACT_BUY))
			return true;
		shelf[index].selected = !shelf[index].selected;
		shelf[index].purchased = shelf[index].selected ? 1 : 0;
		_UpdateWindow();
	} else if (controlID >= kStoreOwnFirstID && controlID < kStoreOwnFirstID + kStoreSlots) {
		std::vector<ScreenSupport::LootEntry> own;
		ScreenSupport::CollectOwnEntries(customer, own);
		size_t index = (size_t)fRightRow + (controlID - kStoreOwnFirstID);
		if (index >= own.size()
				|| !(fStore->Actions(own[index].item, true) & STORE_ACT_SELL))
			return true;
		uint32 slot = (uint32)own[index].slot;
		if (!fSellSlots.erase(slot))
			fSellSlots.insert(slot);
		_UpdateWindow();
	}
	return true;
}


/* virtual */
bool
StoreScreen::ControlDoubleClicked(uint16 windowID, uint32 controlID)
{
	if (!IsOpen() || fAmountIndex >= 0 || windowID != kStoreShopWindow
			|| Core::Get()->Game() != game::GAME_BALDURSGATE2)
		return false;
	if (controlID < kStoreShelfFirstID || controlID >= kStoreShelfFirstID + kStoreSlots)
		return false;

	size_t index = (size_t)fLeftRow + (controlID - kStoreShelfFirstID);
	std::vector<store_entry>& shelf = fStore->Items();
	if (index >= shelf.size() || !(fStore->Actions(shelf[index].item, false) & STORE_ACT_BUY))
		return false;
	_OpenAmountWindow(index);
	return true;
}


void
StoreScreen::_OpenAmountWindow(size_t shelfIndex)
{
	const store_entry& entry = fStore->Items()[shelfIndex];
	fAmountIndex = (int32)shelfIndex;
	fAmountMax = entry.amount < 0 ? kStoreAmountInfiniteMax : (uint32)entry.amount;
	fAmountValue = std::min(std::max<uint32>(entry.purchased, 1), fAmountMax);

	GUI* gui = GUI::Get();
	gui->ShowAuxWindow(CHUName(), kStoreAmountWindow);
	Window* window = GetWindow(kStoreAmountWindow);
	if (window == NULL) {
		fAmountIndex = -1;
		return;
	}

	if (Button* icon = dynamic_cast<Button*>(window->GetControlByID(kStoreAmountIconID)))
		icon->SetIcon(ScreenSupport::MakeItemIcon(entry.item.name));
	if (Label* name = dynamic_cast<Label*>(window->GetControlByID(kStoreAmountNameLabelID)))
		name->SetText(ScreenSupport::ItemDisplayName(entry.item.name));
	if (Button* cancel = dynamic_cast<Button*>(window->GetControlByID(kStoreAmountCancelID)))
		cancel->SetText(IDTable::GetDialog(kStoreCancelStrRef));
	if (Button* done = dynamic_cast<Button*>(window->GetControlByID(kStoreAmountDoneID)))
		done->SetText(IDTable::GetDialog(kStoreDoneStrRef));
	if (Control* box = window->GetControlByID(kStoreAmountBoxID)) {
		if (window->GetControlByID(kStoreAmountValueLabelID) == NULL) {
			GFX::rect rect = box->Frame();
			ScreenSupport::AddLabel(window, kStoreAmountValueLabelID, rect.x, rect.y, rect.w, rect.h,
				IE::LABEL_JUSTIFY_CENTER);
		}
	}
	_UpdateAmountWindow();
}


// Closes the picker; with `apply`, the chosen amount becomes the item's
// purchase quantity (0 unselects it).
void
StoreScreen::_CloseAmountWindow(bool apply)
{
	if (fAmountIndex < 0)
		return;
	if (apply && (size_t)fAmountIndex < fStore->Items().size()) {
		store_entry& entry = fStore->Items()[fAmountIndex];
		entry.purchased = fAmountValue;
		entry.selected = fAmountValue > 0;
	}
	fAmountIndex = -1;
	if (GUI* gui = GUI::Get())
		gui->HideAuxWindow(CHUName(), kStoreAmountWindow);
	_UpdateWindow();
}


void
StoreScreen::_UpdateAmountWindow()
{
	if (fAmountIndex < 0)
		return;
	Window* window = GetWindow(kStoreAmountWindow);
	if (window == NULL)
		return;
	if (Label* value = dynamic_cast<Label*>(window->GetControlByID(kStoreAmountValueLabelID)))
		value->SetText(std::to_string(fAmountValue));
}


/* virtual */
bool
StoreScreen::ControlHovered(uint16 windowID, uint32 controlID, bool inside)
{
	if (!inside || !IsOpen() || windowID != kStoreShopWindow
			|| fAmountIndex >= 0) {
		GUI::Get()->SetHoverTooltip("");
		return true;
	}
	std::string name;
	if (controlID >= kStoreShelfFirstID && controlID < kStoreShelfFirstID + kStoreSlots) {
		size_t index = (size_t)fLeftRow + (controlID - kStoreShelfFirstID);
		std::vector<store_entry>& shelf = fStore->Items();
		if (index < shelf.size())
			name = ScreenSupport::ItemDisplayName(shelf[index].item.name);
	} else if (controlID >= kStoreOwnFirstID && controlID < kStoreOwnFirstID + kStoreSlots) {
		Actor* customer = fGame.ShownActor();
		std::vector<ScreenSupport::LootEntry> own;
		if (customer != NULL)
			ScreenSupport::CollectOwnEntries(customer, own);
		size_t index = (size_t)fRightRow + (controlID - kStoreOwnFirstID);
		if (index < own.size())
			name = ScreenSupport::ItemDisplayName(own[index].item.name);
	}
	GUI::Get()->SetHoverTooltip(name);
	return true;
}


StoreScreen::StoreScreen(Game& game)
	:
	GameScreen(game, "GUISTORE", { kStoreBarWindow, 0, 1 }),
	fStore(NULL),
	fCustomer(NULL),
	fLeftRow(0),
	fRightRow(0),
	fIdentifyRow(0),
	fUnpause(false),
	fAmountIndex(-1),
	fAmountValue(0),
	fAmountMax(0),
	fPage(-1)
{
}


StoreScreen::~StoreScreen()
{
	ClearStores();
}
