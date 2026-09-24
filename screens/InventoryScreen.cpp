#include "InventoryScreen.h"

#include "Actor.h"
#include "AnimationFactory.h"
#include "AreaRoom.h"
#include "BamResource.h"
#include "Bitmap.h"
#include "Button.h"
#include "Core.h"
#include "CreResource.h"
#include "Game.h"
#include "GUI.h"
#include "ITMResource.h"
#include "Label.h"
#include "PLTResource.h"
#include "ResManager.h"
#include "ScreenSupport.h"
#include "TextArea.h"
#include "Window.h"

#include <iostream>
#include <string>
#include <vector>

// GUIINV.CHU control IDs used by the label-population methods below, named
// rather than left as bare literals at each call site - all
// identified/confirmed as described in those methods' own comments.
static const uint32 kInvNameLabelID = 268435506;
static const uint32 kInvClassLabelID = 268435522;
static const uint32 kInvACLabelID = 268435512;
// Confirmed against GemRB's own GUIINV.py (bg1/bg2, identical control
// IDs in both): current/max hit points and the party gold counter -
// none of the three are CHU-authored static text, all three are set
// from code every time the window refreshes, same as name/AC above.
static const uint32 kInvHPCurrentLabelID = 268435513;
static const uint32 kInvHPMaxLabelID = 268435514;
static const uint32 kInvGoldLabelID = 268435520;
// The paperdoll control itself (128x160, CHU-authored to a fixed
// placeholder bitmap - CIFF4INV, a generic doll unrelated to whichever
// character's inventory is actually open) - see _UpdatePaperdoll().
static const uint32 kInvPaperdollID = 50;
// GUIINV window 5 (background GUIINVHI) is BG2's "examine item" popup,
// opened by right-clicking a slot (confirmed via a real GUIINV.CHU dump):
// id 268435455 = item title, id 7 = the large item-icon button, id 5 =
// the scrollable description text_area. Any button in the window closes
// it (only "Done", id 4, is meaningful here).
static const uint32 kInvInfoTitleID = 268435455;
static const uint32 kInvInfoIconID = 7;
static const uint32 kInvInfoTextID = 5;
// Two CHU-authored labels in that window that otherwise show a literal
// "(No text)" TLK placeholder - blanked rather than left visible (real
// BG2 fills them from code; their exact purpose isn't confirmed here).
static const uint32 kInvInfoBlankLabel1ID = 268435456;
static const uint32 kInvInfoBlankLabel2ID = 268435467;


InventoryScreen::InventoryScreen(Game& game)
	:
	PanelScreen(game, "GUIINV"),
	fDragSlot(-1)
{
}


/* virtual */
uint32
InventoryScreen::CommandBarButton() const
{
	return 3;
}


void
InventoryScreen::_ClearDrag()
{
	fDragSlot = -1;
	GUI::Get()->SetDragBitmap(NULL);
}


/* virtual */
void
InventoryScreen::OnOpen()
{
	_ClearDrag();
}


// The examine popup goes with the screen.
/* virtual */
void
InventoryScreen::OnClose()
{
	_ClearDrag();
	GUI::Get()->HideAuxWindow(CHUName(), kInfoWindow);
}


// Navigating away from the inventory through the command bar copy in
// window 0 drops the held item, too.
/* virtual */
bool
InventoryScreen::ControlInvoked(uint16 windowID, uint32 controlID)
{
	if (windowID == kCommandBarWindow)
		_ClearDrag();
	return PanelScreen::ControlInvoked(windowID, controlID);
}


// A click on the window that didn't land on any control drops the held
// item on the floor.
/* virtual */
bool
InventoryScreen::BackgroundClicked(uint16 /*windowID*/)
{
	_DropHeldItemOnGround();
	return true;
}


// GUIINV window 2 slot-control id -> CRE item-slot index. Both the icon
// refresh (_UpdateInventoryIcons) and the drag/drop click handler
// (InventoryControlInvoked) walk this one table. How each group was
// identified:
//  - general grid (ids 30/32/.../44 then 31/33/.../45): row-major reading
//    order, confirmed against a real GUIINV.CHU dump to be
//    kSlotGeneralFirst..Last (16 slots) in order.
//  - row above the paperdoll (ids 11-14): Armor/Gauntlets/Helmet/Shield,
//    icon-verified on a real character wearing all four.
//  - "Armi rapide" (ids 1-4): Weapon1-4, by count + the same ascending
//    id->ascending slot pattern as the verified row above.
//  - "Faretra" (ids 15-17): the first 3 of the CRE's 4 ammo slots - this
//    CHU layout only has 3 controls (declared deviation).
//  - "Oggetti rapidi" (ids 5-7): QuickItem1-3 (slots 18-20), per the
//    item-order comment in Actor.cpp.
// Still unmapped (deliberately, no empirical confirmation yet):
// rings/amulet/belt/boots/shield (ids ~21-26).
struct inv_slot_control { uint32 controlID; uint32 creSlot; };
static const inv_slot_control kInvSlotControls[] = {
	{ 30, kSlotGeneralFirst +  0 }, { 32, kSlotGeneralFirst +  1 },
	{ 34, kSlotGeneralFirst +  2 }, { 36, kSlotGeneralFirst +  3 },
	{ 38, kSlotGeneralFirst +  4 }, { 40, kSlotGeneralFirst +  5 },
	{ 42, kSlotGeneralFirst +  6 }, { 44, kSlotGeneralFirst +  7 },
	{ 31, kSlotGeneralFirst +  8 }, { 33, kSlotGeneralFirst +  9 },
	{ 35, kSlotGeneralFirst + 10 }, { 37, kSlotGeneralFirst + 11 },
	{ 39, kSlotGeneralFirst + 12 }, { 41, kSlotGeneralFirst + 13 },
	{ 43, kSlotGeneralFirst + 14 }, { 45, kSlotGeneralFirst + 15 },
	{ 11, kSlotArmor }, { 12, kSlotGauntlets }, { 13, kSlotHelmet }, { 14, kSlotCloak },
	{ 1, kSlotWeaponFirst }, { 2, kSlotWeaponFirst + 1 },
	{ 3, kSlotWeaponFirst + 2 }, { 4, kSlotWeaponFirst + 3 },
	{ 15, kSlotAmmoFirst }, { 16, kSlotAmmoFirst + 1 }, { 17, kSlotAmmoFirst + 2 },
	{ 5, 18 }, { 6, 19 }, { 7, 20 },
	// Rings / amulet / belt / boots / shield (GUIINV window-2 ids 21-26,
	// mapped by on-screen position - to be confirmed empirically).
	{ 22, kSlotRingLeft }, { 23, kSlotRingLeft + 1 }, { 25, kSlotAmulet },
	{ 21, kSlotBelt }, { 24, kSlotBoots }, { 26, kSlotShield },
};



// CRE item-slot for a GUIINV control id, or -1 if the control isn't a
// mapped inventory slot.
static int32
_CreSlotForControl(uint32 controlID)
{
	for (const auto& entry : kInvSlotControls) {
		if (entry.controlID == controlID)
			return (int32)entry.creSlot;
	}
	return -1;
}


// GUIINV window 2's 5 "ground item" slot buttons (ids 68-72, confirmed
// identical in both games' real CHU data) - the actual click target for
// dropping a held item to the floor: real GemRB GUIScripts
// (InventoryCommon.OnDragItemGround()) call DropDraggedItem(pc, -2) on
// exactly these while an item is being dragged. Only that drop side is
// wired here; the same buttons are also meant to show and pick back up
// whatever's already on the ground at the character's feet (paged by
// the neighboring scrollbar, control id 66) - not implemented yet, so
// they stay visually empty. Ground items are still only picked up by
// clicking their pile in the game world (AreaRoom::PickUpGroundPile()).
static bool
_IsGroundItemSlotControl(uint32 controlID)
{
	return controlID >= 68 && controlID <= 72;
}


// Populates the inventory-slot buttons in the open GUIINV window 2 with
// the real item icon (ITM's InventoryIcon(), cycle 0/frame 0 of that BAM -
// same convention Button's own constructor uses for its CHU-authored
// bitmaps) for whichever item currently occupies the matching slot in the
// shown character's CREResource, clearing the icon on empty slots.
/* virtual */
void
InventoryScreen::RefreshContent()
{
	Actor* actor = fGame.ShownActor();
	if (actor == NULL || actor->CRE() == NULL)
		return;

	Window* window = GetWindow(kContentWindow);
	if (window == NULL)
		return;

	for (const auto& entry : kInvSlotControls)
		_SetSlotIcon(window, actor, entry.controlID, entry.creSlot);

	_UpdatePaperdoll(window, actor);
	_UpdateLabels(window, actor);
	_UpdateGroundItemSlots(window, actor);
}


// GUI::ControlInvoked() routes clicks on GUIINV slot buttons here (window
// 2). Click-to-pick, click-to-place: the first click on a non-empty slot
// picks the item up (it rides the cursor via GUI::SetDragBitmap()); the
// next click drops it into the clicked slot, swapping with whatever's
// there. A rejected drop (incompatible slot, e.g. armor onto a weapon
// slot) keeps the item on the cursor so the player can try elsewhere;
// clicking the origin slot again puts it back. Operates on whichever
// party member the portrait column currently shows (fGame.ShownActor()).
/* virtual */
void
InventoryScreen::PanelControlInvoked(uint16 windowID, uint32 controlID)
{
	if (windowID == kInfoWindow) {
		// Any button in the examine popup just closes it.
		GUI::Get()->HideAuxWindow(CHUName(), kInfoWindow);
		return;
	}

	if (windowID == kContentWindow && _IsGroundItemSlotControl(controlID)) {
		if (GUI::Get()->IsDraggingItem())
			_DropHeldItemOnGround();
		return;
	}

	int32 slot = _CreSlotForControl(controlID);
	if (slot < 0)
		return;

	Actor* actor = fGame.ShownActor();
	if (actor == NULL || actor->CRE() == NULL)
		return;

	if (!GUI::Get()->IsDraggingItem()) {
		IE::item item;
		if (!actor->CRE()->GetItemAtSlot((uint32)slot, item))
			return; // empty slot - nothing to pick up
		fDragSlot = slot;
		GUI::Get()->SetDragBitmap(ScreenSupport::MakeItemIcon(item.name));
		std::cout << actor->Name() << " picks up " << ScreenSupport::ItemDisplayName(item.name)
			<< std::endl;
		return;
	}

	// Fetched before MoveItemToSlot() - on a swap, fDragSlot no longer
	// holds this item afterwards (it holds whatever was in `slot`).
	IE::item draggedItem;
	actor->CRE()->GetItemAtSlot((uint32)fDragSlot, draggedItem);
	std::string itemName = ScreenSupport::ItemDisplayName(draggedItem.name);

	if (actor->MoveItemToSlot((uint32)fDragSlot, (uint32)slot)) {
		fDragSlot = -1;
		GUI::Get()->SetDragBitmap(NULL);
		RefreshContent();
		std::cout << actor->Name() << " puts " << itemName << " in slot " << slot
			<< (slot == actor->ActiveWeaponSlot() ? " (equipped weapon)" : "")
			<< std::endl;
	} else {
		// Drop rejected (incompatible slot) - keep holding the item.
		std::cout << actor->Name() << " can't put " << itemName << " there"
			<< std::endl;
	}
}


// Right-click on a GUIINV window-2 slot: if the player is mid-drag, drop
// the held item back where it came from (BG2's right-click-cancels); if
// the slot holds an item, examine it (opens the GUIINVHI popup).
/* virtual */
bool
InventoryScreen::ControlRightClicked(uint16 windowID, uint32 controlID)
{
	if (windowID != kContentWindow)
		return false;

	if (GUI::Get()->IsDraggingItem()) {
		_ClearDrag();
		return true;
	}

	int32 slot = _CreSlotForControl(controlID);
	Actor* actor = fGame.ShownActor();
	if (slot < 0 || actor == NULL || actor->CRE() == NULL)
		return true;

	IE::item item;
	if (actor->CRE()->GetItemAtSlot((uint32)slot, item))
		_ShowItemInfo(item.name);
	return true;
}


// Populates and shows GUIINV's examine popup (window 5) for an item.
void
InventoryScreen::_ShowItemInfo(const res_ref& itemName)
{
	ITMResource* itm = gResManager->GetITM(itemName);
	if (itm == NULL)
		return;

	GUI::Get()->ShowAuxWindow(CHUName(), kInfoWindow);
	Window* window = GetWindow(kInfoWindow);
	if (window == NULL) {
		gResManager->ReleaseResource(itm);
		return;
	}

	Label* title = dynamic_cast<Label*>(window->GetControlByID(kInvInfoTitleID));
	if (title != NULL)
		title->SetText(ScreenSupport::ItemDisplayName(itm, itemName));

	for (uint32 id : { kInvInfoBlankLabel1ID, kInvInfoBlankLabel2ID }) {
		Label* label = dynamic_cast<Label*>(window->GetControlByID(id));
		if (label != NULL)
			label->SetText("");
	}

	Button* icon = dynamic_cast<Button*>(window->GetControlByID(kInvInfoIconID));
	if (icon != NULL)
		icon->SetIcon(ScreenSupport::MakeItemIcon(itemName), true);

	TextArea* description =
		dynamic_cast<TextArea*>(window->GetControlByID(kInvInfoTextID));
	if (description != NULL) {
		description->ClearText();
		std::string text = IDTable::GetDialog(itm->DescriptionRef());
		description->AddText(text.empty() ? "(No description)" : text.c_str());
		description->ScrollTo(0, 0);
	}

	gResManager->ReleaseResource(itm);
}


// Hover enter/leave on a GUIINV window-2 slot: show the item's name in a
// tooltip next to the cursor while the pointer is over a filled slot.
/* virtual */
bool
InventoryScreen::ControlHovered(uint16 windowID, uint32 controlID, bool inside)
{
	if (windowID != kContentWindow || !inside) {
		GUI::Get()->SetHoverTooltip("");
		return true;
	}

	int32 slot = _CreSlotForControl(controlID);
	Actor* actor = fGame.ShownActor();
	if (slot < 0 || actor == NULL || actor->CRE() == NULL)
		return true;

	IE::item item;
	if (!actor->CRE()->GetItemAtSlot((uint32)slot, item)) {
		GUI::Get()->SetHoverTooltip("");
		return true;
	}

	ITMResource* itm = gResManager->GetITM(item.name);
	std::string name = ScreenSupport::ItemDisplayName(itm, item.name);
	if (itm != NULL)
		gResManager->ReleaseResource(itm);
	GUI::Get()->SetHoverTooltip(name);
	return true;
}


// Dropping an item: a click on the inventory window that didn't land on
// any slot, while an item rides the cursor. The held item leaves the
// shown character's inventory and becomes a loose pile on the area floor
// at that character's feet (picked back up by clicking it in the world -
// see AreaRoom::PickUpGroundPile()). No-op (item stays on the cursor) if
// the current room isn't an explorable area.
void
InventoryScreen::_DropHeldItemOnGround()
{
	if (!GUI::Get()->IsDraggingItem() || fDragSlot < 0)
		return;

	Actor* actor = fGame.ShownActor();
	AreaRoom* room = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
	if (actor == NULL || actor->CRE() == NULL || room == NULL)
		return;

	IE::item item;
	if (!actor->TakeItemFromSlot((uint32)fDragSlot, item))
		return;

	room->AddGroundItem(item, actor->Position());
	fDragSlot = -1;
	GUI::Get()->SetDragBitmap(NULL);
	RefreshContent();
}


// Composites one equipped item's paperdoll overlay (a "WP" + size +
// animation-code + suffix BAM, e.g. "WPMS1INV" for a medium character's
// long sword) onto `canvas`. The frame's own stored center offset is
// the target position outright (negated, no separate canvas anchor
// involved) - confirmed against GemRB's AnimationFactory::
// GetPaperdollImage()/Button::DrawSelf(), which blit every paperdoll
// layer at the same button-relative point and let each sprite's own
// stored offset place it.
static void
_CompositePaperdollOverlay(Bitmap* canvas, const char* sizeCode,
		const std::string& animationCode, const char* suffix)
{
	if (animationCode.empty())
		return;

	std::string resRef = std::string("WP") + sizeCode + animationCode + suffix;
	BAMResource* bam = gResManager->GetBAM(resRef.c_str());
	if (bam == nullptr)
		return;

	Bitmap* frame = bam->FrameForCycle(0, 0);
	if (frame != nullptr) {
		GFX::rect frameRect = frame->Frame();
		GFX::point where(-(frameRect.x + frame->Width() / 2),
				-(frameRect.y + frame->Height() / 2));
		frame->BlitTo(canvas, where);
		frame->Release();
	}
	gResManager->ReleaseResource(bam);
}


// Swaps the paperdoll control's fixed CHU-authored placeholder (CIFF4INV,
// a generic doll unrelated to the shown character) for the real thing:
// the actual character's own class/race/gender/armor identity (see
// AnimationFactory::PaperdollName()), rendered from its PLT resource and
// recolored with their own CRE colors (see PLTResource::Image()), with
// the equipped weapon and shield/off-hand item composited on top (see
// _CompositePaperdollOverlay()). Helmet/armor overlays aren't - BG2
// already bakes worn armor into the base doll's own resref, and helmets
// aren't handled yet.
void
InventoryScreen::_UpdatePaperdoll(Window* window, Actor* actor)
{
	Button* button = dynamic_cast<Button*>(window->GetControlByID(kInvPaperdollID));
	if (button == NULL)
		return;

	Bitmap* icon = nullptr;
	std::string name = actor->PaperdollName();
	if (Core::Get()->Game() == game::GAME_BALDURSGATE) {
		// Baldur's Gate 1 has paperdolls in BAM resources instead
		// TODO: it only shows the upper body for now, and in the wrong color, too
		BAMResource* bam = gResManager->GetBAM(name.c_str());
		if (bam != nullptr) {
			icon = bam->FrameForCycle(0, 0);
			gResManager->ReleaseResource(bam);
		} else {
			std::cerr << "InventoryScreen::_UpdatePaperdoll(): no BAM resource named "
									<< name << std::endl;
		}
	} else {
		PLTResource* plt = gResManager->GetPLT(name.c_str());
		if (plt == NULL && name.length() >= 5) {
			// Not every class-letter x armor-digit paperdoll exists in every
			// install; fall back to the unarmored (digit 1) doll rather than
			// leaving the paperdoll blank.
			name[4] = '1';
			plt = gResManager->GetPLT(name.c_str());
		}
		if (plt != NULL) {
				icon = plt->Image(actor->CRE()->Colors());
				gResManager->ReleaseResource(plt);
			} else {
				std::cerr << "InventoryScreen::_UpdatePaperdoll(): no PLT resource named "
						<< name << std::endl;
		}

		if (icon != nullptr) {
			const char* sizeCode = AnimationFactory::SizeCodeForActor(actor);

			ITMResource* weapon = actor->EquippedWeapon();
			if (weapon != nullptr) {
				_CompositePaperdollOverlay(icon, sizeCode, weapon->Animation(), "INV");
				gResManager->ReleaseResource(weapon);
			}

			IE::item shieldItem;
			if (actor->CRE()->GetItemAtSlot(kSlotShield, shieldItem)) {
				ITMResource* shield = gResManager->GetITM(shieldItem.name);
				if (shield != nullptr) {
					// 0x000c: real shield, uses the same "INV" suffix as
					// the weapon; anything else in this slot is an
					// off-hand weapon (dual-wielding), which uses "OIN".
					const char* suffix = shield->ItemType() == 0x000c ? "INV" : "OIN";
					_CompositePaperdollOverlay(icon, sizeCode, shield->Animation(), suffix);
					gResManager->ReleaseResource(shield);
				}
			}
		}
	}

	// coverBackground: the paperdoll control's CHU bitmap is just a
	// generic placeholder doll (CIFF4INV) - hide it so it can't show
	// through the real doll's transparent areas.
	button->SetIcon(icon, true);
}


// Fills in the two GUIINV labels whose CHU-authored text_ref resolves to
// a literal "(No text)" TLK placeholder - real BG2 sets these from code,
// not from static CHU data, same as the item icons above.
void
InventoryScreen::_UpdateLabels(Window* window, Actor* actor)
{
	Label* nameLabel = dynamic_cast<Label*>(window->GetControlByID(kInvNameLabelID));
	if (nameLabel != NULL)
		nameLabel->SetText(actor->LongName());

	Label* classLabel = dynamic_cast<Label*>(window->GetControlByID(kInvClassLabelID));
	if (classLabel != NULL) {
		// Prefer the localized title (IDTable::ClassName()'s own
		// comment) over the raw, always-English CLASS.IDS symbol.
		std::string text = IDTable::ClassName(actor->CRE()->Class());
		classLabel->SetText(text.empty() ? IDTable::ClassAt(actor->CRE()->Class()) : text);
	}

	Label* acLabel = dynamic_cast<Label*>(window->GetControlByID(kInvACLabelID));
	if (acLabel != NULL)
		acLabel->SetText(std::to_string(actor->CRE()->AC().effective));

	Label* hpLabel = dynamic_cast<Label*>(window->GetControlByID(kInvHPCurrentLabelID));
	if (hpLabel != nullptr)
		hpLabel->SetText(std::to_string(actor->CRE()->CurrentHitPoints()));

	Label* hpMaxLabel = dynamic_cast<Label*>(window->GetControlByID(kInvHPMaxLabelID));
	if (hpMaxLabel != nullptr)
		hpMaxLabel->SetText(std::to_string(actor->CRE()->MaxHitPoints()));

	// Party-wide, not per-actor - see Core::AddPartyGold()'s own comment.
	Label* goldLabel = dynamic_cast<Label*>(window->GetControlByID(kInvGoldLabelID));
	if (goldLabel != nullptr)
		goldLabel->SetText(std::to_string(Core::Get()->PartyGold()));

	ScreenSupport::EnsureWeightLabels(window);
	Label* weightLabel = dynamic_cast<Label*>(window->GetControlByID(ScreenSupport::kWeightCurrentLabelID));
	if (weightLabel != nullptr)
		weightLabel->SetText(std::to_string(ScreenSupport::CarriedWeight(actor->CRE())) + ":");

	Label* weightMaxLabel = dynamic_cast<Label*>(window->GetControlByID(ScreenSupport::kWeightMaxLabelID));
	if (weightMaxLabel != nullptr)
		weightMaxLabel->SetText(std::to_string(ScreenSupport::MaxEncumbrance(actor->CRE())) + ":");
}


// Looks up controlID's Button in window and sets its icon from whatever
// item (if any) sits in creSlot of cre - shared by the general-grid loop
// above and by individual equipment-slot mappings as they get confirmed.
void
InventoryScreen::_SetSlotIcon(Window* window, Actor* actor, uint32 controlID,
	uint32 creSlot)
{
	CREResource* cre = actor->CRE();
	Button* button = dynamic_cast<Button*>(window->GetControlByID(controlID));
	if (button == NULL)
		return;

	// Lets a real press-drag-release mouse gesture move an item between
	// slots in one motion, not just two separate clicks - see Button::
	// SetDragCapture()'s own comment. Set on every refresh (redundant
	// after the first, harmless) since this is the one place that walks
	// every inventory slot control.
	button->SetDragCapture(true);

	IE::item item;
	Bitmap* icon = NULL;
	int count = 0;
	if (cre->GetItemAtSlot(creSlot, item)) {
		icon = ScreenSupport::MakeItemIcon(item.name);
		count = item.quantity1;
	}
	button->SetIcon(icon);
	button->SetIconCount(count);

	// The only visible cue of which of the four weapon quickslots is in
	// hand, short of attacking to see the animation change: a highlighted
	// border (same mechanism already used for the selected party member's
	// portrait).
	if (creSlot >= kSlotWeaponFirst && creSlot < kSlotWeaponFirst + 4)
		button->SetHighlighted((int32)creSlot == actor->ActiveWeaponSlot());
}


// Mirrors whatever's in the ground pile at the shown character's own
// position into the 5 "ground item" slots (ids 68-72, see
// _IsGroundItemSlotControl()) - the same pile AreaRoom's world-click
// handler picks up via GroundPileAtPoint(). Only display: a click there
// is still handled purely as a drop target (InventoryControlInvoked()),
// not as its own pickup source - picking a specific item back up still
// means clicking the pile in the world. No paging if a pile holds more
// than 5 items (the neighboring scrollbar, control id 66, isn't wired
// yet) - declared simplification, not expected to matter for piles
// built up from drops alone.
void
InventoryScreen::_UpdateGroundItemSlots(Window* window, Actor* actor)
{
	AreaRoom* room = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
	const std::vector<IE::item>* items = NULL;
	if (room != NULL) {
		int32 pileIndex = room->GroundPileAtPoint(actor->Position());
		if (pileIndex >= 0)
			items = &room->GroundPiles()[(size_t)pileIndex].items;
	}

	for (uint32 i = 0; i < 5; i++) {
		Button* button = dynamic_cast<Button*>(window->GetControlByID(68 + i));
		if (button == NULL)
			continue;

		Bitmap* icon = NULL;
		int count = 0;
		if (items != NULL && i < items->size()) {
			icon = ScreenSupport::MakeItemIcon((*items)[i].name);
			count = (*items)[i].quantity1;
		}
		button->SetIcon(icon);
		button->SetIconCount(count);
	}
}
