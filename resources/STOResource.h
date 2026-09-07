/*
 * STOResource.h
 *
 * Reads the STO (store) format - see IESDP sto_v1.htm.
 *
 * Covers what a store fundamentally is - its type/flags/price markups and
 * the two item lists (what it sells, what categories it buys) - which is
 * what STARTSTORE needs. The tavern/inn/temple-specific extras (drinks,
 * cures, room prices, rumours) aren't parsed yet - no code anywhere in
 * this engine reads them, and they only matter once a real store GUI
 * screen exists to show them; add them then.
 */

#ifndef STORESOURCE_H_
#define STORESOURCE_H_

#include "Resource.h"

#include <vector>

enum sto_type {
	STORE_STORE = 0,
	STORE_TAVERN = 1,
	STORE_INN = 2,
	STORE_TEMPLE = 3,
	STORE_CONTAINER = 5
};

enum sto_flags {
	STO_CAN_BUY = 1 << 0,		// user allowed to buy
	STO_CAN_SELL = 1 << 1,		// user allowed to sell
	STO_CAN_IDENTIFY = 1 << 2,
	STO_CAN_STEAL = 1 << 3,
	STO_CAN_DONATE = 1 << 4,
	STO_CAN_BUY_CURES = 1 << 5,
	STO_CAN_BUY_DRINKS = 1 << 6
};

// One "Items for Sale" entry (0x1c bytes) - see IESDP sto_v1.htm.
struct sto_item {
	res_ref itemName;
	uint16 expirationTime;
	uint16 charges1;
	uint16 charges2;
	uint16 charges3;
	uint32 flags;
	uint32 amount;
	uint32 infiniteSupply;	// 0 = limited stock, 1 = infinite
};

class STOResource : public Resource {
public:
	STOResource(const res_ref& name);
	static Resource* Create(const res_ref& name);

	virtual bool Load(Archive* archive, uint32 key);

	uint32 StoreType() const;
	uint32 NameRef() const;
	uint32 Flags() const;
	uint32 SellMarkup() const;	// % of base price the store charges
	uint32 BuyMarkup() const;	// % of base price the store pays
	uint32 Capacity() const;	// 0 = unlimited

	std::vector<sto_item> ItemsForSale() const;
	// Item category codes (see IESDP sto_v1.htm's "Item category codes"
	// table) this store is willing to buy from the player.
	std::vector<uint32> ItemCategoriesPurchased() const;

private:
	virtual ~STOResource();

	uint32 fItemsForSaleOffset;
	uint32 fItemsForSaleCount;
	uint32 fItemsPurchasedOffset;
	uint32 fItemsPurchasedCount;
};

#endif // STORESOURCE_H_
