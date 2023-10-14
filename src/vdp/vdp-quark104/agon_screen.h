#ifndef AGON_SCREEN_H
#define AGON_SCREEN_H

#include <memory>
#include <fabgl.h>

std::unique_ptr<fabgl::Canvas>	canvas;			// The canvas class
std::unique_ptr<fabgl::VGABaseController>	_VGAController;		// Pointer to the current VGA controller class (one of the above)

uint8_t			_VGAColourDepth = -1;			// Number of colours per pixel (2, 4, 8, 16 or 64)

// Get a new VGA controller
// Parameters:
// - colours: Number of colours per pixel (2, 4, 8, 16 or 64)
// Returns:
// - A singleton instance of a VGAController class
//
std::unique_ptr<fabgl::VGABaseController> getVGAController(uint8_t colours) {
	switch (colours) {
		case  2: return std::move(std::unique_ptr<fabgl::VGA2Controller>(new fabgl::VGA2Controller()));
		case  4: return std::move(std::unique_ptr<fabgl::VGA4Controller>(new fabgl::VGA4Controller()));
		case  8: return std::move(std::unique_ptr<fabgl::VGA8Controller>(new fabgl::VGA8Controller()));
		case 16: return std::move(std::unique_ptr<fabgl::VGA16Controller>(new fabgl::VGA16Controller()));
		case 64: return std::move(std::unique_ptr<fabgl::VGAController>(new fabgl::VGAController()));
	}
	return nullptr;
}

// Update the internal FabGL LUT
//
void updateRGB2PaletteLUT() {
	// Use instance, as call not present on VGABaseController
	switch (_VGAColourDepth) {
		case 2: fabgl::VGA2Controller::instance()->updateRGB2PaletteLUT(); break;
		case 4: fabgl::VGA4Controller::instance()->updateRGB2PaletteLUT(); break;
		case 8: fabgl::VGA8Controller::instance()->updateRGB2PaletteLUT(); break;
		case 16: fabgl::VGA16Controller::instance()->updateRGB2PaletteLUT(); break;
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
		// Use instance, as call not present on VGABaseController
		switch (depth) {
			case 2: fabgl::VGA2Controller::instance()->setPaletteItem(l, c); break;
			case 4: fabgl::VGA4Controller::instance()->setPaletteItem(l, c); break;
			case 8: fabgl::VGA8Controller::instance()->setPaletteItem(l, c); break;
			case 16: fabgl::VGA16Controller::instance()->setPaletteItem(l, c); break;
		}
	}
}

// Update our VGA controller based on number of colours
// returns true on success, false if the number of colours was invalid
bool updateVGAController(uint8_t colours) {
	if (colours == _VGAColourDepth) {
		return true;
	}

	auto controller = getVGAController(colours);	// Get a new controller
	if (!controller) {
		return false;
	}

	_VGAColourDepth = colours;
	if (_VGAController) {						// If there is an existing controller running then
		_VGAController->end();					// end it
		_VGAController.reset();					// Delete it
	}
	_VGAController = std::move(controller);		// Switch to the new controller
	_VGAController->begin();					// And spin it up
	_VGAController->enableBackgroundPrimitiveExecution(true);
	_VGAController->enableBackgroundPrimitiveTimeout(false);

	return true;
}

// Change video resolution
// Parameters:
// - colours: Number of colours per pixel (2, 4, 8, 16 or 64)
// - modeLine: A modeline string (see the FabGL documentation for more details)
// Returns:
// - 0: Successful
// - 1: Invalid # of colours
// - 2: Not enough memory for mode
//
int8_t change_resolution(uint8_t colours, const char * modeLine, bool doubleBuffered = false) {
	if (!updateVGAController(colours)) {			// If we can't update the controller then
		return 1;									// Return the error
	}

	canvas.reset();									// Delete the canvas

	if (modeLine) {									// If modeLine is not a null pointer then
		_VGAController->setResolution(modeLine, -1, -1, doubleBuffered);	// Set the resolution
	} else {
		debug_log("change_resolution: modeLine is null\n\r");
	}

	canvas.reset(new fabgl::Canvas(_VGAController.get()));		// Create the new canvas
	debug_log("after change of canvas...\n\r");
	debug_log("  free internal: %d\n\r  free 8bit: %d\n\r  free 32bit: %d\n\r",
		heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
		heap_caps_get_free_size(MALLOC_CAP_8BIT),
		heap_caps_get_free_size(MALLOC_CAP_32BIT)
	);
	//
	// Check whether the selected mode has enough memory for the vertical resolution
	//
	if (_VGAController->getScreenHeight() != _VGAController->getViewPortHeight()) {
		return 2;
	}
	return 0;										// Return with no errors
}

// Ask our screen controller if we're double buffered
//
inline bool isDoubleBuffered() {
	return _VGAController->isDoubleBuffered();
}

// Wait for plot completion
//
inline void waitPlotCompletion(bool waitForVSync = false) {
	canvas->waitCompletion(waitForVSync);
}

// Swap to other buffer if we're in a double-buffered mode
// Always waits for VSYNC
//
void switchBuffer() {
	if (isDoubleBuffered()) {
		canvas->swapBuffers();
	} else {
		waitPlotCompletion(true);
	}
}

#endif // AGON_SCREEN_H
