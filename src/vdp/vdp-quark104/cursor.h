#ifndef CURSOR_H
#define CURSOR_H

#include <fabgl.h>

#include "agon_keyboard.h"
#include "graphics.h"
#include "viewport.h"
// TODO remove this, somehow
#include "vdp_protocol.h"

extern uint8_t	fontW;
extern uint8_t	fontH;
extern void drawCursor(Point p);
extern void scrollRegion(Rect * r, uint8_t direction, int16_t amount);

Point			textCursor;						// Text cursor
Point *			activeCursor;					// Pointer to the active text cursor (textCursor or p1)
bool			cursorEnabled = true;			// Cursor visibility
uint8_t			cursorBehaviour = 0;			// Cursor behavior
bool 			pagedMode = false;				// Is output paged or not? Set by VDU 14 and 15
uint8_t			pagedModeCount = 0;				// Scroll counter for paged mode


// Render a cursor at the current screen position
//
void do_cursor() {
	if (cursorEnabled) {
		drawCursor(textCursor);
	}
}

inline Point * getTextCursor() {
	return &textCursor;
}

inline bool textCursorActive() {
	return activeCursor == &textCursor;
}

inline void setActiveCursor(Point * cursor) {
	activeCursor = cursor;
}

inline void setCursorBehaviour(uint8_t setting, uint8_t mask = 0xFF) {
	cursorBehaviour = (cursorBehaviour & mask) ^ setting;
}

// Move the active cursor to the leftmost position in the viewport
//
void cursorCR() {
	activeCursor->X = activeViewport->X1;
}

// Move the active cursor down a line
//
void cursorDown() {
	activeCursor->Y += fontH;
	//
	// If in graphics mode, then don't scroll, wrap to top
	// 
	if (!textCursorActive()) {
		if (activeCursor->Y > activeViewport->Y2) {	
			activeCursor->Y = activeViewport->Y2;
		}
		return;
	}
	//
	// Using the text cursor, so handle paging and scrolling
	//
	if (pagedMode) {
		pagedModeCount++;
		if (pagedModeCount >= (activeViewport->Y2 - activeViewport->Y1 + 1) / fontH) {
			pagedModeCount = 0;
			uint8_t ascii;
			uint8_t vk;
			uint8_t down;
			if (!wait_shiftkey(&ascii, &vk, &down)) {
				// ESC pressed
				uint8_t packet[] = {
					ascii,
					0,
					vk,
					down,
				};
				// TODO replace this, somehow
				send_packet(PACKET_KEYCODE, sizeof packet, packet);
			}
		}
	}
	//
	// Check if scroll required
	//
	if (activeCursor->Y > activeViewport->Y2) {
		activeCursor->Y -= fontH;
		if (~cursorBehaviour & 0x01) {
			scrollRegion(activeViewport, 3, fontH);
		}
		else {
			activeCursor->X = activeViewport->X2 + 1;
		}
	}
}

// Move the active cursor up a line
//
void cursorUp() {
	activeCursor->Y -= fontH;
	if (activeCursor->Y < activeViewport->Y1) {
		activeCursor->Y = activeViewport->Y1;
	}
}

// Move the active cursor back one character
//
void cursorLeft() {
	activeCursor->X -= fontW;											
	if (activeCursor->X < activeViewport->X1) {						// If moved past left edge of active viewport then
		activeCursor->X = activeViewport->X1;						// Lock it there
	}
}

// Advance the active cursor right one character
//
void cursorRight() {
	activeCursor->X += fontW;											
	if (activeCursor->X > activeViewport->X2) {						// If advanced past right edge of active viewport
		if (textCursorActive() || (~cursorBehaviour & 0x40)) {		// If it is a text cursor or VDU 5 CR/LF is enabled then
			cursorCR();												// Do carriage return
			cursorDown();											// and line feed
		}
	}
}

// Move the active cursor to the top-left position in the viewport
//
void cursorHome() {
	activeCursor->X = activeViewport->X1;
	activeCursor->Y = activeViewport->Y1;
}

// TAB(x,y)
//
void cursorTab(uint8_t x, uint8_t y) {
	activeCursor->X = x * fontW;
	activeCursor->Y = y * fontH;
}

void setPagedMode(bool mode = pagedMode) {
	pagedMode = mode;
	pagedModeCount = 0;
}

// Reset basic cursor control
//
void resetCursor() {
	setActiveCursor(getTextCursor());
	cursorEnabled = true;
	cursorBehaviour = 0;
	setPagedMode(false);
}

void homeCursor() {
	textCursor = Point(activeViewport->X1, activeViewport->Y1);
}

inline void enableCursor(bool enable = true) {
	cursorEnabled = enable;
}

void ensureCursorInViewport(Rect viewport) {
	if (activeCursor->X < viewport.X1
		|| activeCursor->X > viewport.X2
		|| activeCursor->Y < viewport.Y1
		|| activeCursor->Y > viewport.Y2
		) {
		activeCursor->X = viewport.X1;
		activeCursor->Y = viewport.Y1;
	}
}

#endif	// CURSOR_H
