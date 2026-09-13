/*
 * Scrollbar.h
 *
 *  Created on: 21/ott/2012
 *      Author: stefano
 */

#pragma once

#include "Bitmap.h"
#include "Control.h"

namespace GFX {
	class rect;
}

class BAMResource;
class TextArea;
class Scrollbar: public Control {
public:
	Scrollbar(IE::scrollbar* scrollbar);
	virtual ~Scrollbar();

	void AttachedToWindow(::Window* window) override;
	void Draw() override;
	void MouseMoved(IE::point point, uint32 transit) override;
	void MouseDown(IE::point point) override;
	void MouseUp(IE::point point) override;
	void Pulse() override;

	// Called by the linked TextArea whenever its scroll state changes:
	// the current offset and the maximum offset, both in content pixels.
	void SetScrollInfo(int32 offset, int32 range);

private:
	BAMResource* fResource;
	Bitmap* fUpArrow;
	Bitmap* fUpArrowPressed;
	Bitmap* fDownArrow;
	Bitmap* fDownArrowPressed;
	Bitmap* fTrough;
	Bitmap* fThumb;

	bool fUpPressed;
	bool fDownPressed;
	bool fDraggingThumb;
	int16 fThumbGrabOffset;

	int32 fOffset;
	int32 fRange;

	// All window-local (pre-screen-conversion).
	GFX::rect _UpArrowRect() const;
	GFX::rect _DownArrowRect() const;
	GFX::rect _TroughRect() const;
	GFX::rect _ThumbRect() const;

	TextArea* _TextArea() const;
	void _ScrollTo(int32 offset);
	void _ScrollBy(int32 delta);
};
