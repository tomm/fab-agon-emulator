#include "fabutils.h"
#include "fake_fabgl.h"
#include "fabgl.h"
#include "video.h"
#include "dispdrivers/vga16controller.h"
#include "ps2controller.h"

extern "C" void sendHostKbEventToFabgl(uint8_t ascii, bool isDown)
{
	struct fabgl::VirtualKeyItem k;
	k.ASCII = toupper(ascii);
	k.down = isDown;
	fabgl::PS2Controller::keyboard()->injectVirtualKey(k, false);
}

/* Buffer must be big enough for any screen resolution - up to 1024x768x3 bytes :) */
extern "C" void copyVgaFramebuffer(int *outWidth, int *outHeight, void *buffer)
{
	auto vga = fabgl::VGA16Controller::instance();
	const int w = vga->getScreenWidth();
	const int h = vga->getScreenHeight();
	*outHeight = h;
	*outWidth = w;
	// rect is inclusive range
	Rect rect(0, 0, w-1, h-1);
	vga->readScreen(rect, (fabgl::RGB888*)buffer);
}

extern "C" void vdp_setup() {
	setup();
}

extern "C" void vdp_loop() {
	loop();
}
