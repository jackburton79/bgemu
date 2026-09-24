#include "ScreenSupport.h"

#include "BamResource.h"
#include "Bitmap.h"
#include "ResManager.h"
#include "SPLResource.h"

#include "Actor.h"
#include "CreResource.h"
#include "ITMResource.h"
#include "Label.h"
#include "Window.h"


Bitmap*
ScreenSupport::MakeSpellIcon(const res_ref& spellName)
{
	SPLResource* spl = gResManager->GetSPL(spellName);
	if (spl == NULL)
		return NULL;
	Bitmap* icon = NULL;
	BAMResource* bam = gResManager->GetBAM(spl->BookIcon());
	if (bam != NULL) {
		icon = bam->FrameForCycle(0, 0);
		gResManager->ReleaseResource(bam);
	}
	gResManager->ReleaseResource(spl);
	return icon;
}


// Builds the inventory icon (cycle 0 / frame 0 of the ITM's inventory-icon
// BAM, same convention Button uses for its own CHU bitmaps) for an item
// resref. Returns a new reference the caller owns, or NULL.
Bitmap*
ScreenSupport::MakeItemIcon(const res_ref& itemName)
{
	ITMResource* itm = gResManager->GetITM(itemName);
	if (itm == NULL)
		return NULL;
	Bitmap* icon = NULL;
	// Some items (e.g. an unresolved RNDTRE* random-treasure placeholder)
	// have no inventory icon at all.
	if (itm->InventoryIcon().name[0] != '\0') {
		BAMResource* bam = gResManager->GetBAM(itm->InventoryIcon());
		if (bam != NULL) {
			icon = bam->FrameForCycle(0, 0);
			gResManager->ReleaseResource(bam);
		}
	}
	gResManager->ReleaseResource(itm);
	return icon;
}


// Human-readable name for an item: its identified name, else its
// unidentified name, else the bare resref.
std::string
ScreenSupport::ItemDisplayName(ITMResource* itm, const res_ref& itemName)
{
	if (itm != NULL) {
		std::string name = IDTable::GetDialog(itm->IdentifiedNameRef());
		if (name.empty())
			name = IDTable::GetDialog(itm->UnidentifiedNameRef());
		if (!name.empty())
			return name;
	}
	return itemName.CString();
}


// Same as above, resolving the ITM resource itself from the resref -
// for call sites (inventory drag/drop logging) that only have the
// resref, not an already-loaded ITMResource*.
std::string
ScreenSupport::ItemDisplayName(const res_ref& itemName)
{
	ITMResource* itm = gResManager->GetITM(itemName);
	std::string name = ItemDisplayName(itm, itemName);
	if (itm != NULL)
		gResManager->ReleaseResource(itm);
	return name;
}


// Sum of every item the CRE carries (equipped or not - every one of its
// 40 slots), stack count included for stackable items (e.g. a quiver of
// 20 arrows counts as 20, not 1) - matches GemRB's Inventory::
// CalculateWeight().
uint32
ScreenSupport::CarriedWeight(CREResource* cre)
{
	uint32 weight = 0;
	for (uint32 i = 0; i < kNumItemSlots; i++) {
		IE::item item;
		if (!cre->GetItemAtSlot(i, item))
			continue;
		ITMResource* itm = gResManager->GetITM(item.name);
		if (itm == nullptr)
			continue;
		uint32 count = (item.quantity1 != 0 && itm->StackAmount() != 0)
				? item.quantity1 : 1;
		weight += itm->Weight() * count;
		gResManager->ReleaseResource(itm);
	}
	return weight;
}


// STR-based carry capacity: STRMOD.2DA's WEIGHT_ALLOWANCE column (row =
// STR score), plus the exceptional-strength (18/xx) bonus from
// STRMODEX.2DA (row = the percentile extra) when STR is exactly 18 -
// same two tables and the same STR==18 special case GemRB's own
// GetMaxEncumbrance()/GetStrengthBonus() read. Real data still ships
// both as loadable 2DAs in this install (unlike avatars.2da - see
// AnimationFactory.cpp's own note on that one).
uint32
ScreenSupport::MaxEncumbrance(CREResource* cre)
{
	return (uint32)Actor::StrengthBonus(cre, 3);
}


// A label that isn't CHU-authored (see kWeightCurrentLabelID's own
// comment), created at runtime in `window`. The underlying IE::label struct
// is heap-allocated the same way CHUIResource::_ReadControl() allocates
// every other control's, since Control::~Control() unconditionally frees it
// the same way. Same font as the CHU-authored AC/HP labels (confirmed by
// dumping their real font_bam - GemRB's own "NUMBER" is an engine-internal
// font id, not a real BAM resref).
void
ScreenSupport::AddLabel(Window* window, uint32 id, sint16 x, sint16 y, uint16 width,
	uint16 height, uint16 flags)
{
	IE::label* label = (IE::label*)new uint8[sizeof(IE::label)];
	label->id = id;
	label->x = x;
	label->y = y;
	label->w = width;
	label->h = height;
	label->type = IE::CONTROL_LABEL;
	label->unk = 0;
	label->text_ref = 0xffffffff;
	label->font_bam = res_ref("STONESML");
	label->color1_r = label->color1_g = label->color1_b = 255;
	label->color1_a = 0;
	label->color2_r = label->color2_g = label->color2_b = label->color2_a = 0;
	label->flags = flags;
	window->Add(new Label(label));
}


// The two weight labels aren't CHU-authored (see kWeightCurrentLabelID's
// own comment) - create them once, the first time this window instance is
// refreshed, anchored to the bag icon's rect exactly like GemRB's
// Window.CreateLabel() calls do (top-left for current weight, bottom-right
// for max). The underlying IE::label struct is heap-allocated the same way
// CHUIResource::_ReadControl() allocates every other control's, since
// Control::~Control() unconditionally frees it the same way.
void
ScreenSupport::EnsureWeightLabels(Window* window, uint32 iconID)
{
	if (window->GetControlByID(kWeightCurrentLabelID) != nullptr)
		return;

	Control* bagIcon = window->GetControlByID(iconID);
	if (bagIcon == nullptr)
		return;
	GFX::rect rect = bagIcon->Frame();

	AddLabel(window, kWeightCurrentLabelID, rect.x, rect.y, rect.w, 20,
		IE::LABEL_JUSTIFY_LEFT | IE::LABEL_JUSTIFY_TOP);
	AddLabel(window, kWeightMaxLabelID, rect.x, (sint16)(rect.y + rect.h - 20),
		rect.w, 20, IE::LABEL_JUSTIFY_RIGHT | IE::LABEL_JUSTIFY_BOTTOM);
}
