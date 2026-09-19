/*
 * Store.h
 *
 * A store's live state (its stock changes as the party buys and sells) and
 * the buy/sell rules and pricing. The rules follow GemRB's Store.cpp and
 * GUISTORE.py (GetRealPrice()), which is what the real games' behavior was
 * checked against.
 */

#ifndef STORE_H_
#define STORE_H_

#include "IETypes.h"

#include <vector>

class Actor;

// Bit flags describing what the party may do with an item, from the party's
// point of view (same values GemRB's IsValidStoreItem() returns).
enum store_actions {
	STORE_ACT_BUY = 1,
	STORE_ACT_SELL = 2,
	STORE_ACT_IDENTIFY = 4,
	STORE_ACT_STEAL = 8
};

// Inventory-slot style item flags the store rules are written against: the
// CRE item flags, plus the ITM header's own flags shifted up a byte, plus a
// few derived ones (see Store::SlotFlags()).
enum store_item_flags {
	STORE_ITEM_IDENTIFIED = 0x1,
	STORE_ITEM_UNSTEALABLE = 0x2,
	STORE_ITEM_STOLEN = 0x4,
	STORE_ITEM_UNDROPPABLE = 0x8,
	STORE_ITEM_DESTRUCTIBLE = 0x20,
	STORE_ITEM_CRITICAL = 0x100,
	STORE_ITEM_MOVABLE = 0x400
};

// One line of a store's stock.
struct store_entry {
	IE::item item;		// quantity1..3 = the pack's stack size/charges
	int32 amount;		// how many packs are in stock; -1 = infinite
	bool selected;		// marked for purchase in the shopping window
	uint32 purchased;	// how many packs
};

class Store {
public:
	// NULL if there is no such store resource.
	static Store* Load(const res_ref& name);

	const res_ref& Name() const { return fName; }
	uint32 Type() const { return fType; }
	uint32 NameRef() const { return fNameRef; }
	uint32 Flags() const { return fFlags; }
	// Can the party shop here at all (buy and/or sell)?
	bool IsShop() const;
	// Gold per item the store identifies.
	uint32 IdentifyPrice() const { return fIdentifyPrice; }

	std::vector<store_entry>& Items() { return fItems; }

	// The inventory-slot style flags for `item` (see store_item_flags).
	static uint32 SlotFlags(const IE::item& item);

	// What the party may do with `item` (a combination of store_actions).
	// `fromParty` = it's in a party member's inventory, else it's on the
	// store's own shelf.
	int Actions(const IE::item& item, bool fromParty) const;

	// Gold the party pays for one pack of `entry`.
	int32 PriceToBuy(const store_entry& entry, Actor* haggler) const;
	// Gold the store pays for `item`.
	int32 PriceToSell(const IE::item& item, Actor* haggler) const;

	// Moves `entry.purchased` packs of item `index` to `buyer`'s inventory,
	// updating the stock. False (nothing changes) if they don't fit.
	bool Buy(size_t index, Actor* buyer);
	// Takes `item` (already removed from the party) into the stock.
	void Accept(const IE::item& item);
	// The store has no room for more kinds of item.
	bool IsFull() const;

private:
	Store(const res_ref& name);
	int32 _StockOf(const res_ref& itemName) const;

	res_ref fName;
	uint32 fType;
	uint32 fNameRef;
	uint32 fFlags;
	uint32 fSellMarkup;
	uint32 fBuyMarkup;
	uint32 fDepreciation;
	uint32 fCapacity;
	uint32 fIdentifyPrice;
	std::vector<uint32> fPurchasedCategories;
	std::vector<store_entry> fItems;
};

#endif // STORE_H_
