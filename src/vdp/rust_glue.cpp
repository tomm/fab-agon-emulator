#include "fabutils.h"
#include "fake_fabgl.h"
#include "fabgl.h"
#include "vdp.h"
#include "dispdrivers/vga16controller.h"
#include "dispdrivers/vgabasecontroller.h"
#include "ps2controller.h"

// Arduino.h
extern void delay(int ms);

/* ps2scancode is the set2 'make' code */
extern "C" void sendHostKbEventToFabgl(uint16_t ps2scancode, uint8_t isDown)
{
	// get the VGA controller, since if it's initialized we know the keyboard is ready
	fabgl::VGABaseController *vga = getVDPVGAController();
	if (vga) {
		fabgl::PS2Controller::keyboard()->injectScancode(ps2scancode, isDown);
	}
}

/* Buffer must be big enough for any screen resolution - up to 1024x768x3 bytes :) */
extern "C" void copyVgaFramebuffer(int *outWidth, int *outHeight, void *buffer)
{
	fabgl::VGABaseController *vga = getVDPVGAController();

	// the VGA controller may not have been initialized yet -- the rust render 
	// loop may be quicker to start than the VDP setup() function, or the 
	// emulator debugger may have paused the ez80 at startup, delaying VGA 
	// controller init. Handle this gracefully.
	if (vga == nullptr) {
		*outWidth = 640;
		*outHeight = 480;
		memset(buffer, 0, 640*480*3);
		return;
	}

	auto lock = vga->lock();
	if (!vga->is_started()) {
		// this can arise as a race condition, as the lock in 
		// setResolution is dropped briefly between end() and the rest of the function.
		*outWidth = 640;
		*outHeight = 480;
		memset(buffer, 0, 640*480*3);
		return;
	}
	const int w = vga->getScreenWidth();
	const int h = vga->getScreenHeight();
	*outHeight = h;
	*outWidth = w;
	// rect is inclusive range
	Rect rect(0, 0, w-1, h-1);
	vga->readScreen(rect, (fabgl::RGB888*)buffer);
}

extern "C" void getAudioSamples(uint8_t *buffer, uint32_t length)
{
  auto gen = getVDPSoundGenerator();
  for (uint32_t i = 0; i < length; ++i) {
    buffer[i] = gen->getSample() + 127;
  }
}

extern "C" void vdp_setup() {
	setup();
}

extern "C" void vdp_loop() {
	loop();
}

extern "C" void vdp_shutdown() {
	is_fabgl_terminating = true;
}
