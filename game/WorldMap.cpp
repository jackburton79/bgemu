#include "WorldMap.h"

#include "BmpResource.h"
#include "Bitmap.h"
#include "Control.h"
#include "Core.h"
#include "Game.h"
#include "Graphics.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "Label.h"
#include "MOSResource.h"
#include "ResManager.h"
#include "TextArea.h"
#include "TextSupport.h"
#include "TisResource.h"
#include "WMAPResource.h"

#include <algorithm>
#include <assert.h>
#include <cstring>
#include <iostream>
#include <stdexcept>


WorldMap::WorldMap(const res_ref& previousArea, int direction)
	:
	fWorldMap(NULL),
	fWorldMapBackground(NULL),
	fWorldMapBitmap(NULL),
	fAreaUnderMouse(NULL),
	fCurrentAreaName(previousArea),
	fDirection(direction)
{
	GUI* gui = GUI::Get();
	gui->Clear();

	if (!gui->Load("GUIWMAP")) {
		throw std::runtime_error("Cannot load GUIWMAP");
	}

	gui->ShowWindow(0);
	gui->Show();

	SetName("WORLDMAP");

	GraphicsEngine::Get()->SetWindowCaption(Name());

	fWorldMap = gResManager->GetWMAP(Name());

	worldmap_entry entry = fWorldMap->WorldMapEntry();
	fWorldMapBackground = gResManager->GetMOS(entry.background_mos);
	if (fWorldMapBackground == NULL) {
		gResManager->ReleaseResource(fWorldMap);
		throw std::runtime_error("Cannot load World map bitmap");
	}
	fWorldMapBitmap = fWorldMapBackground->Image();
	_LoadAreaEntries();
	_RevealAdjacentAreas(previousArea, direction);

	for (uint32 i = 0; i < fWorldMap->CountAreaEntries(); i++) {
		AreaEntry* areaEntry = fAreaEntries.at(i);
		if (!areaEntry->IsVisible())
			continue;

		const Bitmap* iconFrame = areaEntry->Icon();
		IE::point position = areaEntry->Position();
		GFX::rect iconRect(int16(position.x - iconFrame->Frame().w / 2),
					int16(position.y - iconFrame->Frame().h / 2),
					iconFrame->Frame().w, iconFrame->Frame().h);

		iconFrame->BlitTo(fWorldMapBitmap, iconRect.LeftTop());

		if (!areaEntry->Caption().empty()) {
			const Font* font = FontRoster::GetFont("TOOLFONT");
			Bitmap* nameBitmap = font->GetRenderedString(areaEntry->Caption(), 0,
														 GFX::kPaletteBlack);
			if (nameBitmap != NULL) {
				GFX::rect textRect = nameBitmap->Frame();
				// center horizontally
				textRect.x = iconRect.x + (iconRect.w - textRect.w) / 2;
				textRect.y = iconRect.y + iconRect.h + 5;
				nameBitmap->BlitTo(fWorldMapBitmap, textRect.LeftTop());
				nameBitmap->Release();
			}
		}
	}

	::Window* window = gui->GetWindow(0);
	if (window != NULL) {
		fSavedControl = window->ReplaceControl(4, this);
	}

	// Control::Frame() is only valid from here on (ReplaceControl() above
	// is what copies the real w/h into it), and SetAreaOffsetCenter()
	// needs it to compute a centered offset - has to run after the icon
	// draw loop above, not before, even though it's previousArea's
	// *position* (not anything screen/offset-related) driving it.
	AreaEntry* currentEntry = _CenterOnArea(previousArea);
	_MarkCurrentArea(currentEntry);
}


WorldMap::~WorldMap()
{
	Unload();
}


/* virtual */
GFX::rect
WorldMap::AreaRect() const
{
	GFX::rect rect;
	rect.x = rect.y = 0;
	rect.w = fWorldMapBitmap->Width();
	rect.h = fWorldMapBitmap->Height();
	return rect;
}


void
WorldMap::Draw()
{
	GraphicsEngine* gfx = GraphicsEngine::Get();

	if (fWorldMap != NULL) {
		GFX::rect sourceRect = Control::Frame();
		ConvertToArea(sourceRect);
		GFX::rect visibleArea = rect_to_gfx_rect(VisibleMapArea());

		// Control::Frame() is window-relative
		GFX::rect viewPort = Control::Frame();
		Window()->ConvertToScreen(viewPort);
		gfx->BlitToScreen(fWorldMapBitmap, &visibleArea, &viewPort);
		if (fAreaUnderMouse != NULL) {
			GFX::rect areaRect = fAreaUnderMouse->Rect();
			areaRect.x += viewPort.x - AreaOffset().x;
			areaRect.y += viewPort.y - AreaOffset().y;
			GraphicsEngine::Get()->ScreenBitmap()->StrokeRect(areaRect, 600);
		}
	}
}


void
WorldMap::MouseDown(IE::point point)
{
	if (fAreaUnderMouse == NULL)
		return;

	// Clicking the area the party is actually standing in (backgrounded
	// in Core::fPreviousRoom) used to go through Core::LoadArea() like
	// any other destination - which unconditionally discards that
	// backgrounded room (see Core::UnloadCurrentRoom()) and builds a
	// fresh AreaRoom from scratch instead, at an arbitrary entrance
	// (WorldMap::MouseDown() passes no entrance name) rather than back
	// where the party actually was. ReturnFromWorldMap() - the same path
	// GUIWMAP's "Done" button already uses - is the correct one here: no
	// real travel is happening, the party never left.
	if (fAreaUnderMouse->Name() == fCurrentAreaName) {
		Core::Get()->ReturnFromWorldMap();
		return;
	}

	Core::Get()->LoadArea(fAreaUnderMouse->Name(), fAreaUnderMouse->LongName(),
		_EntranceTo(fAreaUnderMouse));
}


// The entrance of `destination` the current area's link to it names, the
// edge the party left through first; empty if there is no such link.
std::string
WorldMap::_EntranceTo(const AreaEntry* destination) const
{
	const auto found = std::find(fAreaEntries.begin(), fAreaEntries.end(), destination);
	const AreaEntry* current = nullptr;
	for (const AreaEntry* entry : fAreaEntries) {
		if (entry->Name() == fCurrentAreaName)
			current = entry;
	}
	if (current == nullptr || found == fAreaEntries.end())
		return "";

	const uint32 destinationIndex = static_cast<uint32>(found - fAreaEntries.begin());
	for (int i = 0; i < 4; i++) {
		const int direction = fDirection >= 0 ? (fDirection + i) % 4 : i;
		uint32 linkIndex, linkCount;
		current->LinkRange(direction, &linkIndex, &linkCount);
		for (uint32 l = 0; l < linkCount; l++) {
			const arealink_entry link = fWorldMap->GetAreaLink(linkIndex + l);
			if (link.destination_index == destinationIndex)
				return std::string(link.entry_point, strnlen(link.entry_point, sizeof(link.entry_point)));
		}
	}
	return "";
}


void
WorldMap::MouseMoved(IE::point point, uint32 transit)
{
	ConvertFromScreen(point);
	ConvertToArea(point);

	fAreaUnderMouse = NULL;

	for (const auto area : fAreaEntries) {
		if (area->IsVisible() && area->Rect().Contains(point.x, point.y)) {
			fAreaUnderMouse = area;
			break;
		}
	}

	GUI::Get()->SetArrowCursor(IE::CURSOR_HAND);
#if 0
	::Window* window = GUI::Get()->GetWindow(0);
	if (window == NULL)
		return;
	Label* label = dynamic_cast<Label*>(window->GetControlByID(268435458));
	if (label != NULL) {
		if (fAreaUnderMouse != NULL) {
			label->SetText(fAreaUnderMouse->Caption());
		} else
			label->SetText("");
	}
#endif
}


/* virtual */
void
WorldMap::Unload()
{
	_UnloadWorldMap();
	RoomBase::Unload();
}


/* virtual */
void
WorldMap::ReloadArea()
{
}


void
WorldMap::_UnloadWorldMap()
{
	std::cout << "WorldMap::Unload()" << std::endl;

	// TODO: here we could have been called by the Window destructor,
	// so some of these fields could have already been deleted
	DetachFromWindow();
	GraphicsEngine::Get()->ScreenBitmap()->Clear(0);

	for (auto entry : fAreaEntries)
		delete entry;
	fAreaEntries.clear();

	if (fWorldMap != nullptr) {
		gResManager->ReleaseResource(fWorldMap);
		fWorldMap = NULL;
	}
	if (fWorldMapBackground != nullptr) {
		gResManager->ReleaseResource(fWorldMapBackground);
		fWorldMapBackground = NULL;
	}
	if (fWorldMapBitmap != NULL) {
		fWorldMapBitmap->Release();
		fWorldMapBitmap = NULL;
	}

	gResManager->TryEmptyResourceCache();
}


void
WorldMap::_LoadAreaEntries()
{
	for (uint32 c = 0; c < fWorldMap->CountAreaEntries(); c++) {
		AreaEntry* areaEntry = fWorldMap->GetAreaEntry(c);

		// A script's REVEALAREAONMAP/HIDEAREAONMAP (Game::
		// SetAreaMapVisible()) overrides the file's own visibility bit,
		// same as this engine already does elsewhere for LOCALS-style
		// runtime overrides of on-disk data.
		bool visible;
		if (Game::Get()->AreaMapVisibleOverride(areaEntry->Name().CString(), &visible))
			areaEntry->SetVisible(visible);

		fAreaEntries.push_back(areaEntry);
	}
}


// Real WMP data marks most areas AREA_VISIBLE_FROM_ADJACENT instead of
// relying on a script's explicit RevealAreaOnMap() call - leaving
// `previousArea` through its `direction` edge is what's actually
// supposed to make that side's linked neighbors selectable on the map
// (see GemRB's WorldMap::UpdateAreaVisibility() for the same real-engine
// behavior this mirrors), not just areas a script happened to reveal.
// direction < 0 (a manual open, HUD button/hotkey - see WorldMap's own
// constructor comment) is a no-op, same as real IE.
//
// Persists through Game::SetAreaMapVisible() (the same override
// REVEALAREAONMAP itself uses) so it survives this WorldMap instance -
// _LoadAreaEntries() already re-applies it on every future visit - but
// this instance's own fAreaEntries need updating directly too, since
// _LoadAreaEntries() already ran before this reveal existed.
void
WorldMap::_RevealAdjacentAreas(const res_ref& previousArea, int direction)
{
	if (direction < 0)
		return;

	for (auto entry : fAreaEntries) {
		if (entry->Name() != previousArea)
			continue;

		// The area just left is always at least visible now - real
		// content usually already has this set, but a few areas
		// (ambush encounters, scripted intro areas) are only ever
		// entered directly, never clicked from the map first.
		Game::Get()->SetAreaMapVisible(previousArea.CString(), true);
		entry->SetVisible(true);

		uint32 linkIndex, linkCount;
		entry->LinkRange(direction, &linkIndex, &linkCount);
		for (uint32 i = 0; i < linkCount; i++) {
			arealink_entry link = fWorldMap->GetAreaLink(linkIndex + i);
			if (link.destination_index >= fAreaEntries.size())
				continue;

			AreaEntry* destination = fAreaEntries[link.destination_index];
			if ((destination->Flags() & AREA_VISIBLE_FROM_ADJACENT) == 0)
				continue;

			Game::Get()->SetAreaMapVisible(destination->Name().CString(), true);
			destination->SetVisible(true);
		}
		return;
	}
}


AreaEntry*
WorldMap::_CenterOnArea(const res_ref& areaName)
{
	for (auto entry : fAreaEntries) {
		if (entry->Name() != areaName)
			continue;

		SetAreaOffsetCenter(entry->Position());
		return entry;
	}
	return NULL;
}


// A permanent "you are here" marker (as opposed to the transient hover
// highlight WorldMap::Draw() already strokes around fAreaUnderMouse) for
// the area actually backgrounded under this WorldMap - baked directly
// into fWorldMapBitmap alongside the icons/captions above instead of
// redrawn every frame, same reasoning as those.
void
WorldMap::_MarkCurrentArea(const AreaEntry* entry)
{
	if (entry == NULL)
		return;

	GFX::rect rect = entry->Rect();
	uint32 color = fWorldMapBitmap->MapRGBColor(0, 255, 0);
	fWorldMapBitmap->StrokeRect(rect, color);
}
