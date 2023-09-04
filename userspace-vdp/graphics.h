#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <fabgl.h>

#include "agon.h"
#include "agon_screen.h"
#include "agon_fonts.h"							// The Acorn BBC Micro Font
#include "agon_palette.h"						// Colour lookup table
#include "cursor.h"
#include "sprites.h"
#include "viewport.h"

fabgl::PaintOptions			gpo;				// Graphics paint options
fabgl::PaintOptions			tpo;				// Text paint options

Point			p1, p2, p3;						// Coordinate store for plot
RGB888			gfg, gbg;						// Graphics foreground and background colour
RGB888			tfg, tbg;						// Text foreground and background colour
uint8_t			fontW;							// Font width
uint8_t			fontH;							// Font height
uint8_t			videoMode;						// Current video mode
bool			legacyModes = false;			// Default legacy modes being false
uint8_t			palette[64];					// Storage for the palette


// VDU 23, 0, &86: Send MODE information (screen details)
//
void sendModeInformation() {
	uint8_t packet[] = {
		canvasW & 0xFF,						// Width in pixels (L)
		(canvasW >> 8) & 0xFF,				// Width in pixels (H)
		canvasH & 0xFF,						// Height in pixels (L)
		(canvasH >> 8) & 0xFF,				// Height in pixels (H)
		canvasW / fontW,					// Width in characters (byte)
		canvasH / fontH,					// Height in characters (byte)
		getVGAColourDepth(),				// Colour depth
		videoMode,							// The video mode number
	};
	send_packet(PACKET_MODE, sizeof packet, packet);
}

// Copy the AGON font data from Flash to RAM
//
void copy_font() {
	memcpy(fabgl::FONT_AGON_DATA + 256, fabgl::FONT_AGON_BITMAP, sizeof(fabgl::FONT_AGON_BITMAP));
}

// Redefine a character in the font
//
void redefineCharacter(uint8_t c, uint8_t * data) {
	memcpy(&fabgl::FONT_AGON_DATA[c * 8], data, 8);
}

bool cmpChar(uint8_t * c1, uint8_t *c2, uint8_t len) {
	for (uint8_t i = 0; i < len; i++) {
		if (*c1++ != *c2++) {
			return false;
		}
	}
	return true;
}

// Try and match a character at given pixel position
//
char getScreenChar(uint16_t px, uint16_t py) {
	RGB888	pixel;
	uint8_t	charRow;
	uint8_t	charData[8];
	uint8_t R = tbg.R;
	uint8_t G = tbg.G;
	uint8_t B = tbg.B;

	// Do some bounds checking first
	//
	if (px < 0 || py < 0 || px >= canvasW - 8 || py >= canvasH - 8) {
		return 0;
	}

	// Now scan the screen and get the 8 byte pixel representation in charData
	//
	for (uint8_t y = 0; y < 8; y++) {
		charRow = 0;
		for (uint8_t x = 0; x < 8; x++) {
			pixel = canvas->getPixel(px + x, py + y);
			if (!(pixel.R == R && pixel.G == G && pixel.B == B)) {
				charRow |= (0x80 >> x);
			}
		}
		charData[y] = charRow;
	}
	//
	// Finally try and match with the character set array
	//
	for (uint8_t i = 32; i <= 255; i++) {
		if (cmpChar(charData, &fabgl::FONT_AGON_DATA[i * 8], 8)) {	
			return i;		
		}
	}
	return 0;
}

// Get pixel value at screen coordinates
//
RGB888 getPixel(uint16_t x, uint16_t y) {
	Point p = translateViewport(scale(x, y));
	if (p.X >= 0 && p.Y >= 0 && p.X < canvasW && p.Y < canvasH) {
		return canvas->getPixel(p.X, p.Y);
	}
	return RGB888(0,0,0);
}

// Get the palette index for a given RGB888 colour
//
uint8_t getPaletteIndex(RGB888 colour) {
	for (uint8_t i = 0; i < getVGAColourDepth(); i++) {
		if (colourLookup[palette[i]] == colour) {
			return i;
		}
	}
	return 0;
}

// Set logical palette
// Parameters:
// - l: The logical colour to change
// - p: The physical colour to change
// - r: The red component
// - g: The green component
// - b: The blue component
//
void setPalette(uint8_t l, uint8_t p, uint8_t r, uint8_t g, uint8_t b) {
	RGB888 col;				// The colour to set

	if (getVGAColourDepth() < 64) {		// If it is a paletted video mode
		if (p == 255) {					// If p = 255, then use the RGB values
			col = RGB888(r, g, b);
		} else if (p < 64) {			// If p < 64, then look the value up in the colour lookup table
			col = colourLookup[p];
		} else {				
			debug_log("vdu_palette: p=%d not supported\n\r", p);
			return;
		}
		setPaletteItem(l, col);
		waitPlotCompletion();
		debug_log("vdu_palette: %d,%d,%d,%d,%d\n\r", l, p, r, g, b);
	} else {
		debug_log("vdu_palette: not supported in this mode\n\r");
	}
}

// Reset the palette and set the foreground and background drawing colours
// Parameters:
// - colour: Array of indexes into colourLookup table
// - sizeOfArray: Size of passed colours array
//
void resetPalette(const uint8_t colours[]) {
	for (uint8_t i = 0; i < 64; i++) {
		uint8_t c = colours[i % getVGAColourDepth()];
		palette[i] = c;
		setPaletteItem(i, colourLookup[c]);
	}
	updateRGB2PaletteLUT();
}

// Get the paint options for a given GCOL mode
//
fabgl::PaintOptions getPaintOptions(uint8_t mode, fabgl::PaintOptions priorPaintOptions) {
	fabgl::PaintOptions p = priorPaintOptions;
	
	switch (mode) {
		case 0: p.NOT = 0; p.swapFGBG = 0; break;
		case 4: p.NOT = 1; p.swapFGBG = 0; break;
	}
	return p;
}

// Set text colour (handles COLOUR / VDU 17)
//
void setTextColour(uint8_t colour) {
	uint8_t c = palette[colour % getVGAColourDepth()];

	if (colour >= 0 && colour < 64) {
		tfg = colourLookup[c];
		debug_log("vdu_colour: tfg %d = %02X : %02X,%02X,%02X\n\r", colour, c, tfg.R, tfg.G, tfg.B);
	}
	else if (colour >= 128 && colour < 192) {
		tbg = colourLookup[c];
		debug_log("vdu_colour: tbg %d = %02X : %02X,%02X,%02X\n\r", colour, c, tbg.R, tbg.G, tbg.B);	
	}
	else {
		debug_log("vdu_colour: invalid colour %d\n\r", colour);
	}
}

// Set graphics colour (handles GCOL / VDU 18)
//
void setGraphicsColour(uint8_t mode, uint8_t colour) {
	uint8_t c = palette[colour % getVGAColourDepth()];

	if (mode >= 0 && mode <= 6) {
		if (colour >= 0 && colour < 64) {
			gfg = colourLookup[c];
			debug_log("vdu_gcol: mode %d, gfg %d = %02X : %02X,%02X,%02X\n\r", mode, colour, c, gfg.R, gfg.G, gfg.B);
		}
		else if (colour >= 128 && colour < 192) {
			gbg = colourLookup[c];
			debug_log("vdu_gcol: mode %d, gbg %d = %02X : %02X,%02X,%02X\n\r", mode, colour, c, gbg.R, gbg.G, gbg.B);
		}
		else {
			debug_log("vdu_gcol: invalid colour %d\n\r", colour);
		}
		gpo = getPaintOptions(mode, gpo);
	}
	else {
		debug_log("vdu_gcol: invalid mode %d\n\r", mode);
	}
}

// Clear a viewport
//
void clearViewport(Rect * viewport) {
	if (canvas) {
		if (useViewports) {
			canvas->fillRectangle(*viewport);
		}
		else {
			canvas->clear();
		}
	}
}

//// Graphics drawing routines

// Push point to list
//
void pushPoint(Point p) {
	p3 = p2;
	p2 = p1;
	p1 = p;
}
void pushPoint(uint16_t x, uint16_t y) {
	pushPoint(translateViewport(scale(x, y)));
}

// get graphics cursor
//
Point * getGraphicsCursor() {
	return &p1;
}

// Set up canvas for drawing graphics
//
void setGraphicsOptions() {
	canvas->setClippingRect(graphicsViewport);
	canvas->setPenColor(gfg);
	canvas->setPaintOptions(gpo);
}

// Move to
//
void moveTo() {
	canvas->moveTo(p1.X, p1.Y);
}

// Line plot
//
void plotLine() {
	canvas->lineTo(p1.X, p1.Y);
}

// Point point
//
void plotPoint() {
	canvas->setPixel(p1.X, p1.Y);
}

// Triangle plot
//
void plotTriangle(uint8_t mode) {
	Point p[3] = {
		p3,
		p2,
		p1, 
	};
	canvas->setBrushColor(gfg);
	canvas->fillPath(p, 3);
	canvas->setBrushColor(tbg);
}

// Circle plot
//
void plotCircle(uint8_t mode) {
	switch (mode) {
		case 0x00 ... 0x03: { // Circle
			auto r = 2 * (p1.X + p1.Y);
			canvas->drawEllipse(p2.X, p2.Y, r, r);
		} break;
		case 0x04 ... 0x07: { // Circle
			auto a = p2.X - p1.X;
			auto b = p2.Y - p1.Y;
			auto r = 2 * sqrt(a * a + b * b);
			canvas->drawEllipse(p2.X, p2.Y, r, r);
		} break;
	}
}

// Character plot
//
void plotCharacter(char c) {
	if (textCursorActive()) {
		canvas->setClippingRect(defaultViewport);
		canvas->setPenColor(tfg);
		canvas->setBrushColor(tbg);
		canvas->setPaintOptions(tpo);
	}
	else {
		canvas->setClippingRect(graphicsViewport);
		canvas->setPenColor(gfg);
		canvas->setPaintOptions(gpo);
	}
	canvas->drawChar(activeCursor->X, activeCursor->Y, c);
	cursorRight();
}

// Backspace plot
//
void plotBackspace() {
	cursorLeft();
	canvas->setBrushColor(textCursorActive() ? tbg : gbg);
	canvas->fillRectangle(activeCursor->X, activeCursor->Y, activeCursor->X + fontW - 1, activeCursor->Y + fontH - 1);
}

// Set character overwrite mode (background fill)
//
inline void setCharacterOverwrite(bool overwrite) {
	canvas->setGlyphOptions(GlyphOptions().FillBackground(overwrite));
}

// Set a clipping rectangle
//
void setClippingRect(Rect rect) {
	canvas->setClippingRect(rect);
}

// Draw cursor
//
void drawCursor(Point p) {
	canvas->swapRectangle(p.X, p.Y, p.X + fontW - 1, p.Y + fontH - 1);
}


// Clear the screen
// 
void cls(bool resetViewports) {
	if (resetViewports) {
		viewportReset();
	}
	if (canvas) {
		canvas->setPenColor(tfg);
		canvas->setBrushColor(tbg);	
		canvas->setPaintOptions(tpo);
		clearViewport(getViewport(VIEWPORT_TEXT));
	}
	if (hasActiveSprites()) {
		activateSprites(0);
		clearViewport(getViewport(VIEWPORT_TEXT));
	}
	homeCursor();
	setPagedMode();
}

// Clear the graphics area
//
void clg() {
	if (canvas) {
		canvas->setPenColor(gfg);
		canvas->setBrushColor(gbg);	
		canvas->setPaintOptions(gpo);
		clearViewport(getViewport(VIEWPORT_GRAPHICS));
	}
	pushPoint(0, 0);		// Reset graphics origin (as per BBC Micro CLG)
}

// Do the mode change
// Parameters:
// - mode: The video mode
// Returns:
// -  0: Successful
// -  1: Invalid # of colours
// -  2: Not enough memory for mode
// - -1: Invalid mode
//
int8_t change_mode(uint8_t mode) {
	int8_t errVal = -1;

	doubleBuffered = false;			// Default is to not double buffer the display

	cls(true);
	switch (mode) {
		case 0:
			if (legacyModes == true) {
				errVal = change_resolution(2, SVGA_1024x768_60Hz);
			} else {
				errVal = change_resolution(16, VGA_640x480_60Hz);	// VDP 1.03 Mode 3, VGA Mode 12h
			}
			break;
		case 1:
			if (legacyModes == true) {
				errVal = change_resolution(16, VGA_512x384_60Hz);
			} else {
				errVal = change_resolution(4, VGA_640x480_60Hz);
			}
			break;
		case 2:
			if (legacyModes == true) {
				errVal = change_resolution(64, VGA_320x200_75Hz);
			} else {
				errVal = change_resolution(2, VGA_640x480_60Hz);
			}
			break;
		case 3:
			if (legacyModes == true) {
				errVal = change_resolution(16, VGA_640x480_60Hz);
			} else {
				errVal = change_resolution(64, VGA_640x240_60Hz);
			}
			break;
		case 4:
			errVal = change_resolution(16, VGA_640x240_60Hz);
			break;
		case 5:
			errVal = change_resolution(4, VGA_640x240_60Hz);
			break;
		case 6:
			errVal = change_resolution(2, VGA_640x240_60Hz);
			break;
		case 8:
			errVal = change_resolution(64, QVGA_320x240_60Hz);		// VGA "Mode X"
			break;
		case 9:
			errVal = change_resolution(16, QVGA_320x240_60Hz);
			break;
		case 10:
			errVal = change_resolution(4, QVGA_320x240_60Hz);
			break;
		case 11:
			errVal = change_resolution(2, QVGA_320x240_60Hz);
			break;
		case 12:
			errVal = change_resolution(64, VGA_320x200_70Hz);		// VGA Mode 13h
			break;
		case 13:
			errVal = change_resolution(16, VGA_320x200_70Hz);
			break;
		case 14:
			errVal = change_resolution(4, VGA_320x200_70Hz);
			break;
		case 15:
			errVal = change_resolution(2, VGA_320x200_70Hz);
			break;
		case 16:
			errVal = change_resolution(4, SVGA_800x600_60Hz);
			break;
		case 17:
			errVal = change_resolution(2, SVGA_800x600_60Hz);
			break;
		case 18:
			errVal = change_resolution(2, SVGA_1024x768_60Hz);		// VDP 1.03 Mode 0
			break;
		case 129:
			doubleBuffered = true;
			errVal = change_resolution(4, VGA_640x480_60Hz);
			break;
		case 130:
			doubleBuffered = true;
			errVal = change_resolution(2, VGA_640x480_60Hz);
			break;
		case 132:
			doubleBuffered = true;
			errVal = change_resolution(16, VGA_640x240_60Hz);
			break;
		case 133:
			doubleBuffered = true;
			errVal = change_resolution(4, VGA_640x240_60Hz);
			break;
		case 134:
			doubleBuffered = true;
			errVal = change_resolution(2, VGA_640x240_60Hz);
			break;
		case 136:
			doubleBuffered = true;
			errVal = change_resolution(64, QVGA_320x240_60Hz);		// VGA "Mode X"
			break;
		case 137:
			doubleBuffered = true;
			errVal = change_resolution(16, QVGA_320x240_60Hz);
			break;
		case 138:
			doubleBuffered = true;
			errVal = change_resolution(4, QVGA_320x240_60Hz);
			break;	
		case 139:
			doubleBuffered = true;
			errVal = change_resolution(2, QVGA_320x240_60Hz);
			break;
		case 140:
			doubleBuffered = true;
			errVal = change_resolution(64, VGA_320x200_70Hz);		// VGA Mode 13h
			break;
		case 141:
			doubleBuffered = true;
			errVal = change_resolution(16, VGA_320x200_70Hz);
			break;
		case 142:
			doubleBuffered = true;
			errVal = change_resolution(4, VGA_320x200_70Hz);
			break;
		case 143:
			doubleBuffered = true;
			errVal = change_resolution(2, VGA_320x200_70Hz);
			break;									
	}
	if (errVal != 0) {
		return errVal;
	}
	switch (getVGAColourDepth()) {
		case  2: resetPalette(defaultPalette02); break;
		case  4: resetPalette(defaultPalette04); break;
		case  8: resetPalette(defaultPalette08); break;
		case 16: resetPalette(defaultPalette10); break;
		case 64: resetPalette(defaultPalette40); break;
	}
	tpo = getPaintOptions(0, tpo);
	gpo = getPaintOptions(0, gpo);
	gfg = colourLookup[0x3F];
	gbg = colourLookup[0x00];
	tfg = colourLookup[0x3F];
	tbg = colourLookup[0x00];
	canvas->selectFont(&fabgl::FONT_AGON);
	setCharacterOverwrite(true);
	canvas->setPenWidth(1);
	setOrigin(0,0);
	pushPoint(0,0);
	pushPoint(0,0);
	pushPoint(0,0);
	setCanvasWH(canvas->getWidth(), canvas->getHeight());
	fontW = canvas->getFontInfo()->width;
	fontH = canvas->getFontInfo()->height;
	viewportReset();
	resetCursor();
	homeCursor();
	sendModeInformation();
	debug_log("do_modeChange: canvas(%d,%d), scale(%f,%f), mode %d, videoMode %d\n\r", canvasW, canvasH, logicalScaleX, logicalScaleY, mode, videoMode);
	return 0;
}

// Change the video mode
// If there is an error, restore the last mode
// Parameters:
// - mode: The video mode
//
void set_mode(uint8_t mode) {
	auto errVal = change_mode(mode);
	if (errVal != 0) {
		debug_log("set_mode: error %d\n\r", errVal);
		errVal = change_mode(videoMode);
		if (errVal != 0) {
			debug_log("set_mode: error %d on restoring mode\n\r", errVal);
			videoMode = 1;
			change_mode(1);
		}
		return;
	}
	videoMode = mode;
}

void setLegacyModes(bool legacy) {
	legacyModes = legacy;
}

void scrollRegion(Rect * region, uint8_t direction, int16_t movement) {
	canvas->setScrollingRegion(region->X1, region->Y1, region->X2, region->Y2);

	switch (direction) {
		case 0:	// Right
			canvas->scroll(movement, 0);
			break;
		case 1: // Left
			canvas->scroll(-movement, 0);
			break;
		case 2: // Down
			canvas->scroll(0, movement);
			break;
		case 3: // Up
			canvas->scroll(0, -movement);
			break;
	}
	waitPlotCompletion();
}

#endif
