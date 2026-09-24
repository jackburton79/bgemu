#pragma once

#include "GameScreen.h"

#include <map>
#include <set>
#include <string>
#include <vector>

class Actor;
class Store;


// The store (GUISTORE): opened by the STARTSTORE action for a party member
// shopping, not by a key. Two pages - Buy/Sell (window 2) and Identify
// (window 4) - under a bottom bar (window 3) with the page tabs and "Done",
// the side columns (0, decoration, and 1, who is shopping), and on BG2 a
// quantity picker (window 16). The game stands still while it is open. It
// keeps every store it has met, so what was bought or sold stays that way for
// the session (not written to savegames).
class StoreScreen : public GameScreen {
public:
	StoreScreen(Game& game);
	virtual ~StoreScreen();

	// Opens the store for `customer`; false if it doesn't exist or isn't a
	// shop (taverns, inns and temples have pages this window doesn't have
	// yet).
	bool OpenStore(Actor* customer, const res_ref& storeName);

	virtual bool IsOpen() const;
	virtual void Refresh();
	virtual void OnShownCharacterChanged();
	virtual bool ControlInvoked(uint16 windowID, uint32 controlID);
	virtual bool ControlDoubleClicked(uint16 windowID, uint32 controlID);
	virtual bool ControlHovered(uint16 windowID, uint32 controlID, bool inside);

	// A store met earlier this session, NULL if none.
	Store* LoadedStore(const char* name) const;
	// Forgets every store (a loaded game starts from the stores' original
	// stock).
	void ClearStores();

protected:
	virtual void OnClose();

private:
	void _UpdateWindow();
	void _UpdateShopPage();
	void _UpdateIdentifyPage();
	void _IdentifySelected();
	void _BuySelected();
	void _SellSelected();
	void _SetupTabs();
	void _ShowPage(int32 page);
	void _OpenAmountWindow(size_t shelfIndex);
	void _CloseAmountWindow(bool apply);
	void _UpdateAmountWindow();

	// Owns every Store.
	std::map<std::string, Store*> fStores;
	Store* fStore;
	// Whose turn at the counter it is; their selection starts blank when it
	// changes.
	Actor* fCustomer;
	// The customer's item slots picked to sell / to identify.
	std::set<uint32> fSellSlots;
	std::set<uint32> fIdentifySlots;
	int32 fLeftRow;
	int32 fRightRow;
	int32 fIdentifyRow;
	// Whether the game was paused by this window, and so is unpaused by it.
	bool fUnpause;
	// The shelf item the quantity picker is choosing an amount of (-1 =
	// picker closed), the amount and its cap.
	int32 fAmountIndex;
	uint32 fAmountValue;
	uint32 fAmountMax;
	// The page shown (-1 = none) and what each tab button offers.
	int32 fPage;
	std::vector<int32> fTabs;
};
