/*
 * Store.cpp
 */

#include "Store.h"

#include "2DAResource.h"
#include "Actor.h"
#include "CreResource.h"
#include "ITMResource.h"
#include "ResManager.h"
#include "STOResource.h"

#include <algorithm>


// Store flag bits the rules below read that STOResource.h doesn't name:
// "fence" (buys stolen goods), "no reputation adjustment" and "buys critical
// items too" - see GemRB's Store.h.
static const uint32 kStoreFence = 0x1000;
static const uint32 kStoreNoRepAdjust = 0x2000;
static const uint32 kStoreBuysCriticals = 0x8000;
// Store types below this are ordinary stores; the container-like ones don't
// apply the "only sell what the store buys" restrictions.
static const uint32 kFirstContainerType = 4;

// ITM item types (itm_v1.htm) that don't lose value when the store already
// holds copies: jewelry.
static const uint16 kItemTypeAmulet = 0x01;
static const uint16 kItemTypeRing = 0x0a;
static const uint16 kItemTypeGem = 0x22;

static const uint32 kItemFlagMovable = 0x4;
static const uint32 kItemFlagStolen = 0x400;


Store::Store(const res_ref& name)
	:
	fName(name),
	fType(0),
	fNameRef(0),
	fFlags(0),
	fSellMarkup(100),
	fBuyMarkup(100),
	fDepreciation(0),
	fCapacity(0),
	fIdentifyPrice(0)
{
}


/* static */
Store*
Store::Load(const res_ref& name)
{
	STOResource* sto = gResManager->GetSTO(name);
	if (sto == NULL)
		return NULL;

	Store* store = new Store(name);
	store->fType = sto->StoreType();
	store->fNameRef = sto->NameRef();
	store->fFlags = sto->Flags();
	store->fSellMarkup = sto->SellMarkup();
	store->fBuyMarkup = sto->BuyMarkup();
	store->fDepreciation = sto->Depreciation();
	store->fCapacity = sto->Capacity();
	store->fIdentifyPrice = sto->IdentifyPrice();
	store->fPurchasedCategories = sto->ItemCategoriesPurchased();
	for (const sto_item& stoItem : sto->ItemsForSale()) {
		store_entry entry;
		entry.item.name = stoItem.itemName;
		entry.item.expiration_time = 0;
		entry.item.expiration_time2 = 0;
		entry.item.quantity1 = stoItem.charges1;
		entry.item.quantity2 = stoItem.charges2;
		entry.item.quantity3 = stoItem.charges3;
		entry.item.flags = stoItem.flags;
		entry.amount = stoItem.infiniteSupply != 0 ? -1 : (int32)stoItem.amount;
		entry.selected = false;
		entry.purchased = 0;
		store->fItems.push_back(entry);
	}
	gResManager->ReleaseResource(sto);
	return store;
}


bool
Store::IsShop() const
{
	return (fFlags & (STO_CAN_BUY | STO_CAN_SELL)) != 0 && fType != STORE_CONTAINER;
}


/* static */
uint32
Store::SlotFlags(const IE::item& item)
{
	uint32 flags = item.flags;
	ITMResource* itm = gResManager->GetITM(item.name);
	if (itm == NULL)
		return flags;

	flags |= itm->Flags() << 8;
	if (!(flags & STORE_ITEM_CRITICAL))
		flags |= STORE_ITEM_DESTRUCTIBLE;
	if (itm->Flags() & kItemFlagStolen)
		flags |= STORE_ITEM_STOLEN;
	if (itm->LoreToIdentify() == 0)
		flags |= STORE_ITEM_IDENTIFIED;
	if (!(itm->Flags() & kItemFlagMovable))
		flags |= STORE_ITEM_UNDROPPABLE;
	gResManager->ReleaseResource(itm);
	return flags;
}


int
Store::Actions(const IE::item& item, bool fromParty) const
{
	// A party member's item carries its ITM header flags too; a store's
	// shelf entry only has the flags its STO file stores for it.
	const uint32 flags = fromParty ? SlotFlags(item) : item.flags;

	int actions = 0;
	if (!(flags & STORE_ITEM_UNDROPPABLE))
		actions = STORE_ACT_BUY | STORE_ACT_SELL | STORE_ACT_STEAL;
	if (flags & STORE_ITEM_UNSTEALABLE)
		actions &= ~STORE_ACT_STEAL;
	if (!(flags & STORE_ITEM_IDENTIFIED))
		actions |= STORE_ACT_IDENTIFY;

	// Nothing to buy or sell if the store doesn't do that at all.
	if (!(fFlags & STO_CAN_SELL))
		actions &= ~STORE_ACT_SELL;
	if (!(fFlags & STO_CAN_BUY))
		actions &= ~STORE_ACT_BUY;

	if (fromParty && fType < kFirstContainerType) {
		// Only removable items can be sold; critical ones only to a store
		// that says so; stolen ones only to a fence.
		if (!(flags & STORE_ITEM_DESTRUCTIBLE))
			actions &= ~STORE_ACT_SELL;
		if ((flags & STORE_ITEM_CRITICAL) && !(fFlags & kStoreBuysCriticals))
			actions &= ~STORE_ACT_SELL;
		if ((flags & STORE_ITEM_STOLEN) && !(fFlags & kStoreFence))
			actions &= ~STORE_ACT_SELL;
	}

	if (!fromParty)
		return actions;

	// The store only takes the kinds of item it lists (it can still
	// identify others).
	uint16 itemType = 0;
	if (ITMResource* itm = gResManager->GetITM(item.name)) {
		itemType = itm->ItemType();
		gResManager->ReleaseResource(itm);
	}
	if (std::find(fPurchasedCategories.begin(), fPurchasedCategories.end(),
			(uint32)itemType) == fPurchasedCategories.end())
		actions &= ~STORE_ACT_SELL;
	return actions;
}


// How many packs of `itemName` the store holds (0 if it has none or has
// an infinite supply) - what depreciation is counted against.
int32
Store::_StockOf(const res_ref& itemName) const
{
	for (const store_entry& entry : fItems) {
		if (entry.item.name == itemName)
			return entry.amount > 0 ? entry.amount : 0;
	}
	return 0;
}


// Base price of `item`, scaled by its pack size or remaining charges.
static int32
_BasePrice(const IE::item& item, ITMResource* itm)
{
	int64 price = itm->Price();
	if (itm->StackAmount() > 1)
		price *= item.quantity1;
	else if (itm->MaxCharges() > 0)
		price = price * item.quantity1 / itm->MaxCharges();
	return (int32)price;
}


int32
Store::PriceToBuy(const store_entry& entry, Actor* haggler) const
{
	ITMResource* itm = gResManager->GetITM(entry.item.name);
	if (itm == NULL)
		return 0;

	int64 modifier = fSellMarkup;
	const int32 basePrice = _BasePrice(entry.item, itm);
	const uint32 itemPrice = itm->Price();
	gResManager->ReleaseResource(itm);

	if (haggler != NULL && haggler->CRE() != NULL) {
		// Charisma discount: CHRMODST.2DA's one row, column = charisma - 1.
		BaseAttributes attributes;
		haggler->CRE()->GetAttributes(attributes);
		if (TWODAResource* table = gResManager->Get2DA("CHRMODST")) {
			int32 column = std::max<int32>(0,
				std::min<int32>(attributes.charisma - 1, table->CountColumns() - 1));
			modifier += table->IntegerValueAt(0, column);
			gResManager->ReleaseResource(table);
		}

		// Reputation: REPMODST.2DA's one row (percent), column = reputation
		// - 1, unless this store ignores reputation.
		if (!(fFlags & kStoreNoRepAdjust)) {
			if (TWODAResource* table = gResManager->Get2DA("REPMODST")) {
				int32 column = std::max<int32>(0,
					std::min<int32>(haggler->CRE()->Reputation() - 1,
						table->CountColumns() - 1));
				modifier = modifier * table->IntegerValueAt(0, column) / 100;
				gResManager->ReleaseResource(table);
			}
		}
	}

	int32 price = (int32)(basePrice * modifier / 100);
	// Even a 1 gp item never comes free.
	if (price == 0 && itemPrice > 0)
		price = 1;
	return price;
}


int32
Store::PriceToSell(const IE::item& item, Actor* /* haggler */) const
{
	ITMResource* itm = gResManager->GetITM(item.name);
	if (itm == NULL)
		return 0;

	int64 modifier = fBuyMarkup;
	// Every copy the store already holds, up to two, cuts what it pays -
	// except for jewelry, which keeps its value.
	int32 count = _StockOf(item.name);
	const uint16 type = itm->ItemType();
	if (type == kItemTypeAmulet || type == kItemTypeRing || type == kItemTypeGem)
		count = 0;
	modifier -= (int64)std::min<int32>(count, 2) * fDepreciation;

	const int32 basePrice = _BasePrice(item, itm);
	const uint32 itemPrice = itm->Price();
	gResManager->ReleaseResource(itm);

	int32 price = (int32)(basePrice * modifier / 100);
	if (price == 0 && itemPrice > 0)
		price = 1;
	return price;
}


bool
Store::Buy(size_t index, Actor* buyer)
{
	if (index >= fItems.size() || buyer == NULL)
		return false;
	store_entry& entry = fItems[index];
	const uint32 packs = std::max<uint32>(entry.purchased, 1);
	if (entry.amount >= 0 && (uint32)entry.amount < packs)
		return false;

	for (uint32 i = 0; i < packs; i++) {
		if (!buyer->AddItem(entry.item)) {
			// Some packs may already have gone across: keep the stock in
			// step with what the buyer actually got.
			if (entry.amount >= 0)
				entry.amount -= i;
			entry.selected = false;
			entry.purchased = 0;
			if (entry.amount == 0)
				fItems.erase(fItems.begin() + index);
			return false;
		}
	}

	if (entry.amount >= 0)
		entry.amount -= packs;
	if (entry.amount == 0)
		fItems.erase(fItems.begin() + index);
	else {
		entry.selected = false;
		entry.purchased = 0;
	}
	return true;
}


void
Store::Accept(const IE::item& item)
{
	IE::item shelved = item;
	shelved.flags |= STORE_ITEM_IDENTIFIED;

	// An item with neither a stack size nor charges has no quantity worth
	// keeping apart (a party member's copy says 1, a shelf entry says 0).
	bool hasQuantity = false;
	if (ITMResource* itm = gResManager->GetITM(shelved.name)) {
		hasQuantity = itm->StackAmount() > 1 || itm->MaxCharges() > 0;
		gResManager->ReleaseResource(itm);
	}
	if (!hasQuantity)
		shelved.quantity1 = shelved.quantity2 = shelved.quantity3 = 0;

	for (store_entry& entry : fItems) {
		if (entry.item.name == shelved.name && entry.amount >= 0
				&& entry.item.quantity1 == shelved.quantity1
				&& entry.item.quantity2 == shelved.quantity2
				&& entry.item.quantity3 == shelved.quantity3) {
			entry.amount++;
			return;
		}
	}

	store_entry entry;
	entry.item = shelved;
	entry.amount = 1;
	entry.selected = false;
	entry.purchased = 0;
	fItems.push_back(entry);
}


bool
Store::IsFull() const
{
	return fCapacity != 0 && fItems.size() >= fCapacity;
}
