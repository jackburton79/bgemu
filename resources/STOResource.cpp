/*
 * STOResource.cpp
 */

#include "STOResource.h"

#include "Stream.h"

#define STO_SIGNATURE "STOR"
#define STO_VERSION_1 "V1.0"

static const uint32 kItemSize = 0x1c;


/* static */
Resource*
STOResource::Create(const res_ref& name)
{
	return new STOResource(name);
}


STOResource::STOResource(const res_ref& name)
	:
	Resource(name, RES_STO)
{
}


STOResource::~STOResource()
{
}


/* virtual */
bool
STOResource::Load(Archive* archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;

	if (!CheckSignature(STO_SIGNATURE) || !CheckVersion(STO_VERSION_1))
		return false;

	fData->ReadAt(0x2c, fItemsPurchasedOffset);
	fData->ReadAt(0x30, fItemsPurchasedCount);
	fData->ReadAt(0x34, fItemsForSaleOffset);
	fData->ReadAt(0x38, fItemsForSaleCount);

	return true;
}


uint32
STOResource::StoreType() const
{
	uint32 type;
	fData->ReadAt(0x08, type);
	return type;
}


uint32
STOResource::NameRef() const
{
	uint32 nameRef;
	fData->ReadAt(0x0c, nameRef);
	return nameRef;
}


uint32
STOResource::Flags() const
{
	uint32 flags;
	fData->ReadAt(0x10, flags);
	return flags;
}


uint32
STOResource::SellMarkup() const
{
	uint32 markup;
	fData->ReadAt(0x14, markup);
	return markup;
}


uint32
STOResource::BuyMarkup() const
{
	uint32 markup;
	fData->ReadAt(0x18, markup);
	return markup;
}


uint32
STOResource::Capacity() const
{
	uint16 capacity;
	fData->ReadAt(0x22, capacity);
	return capacity;
}


std::vector<sto_item>
STOResource::ItemsForSale() const
{
	std::vector<sto_item> items;
	for (uint32 i = 0; i < fItemsForSaleCount; i++) {
		const uint32 offset = fItemsForSaleOffset + i * kItemSize;

		sto_item item;
		fData->ReadAt(offset + 0x00, item.itemName);
		fData->ReadAt(offset + 0x08, item.expirationTime);
		fData->ReadAt(offset + 0x0a, item.charges1);
		fData->ReadAt(offset + 0x0c, item.charges2);
		fData->ReadAt(offset + 0x0e, item.charges3);
		fData->ReadAt(offset + 0x10, item.flags);
		fData->ReadAt(offset + 0x14, item.amount);
		fData->ReadAt(offset + 0x18, item.infiniteSupply);

		items.push_back(item);
	}

	return items;
}


std::vector<uint32>
STOResource::ItemCategoriesPurchased() const
{
	std::vector<uint32> categories;
	for (uint32 i = 0; i < fItemsPurchasedCount; i++) {
		uint32 category;
		fData->ReadAt(fItemsPurchasedOffset + i * sizeof(category), category);
		categories.push_back(category);
	}

	return categories;
}
