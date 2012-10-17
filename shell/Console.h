/*
 * Console.h
 *
 *  Created on: 10/ott/2012
 *      Author: stefano
 */

#ifndef __CONSOLE_H_
#define __CONSOLE_H_

#include "GraphicsEngine.h"

#include <sstream>

typedef enum {
	LINE_ERASE_WHOLE,
	LINE_ERASE_LEFT,
	LINE_ERASE_RIGHT
} erase_line_mode;


struct console_info;
class Console {
public:
	Console(const GFX::rect& rect);
	~Console();

	void Draw();
	void Toggle();
	bool IsActive() const;

	GFX::rect Rect() const;

	void HandleInput(uint8 input);

	void HideCursor();
	void EraseLine(erase_line_mode mode);
	void PutChar(char c);
	void PutCharacter(char c);
	void Puts(const char *text);

	void ClearScreen();
	void Clear(uint8 attr);

	void ScrollUp();

	void SetPaging(bool paging);
	bool IsPagingEnabled() const;

protected:
	void _SetVt100Attributes(int32 *args, int32 argCount);
	void _ParseCharacter(char c);

	bool _ProcessVt100Command(const char c, bool seenBracket,
				int32 *args, int32 argCount);
	void _NextLine();
	void _BackSpace();
	void _PutGlyph(int32 x, int32 y, uint8 glyph, uint8 attr);
	void _FillGlyph(int32 x, int32 y,
			int32 width, int32 height,
			uint8 glyph, uint8 attr);
	void _RenderGlyph(int32 x, int32 y, uint8 glyph, uint8 attr);
	void _MoveCursor(int32 x, int32 y);
	void _DrawCursor(int32 x, int32 y);

	GFX::rect fRect;
	bool fShowing;
	Bitmap* fBitmap;
	console_info* fDesc;
	std::ostringstream fOutputBuffer;
};

#endif /* __CONSOLE_H_ */
