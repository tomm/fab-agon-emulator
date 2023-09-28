#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <fabgl.h>

#include "agon.h"
#include "graphics.h"

extern void setClippingRect(Rect r);

Point			origin;							// Screen origin
uint16_t		canvasW;						// Canvas width
uint16_t		canvasH;						// Canvas height
bool			logicalCoords = true;			// Use BBC BASIC logical coordinates
double			logicalScaleX;					// Scaling factor for logical coordinates
double			logicalScaleY;
Rect *			activeViewport;					// Pointer to the active text viewport (textViewport or graphicsViewport)
Rect			defaultViewport;				// Default viewport
Rect			textViewport;					// Text viewport
Rect			graphicsViewport;				// Graphics viewport
bool			useViewports = false;			// Viewports are enabled

// Reset viewports to default
//
void viewportReset() {
	defaultViewport = Rect(0, 0, canvasW - 1, canvasH - 1);
	textViewport =	Rect(0, 0, canvasW - 1, canvasH - 1);
	graphicsViewport = Rect(0, 0, canvasW - 1, canvasH - 1);
	activeViewport = &textViewport;
	useViewports = false;
}

Rect * getViewport(uint8_t type) {
	switch (type) {
		case VIEWPORT_TEXT: return &textViewport;
		case VIEWPORT_DEFAULT: return &defaultViewport;
		case VIEWPORT_GRAPHICS: return &graphicsViewport;
		case VIEWPORT_ACTIVE: return activeViewport;
		default: return &defaultViewport;
	}
}

void setActiveViewport(uint8_t type) {
	activeViewport = getViewport(type);
}

// Translate a point relative to the graphics viewport
//
Point translateViewport(int16_t X, int16_t Y) {
	if (logicalCoords) {
		return Point(graphicsViewport.X1 + (origin.X + X), graphicsViewport.Y2 - (origin.Y + Y));
	}
	return Point(graphicsViewport.X1 + (origin.X + X), graphicsViewport.Y1 + (origin.Y + Y));
}
Point translateViewport(Point p) {
	return translateViewport(p.X, p.Y);
}

// Scale a point
//
Point scale(int16_t X, int16_t Y) {
	if (logicalCoords) {
		return Point((double)X / logicalScaleX, (double)Y / logicalScaleY);
	}
	return Point(X, Y);
}
Point scale(Point p) {
	return scale(p.X, p.Y);
}

// Translate a point relative to the canvas
//
Point translateCanvas(int16_t X, int16_t Y) {
	if (logicalCoords) {
		return Point(origin.X + X, (canvasH - 1) - (origin.Y + Y));
	}
	return Point(origin.X + X, origin.Y + Y);
}
Point translateCanvas(Point p) {
	return translateCanvas(p.X, p.Y);
}

// Set graphics viewport
bool setGraphicsViewport(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
	auto p1 = translateCanvas(scale(x1, y1));
	auto p2 = translateCanvas(scale(x2, y2));

	if (p1.X >= 0 && p2.X < canvasW && p1.Y >= 0 && p2.Y < canvasH && p2.X > p1.X && p2.Y > p1.Y) {
		graphicsViewport = Rect(p1.X, p1.Y, p2.X, p2.Y);
		useViewports = true;
		setClippingRect(graphicsViewport);
		return true;
	}
	return false;
}

// Set text viewport
bool setTextViewport(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
	if (x2 >= canvasW) x2 = canvasW - 1;
	if (y2 >= canvasH) y2 = canvasH - 1;

	if (x2 > x1 && y2 > y1) {
		textViewport = Rect(x1, y1, x2, y2);
		useViewports = true;
		return true;
	}
	return false;
}

inline void setOrigin(int x, int y) {
	origin = scale(x, y);
}

inline void setCanvasWH(int w, int h) {
	canvasW = w;
	canvasH = h;
	logicalScaleX = LOGICAL_SCRW / (double)canvasW;
	logicalScaleY = LOGICAL_SCRH / (double)canvasH;
}

inline void setLogicalCoords(bool b) {
	logicalCoords = b;
}

#endif // VIEWPORT_H
