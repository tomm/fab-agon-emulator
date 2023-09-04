#ifndef AGON_SCREEN_H
#define AGON_SCREEN_H

#include <fabgl.h>

fabgl::Canvas *				canvas;				// The canvas class

fabgl::VGA2Controller		_VGAController2;	// VGA class - 2 colours
fabgl::VGA4Controller		_VGAController4;	// VGA class - 4 colours
fabgl::VGA8Controller		_VGAController8;	// VGA class - 8 colours
fabgl::VGA16Controller		_VGAController16;	// VGA class - 16 colours
fabgl::VGAController		_VGAController64;	// VGA class - 64 colours

fabgl::VGABaseController *	_VGAController;		// Pointer to the current VGA controller class (one of the above)

uint8_t			_VGAColourDepth;				// Number of colours per pixel (2, 4, 8, 16 or 64)
bool			doubleBuffered = false;			// Disable double buffering by default


// Get controller
// Parameters:
// - colours: Number of colours per pixel (2, 4, 8, 16 or 64)
// Returns:
// - A singleton instance of a VGAController class
//
fabgl::VGABaseController * getVGAController(uint8_t colours = _VGAColourDepth) {
	switch (colours) {
		case  2: return _VGAController2.instance();
		case  4: return _VGAController4.instance();
		case  8: return _VGAController8.instance();
		case 16: return _VGAController16.instance();
		case 64: return _VGAController64.instance();
	}
	return nullptr;
}

// Update the internal FabGL LUT
//
void updateRGB2PaletteLUT() {
	switch (_VGAColourDepth) {
		case 2: _VGAController2.updateRGB2PaletteLUT(); break;
		case 4: _VGAController4.updateRGB2PaletteLUT(); break;
		case 8: _VGAController8.updateRGB2PaletteLUT(); break;
		case 16: _VGAController16.updateRGB2PaletteLUT(); break;
	}
}

// Get current colour depth
//
inline uint8_t getVGAColourDepth() {
	return _VGAColourDepth;
}

// Set a palette item
// Parameters:
// - l: The logical colour to change
// - c: The new colour
// 
void setPaletteItem(uint8_t l, RGB888 c) {
	auto depth = getVGAColourDepth();
	if (l < depth) {
		switch (depth) {
			case 2: _VGAController2.setPaletteItem(l, c); break;
			case 4: _VGAController4.setPaletteItem(l, c); break;
			case 8: _VGAController8.setPaletteItem(l, c); break;
			case 16: _VGAController16.setPaletteItem(l, c); break;
		}
	}
}

// Change video resolution
// Parameters:
// - colours: Number of colours per pixel (2, 4, 8, 16 or 64)
// - modeLine: A modeline string (see the FagGL documentation for more details)
// Returns:
// - 0: Successful
// - 1: Invalid # of colours
// - 2: Not enough memory for mode
//
int8_t change_resolution(uint8_t colours, char * modeLine) {
	auto controller = getVGAController(colours);

	if (controller == nullptr) {					// If controller is null, then an invalid # of colours was passed
		return 1;									// So return the error
	}
	delete canvas;									// Delete the canvas

	_VGAColourDepth = colours;						// Set the number of colours per pixel
	if (_VGAController != controller) {				// Is it a different controller?
		if (_VGAController) {						// If there is an existing controller running then
			_VGAController->end();					// end it
		}
		_VGAController = controller;				// Switch to the new controller
		controller->begin();						// And spin it up
	}
	if (modeLine) {									// If modeLine is not a null pointer then
		if (doubleBuffered == true) {
			controller->setResolution(modeLine, -1, -1, 1);
		} else {
			controller->setResolution(modeLine);	// Set the resolution
		}
	} else {
		debug_log("change_resolution: modeLine is null\n\r");
	}
	controller->enableBackgroundPrimitiveExecution(true);
	controller->enableBackgroundPrimitiveTimeout(false);

	canvas = new fabgl::Canvas(controller);			// Create the new canvas
	debug_log("after change of canvas...\n\r");
	debug_log("  free internal: %d\n\r  free 8bit: %d\n\r  free 32bit: %d\n\r",
		heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
		heap_caps_get_free_size(MALLOC_CAP_8BIT),
		heap_caps_get_free_size(MALLOC_CAP_32BIT)
	);
	//
	// Check whether the selected mode has enough memory for the vertical resolution
	//
	if (controller->getScreenHeight() != controller->getViewPortHeight()) {
		return 2;
	}
	return 0;										// Return with no errors
}

// Swap to other buffer if we're in a double-buffered mode
//
void switchBuffer() {
	if (doubleBuffered == true) {
		canvas->swapBuffers();
	}
}

// Wait for plot completion
//
inline void waitPlotCompletion(bool waitForVSync = false) {
	canvas->waitCompletion(waitForVSync);
}

#endif // AGON_SCREEN_H
