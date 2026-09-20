/*
 * Button.h
 *
 *  Created on: 19/ott/2012
 *      Author: stefano
 */

#pragma once

#include "Control.h"
#include "GraphicsDefs.h"
#include "IETypes.h"

class Bitmap;
class Window;
class Button: public Control {
public:
	Button(IE::button* button);
	virtual ~Button();

	void AttachedToWindow(::Window* window) override;
	void Draw() override;
	void MouseMoved(IE::point point, uint32 transit) override;
	void MouseDown(IE::point point) override;
	void MouseUp(IE::point point) override;
	bool RightMouseDown(IE::point point) override;

	// Overlay drawn centered on top of the button's own frame - e.g. an
	// item icon over an inventory slot's empty-slot background. Takes a
	// reference (like the CHU-authored bitmaps above); pass NULL to clear
	// it. Not part of the CHU format itself, so no constructor-time
	// equivalent. With coverBackground the button's own frame bitmap is
	// hidden while the icon is set.
	void SetIcon(Bitmap* icon, bool coverBackground = false);

	// A small quantity number drawn in the icon's bottom-right corner
	// (for a stacked inventory item - arrows, potions, gems). 0 or 1
	// draws nothing.
	void SetIconCount(int count);

	// Draws a bright outline around the button - used to mark the
	// currently-selected party member's portrait.
	void SetHighlighted(bool highlighted);

	// Draws an amber outline (like SetHighlighted()'s, but a different
	// color so the two are distinguishable) to persistently mark a
	// command-bar icon whose window is currently open - unlike fPressed's
	// transient mouse-down-only state.
	void SetToggled(bool toggled);

	// Opts this button into "press acts like a click too" - see
	// MouseDown()/MouseUp()'s own comments. Used only by the inventory's
	// slot buttons, so a real press-drag-release gesture (not just two
	// separate clicks) can pick an item up and drop it in one motion;
	// every other button in the GUI leaves this false and keeps its
	// existing release-only Invoke() behavior untouched.
	void SetDragCapture(bool captures);

	// A short caption drawn centered on top of the button's own frame -
	// for a CHU button whose art is a blank, reusable shape (e.g. a
	// generic dialog button) rather than one with its label baked into
	// the BAM itself. Pass an empty string to clear it.
	void SetText(const std::string& text);

	void SetEnabled(bool enabled);
	bool Enabled() const { return fEnabled; }

	// Switches the button's own art to another cycle of the same BAM (the
	// CHU only names one) - for a button whose icon depends on what it's
	// used for, e.g. the store window's page tabs.
	void SetCycle(uint16 cycle);
	// Replaces the button's art with the given frames of another BAM's
	// first cycle (unpressed, pressed, selected, disabled). Returns false
	// (art unchanged) if the BAM or a frame doesn't exist.
	bool SetArt(const res_ref& bam, const uint16 frames[4]);
	// Back to the CHU-authored art (an unused action bar slot).
	void RestoreArt();
	// Draws none of the button's own frame art (an unused slot that should
	// just be empty) - pair it with SetEnabled(false).
	void SetFrameless(bool frameless);

private:
	void _DrawOutline(uint8 r, uint8 g, uint8 b);

	Bitmap* fDisabledBitmap;
	Bitmap* fSelectedBitmap;
	Bitmap* fPressedBitmap;
	Bitmap* fUnpressedBitmap;
	Bitmap* fIcon;
	Bitmap* fText;
	GFX::rect fTextRect;
	// Icon position/size within the button's own (window-local, not yet
	// screen-converted) frame - computed once in SetIcon() rather than
	// every Draw() call, since it only depends on the icon's fixed size
	// and the button's own (static, CHU-authored) frame, neither of
	// which change between frames.
	GFX::rect fIconRect;
	// Part of the icon that fits the button (an icon bigger than its
	// button is cropped around its center, not drawn over the neighbours).
	GFX::rect fIconSource;
	bool fIconCropped;
	int fIconCount;
	// The rendered quantity number, built when the count changes.
	Bitmap* fCountBitmap = NULL;
	bool fCoverBackground;
	bool fHighlighted;
	bool fEnabled;
	bool fSelected;
	bool fPressed;
	bool fToggled;
	bool fDragCapture;
	bool fArmedByPress;
	bool fFrameless;
};
