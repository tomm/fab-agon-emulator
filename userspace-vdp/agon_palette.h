//
// Title:	        Agon Video BIOS - Palette data
// Author:        	Dean Belfield
// Created:       	25/02/2023
// Last Updated:	30/03/2023
//
// Modinfo:
// 04/03/2023:		Now uses the same level values as FabGL
// 30/03/2023:		Added default colours

#pragma once

// Mode palette defaults
// Indexes into colourLookup
//
static const uint8_t defaultPalette02[0x02] = {
	0x00,	// Black
	0x3F,	// Bright White
};
static const uint8_t defaultPalette04[0x04] = {
	0x00,	// Black
	0x30,	// Bright Red
	0x3C,	// Bright Yellow
	0x3F,	// Bright White
};
static const uint8_t defaultPalette08[0x08] = {
	0x00,	// Black
	0x30,	// Bright Red
	0x0C,	// Bright Green
	0x3C,	// Bright Yellow	
	0x03,	// Bright Blue
	0x33,	// Bright Magenta
	0x0F,	// Bright Cyan
	0x3F,	// Bright White
};
static const uint8_t defaultPalette10[0x10] = {
	0x00, 0x20, 0x08, 0x28, 0x02, 0x22, 0x0A, 0x2A,	// As above, but dark
	0x15, 0x30, 0x0C, 0x3C, 0x03, 0x33, 0x0F, 0x3F,	// As above, but bright
};
static const uint8_t defaultPalette40[0x40] = {
	0x00, 0x20, 0x08, 0x28, 0x02, 0x22, 0x0A, 0x2A,	// First 16 colours are the same as above
	0x15, 0x30, 0x0C, 0x3C, 0x03, 0x33, 0x0F, 0x3F,
	0x01, 0x04, 0x05, 0x06, 0x07, 0x09, 0x0B, 0x0D,	// This is the rest of the colours
	0x0E, 0x10, 0x11, 0x12, 0x13, 0x14, 0x16, 0x17,
	0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x21, 0x23, 0x24, 0x25, 0x26, 0x27, 0x29, 0x2B,
	0x2C, 0x2D, 0x2E, 0x2F, 0x31, 0x32, 0x34, 0x35,
	0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3D, 0x3E,
};

// Colour lookup table
// The four FabGL levels are 0x00, 0x55, 0xAA and 0xFF
//
static const RGB888 colourLookup[64] = {
	RGB888(0x00, 0x00, 0x00), RGB888(0x00, 0x00, 0x55), RGB888(0x00, 0x00, 0xAA), RGB888(0x00, 0x00, 0xFF),
	RGB888(0x00, 0x55, 0x00), RGB888(0x00, 0x55, 0x55), RGB888(0x00, 0x55, 0xAA), RGB888(0x00, 0x55, 0xFF),
	RGB888(0x00, 0xAA, 0x00), RGB888(0x00, 0xAA, 0x55), RGB888(0x00, 0xAA, 0xAA), RGB888(0x00, 0xAA, 0xFF),
	RGB888(0x00, 0xFF, 0x00), RGB888(0x00, 0xFF, 0x55), RGB888(0x00, 0xFF, 0xAA), RGB888(0x00, 0xFF, 0xFF),
	RGB888(0x55, 0x00, 0x00), RGB888(0x55, 0x00, 0x55), RGB888(0x55, 0x00, 0xAA), RGB888(0x55, 0x00, 0xFF),
	RGB888(0x55, 0x55, 0x00), RGB888(0x55, 0x55, 0x55), RGB888(0x55, 0x55, 0xAA), RGB888(0x55, 0x55, 0xFF),
	RGB888(0x55, 0xAA, 0x00), RGB888(0x55, 0xAA, 0x55), RGB888(0x55, 0xAA, 0xAA), RGB888(0x55, 0xAA, 0xFF),
	RGB888(0x55, 0xFF, 0x00), RGB888(0x55, 0xFF, 0x55), RGB888(0x55, 0xFF, 0xAA), RGB888(0x55, 0xFF, 0xFF),
	RGB888(0xAA, 0x00, 0x00), RGB888(0xAA, 0x00, 0x55), RGB888(0xAA, 0x00, 0xAA), RGB888(0xAA, 0x00, 0xFF),
	RGB888(0xAA, 0x55, 0x00), RGB888(0xAA, 0x55, 0x55), RGB888(0xAA, 0x55, 0xAA), RGB888(0xAA, 0x55, 0xFF),
	RGB888(0xAA, 0xAA, 0x00), RGB888(0xAA, 0xAA, 0x55), RGB888(0xAA, 0xAA, 0xAA), RGB888(0xAA, 0xAA, 0xFF),
	RGB888(0xAA, 0xFF, 0x00), RGB888(0xAA, 0xFF, 0x55), RGB888(0xAA, 0xFF, 0xAA), RGB888(0xAA, 0xFF, 0xFF),
	RGB888(0xFF, 0x00, 0x00), RGB888(0xFF, 0x00, 0x55), RGB888(0xFF, 0x00, 0xAA), RGB888(0xFF, 0x00, 0xFF),
	RGB888(0xFF, 0x55, 0x00), RGB888(0xFF, 0x55, 0x55), RGB888(0xFF, 0x55, 0xAA), RGB888(0xFF, 0x55, 0xFF),
	RGB888(0xFF, 0xAA, 0x00), RGB888(0xFF, 0xAA, 0x55), RGB888(0xFF, 0xAA, 0xAA), RGB888(0xFF, 0xAA, 0xFF),
	RGB888(0xFF, 0xFF, 0x00), RGB888(0xFF, 0xFF, 0x55), RGB888(0xFF, 0xFF, 0xAA), RGB888(0xFF, 0xFF, 0xFF),
};