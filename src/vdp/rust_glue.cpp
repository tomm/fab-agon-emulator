#include "fabutils.h"
#include "fake_fabgl.h"
#include "fabgl.h"
#include "userspace-vdp-gl/src/displaycontroller.h"
#include "userspace-vdp-gl/src/dispdrivers/vgabasecontroller.h"
#include "userspace-vdp-gl/src/userspace-platform/fake_fabgl.h"
#include "vdp.h"
#include "dispdrivers/vga16controller.h"
#include "dispdrivers/vgabasecontroller.h"
#include "userspace-vdp-gl/src/comdrivers/ps2controller.h"

// Arduino.h
extern void delay(int ms);
// HardwareSerial.h
extern HardwareSerial Serial2;

bool vdp_debug_logging = false;
uint32_t startup_screen_mode = 0;

extern "C" bool z80_uart0_is_cts()
{
	return Serial2.isReadyToReceive();
}

extern "C" void z80_send_to_vdp(uint8_t b)
{
	Serial2.writeToInQueue(b);
}

extern "C" bool z80_recv_from_vdp(uint8_t *out)
{
	return Serial2.readFromOutQueue(out);
}

extern "C" void signal_vblank(void)
{
	fabgl::BaseDisplayController::vblankSignal.store(true);
}

extern "C" void setVdpDebugLogging(bool state)
{
	vdp_debug_logging = state;
}

extern "C" void set_startup_screen_mode(uint32_t mode)
{
	startup_screen_mode = mode;
}

/* ps2scancode is the set2 'make' code */
extern "C" void sendPS2KbEventToFabgl(uint16_t ps2scancode, uint8_t isDown)
{
	if (fabgl::PS2Controller::keyboard() != nullptr) {
		if (ps2scancode == 0x62) {
			if (isDown) {
				/* VK_PAUSE hack */
				fabgl::VirtualKeyItem item;
				item.down = 1;
				item.vk = fabgl::VK_PAUSE;
				fabgl::PS2Controller::keyboard()->injectVirtualKey(item, true);
			}
		} else {
			fabgl::PS2Controller::keyboard()->injectScancode(ps2scancode, isDown);
		}
	}
}

extern "C" void sendVKeyEventToFabgl(uint32_t vkey, uint8_t isDown)
{
	if (fabgl::PS2Controller::keyboard() != nullptr) {
		fabgl::PS2Controller::keyboard()->injectVirtualKey((fabgl::VirtualKey)vkey, isDown);
	}
}

extern "C" void sendHostMouseEventToFabgl(uint8_t mousePacket[4])
{
	fabgl::MousePacket packet;
	packet.data[0] = mousePacket[0];
	packet.data[1] = mousePacket[1];
	packet.data[2] = mousePacket[2];
	packet.data[3] = mousePacket[3];

	auto lock = fabgl::VGABaseController::acquireLock();
	// if the VGA controller is initialized we know the mouse is ready
	if (fabgl::VGABaseController::activeController != nullptr
			&& fabgl::PS2Controller::mouse() != nullptr) {
		fabgl::PS2Controller::mouse()->injectPacket(&packet);
	}
}

/* Buffer must be big enough for any screen resolution - up to 1024x768x3 bytes :) */
extern "C" void copyVgaFramebuffer(int *outWidth, int *outHeight, void *buffer, float *frameRateHz)
{
	auto lock = fabgl::VGABaseController::acquireLock();
	fabgl::VGABaseController *vga = fabgl::VGABaseController::activeController;

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

	if (!vga->is_started()) {
		// this can arise as a race condition, as the lock in 
		// setResolution is dropped briefly between end() and the rest of the function.
		*frameRateHz = 60;
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

	{
		auto timings = vga->getResolutionTimings();
		const int16_t hlen = timings->HVisibleArea + timings->HFrontPorch + timings->HSyncPulse + timings->HBackPorch;
		const int16_t vlen = timings->VVisibleArea + timings->VFrontPorch + timings->VSyncPulse + timings->VBackPorch;
		const int32_t totalPixels = (int32_t)hlen * (int32_t)vlen * (int32_t)timings->scanCount;
		*frameRateHz = (float)timings->frequency / (float)totalPixels;
	}
}

extern "C" void getAudioSamples(uint8_t *buffer, uint32_t length)
{
  auto lock = std::unique_lock<std::mutex>(soundGeneratorMutex);
  auto gen = getVDPSoundGenerator();
  for (uint32_t i = 0; i < length; ++i) {
    buffer[i] = gen ? gen->getSample() + 127 : 0;
  }
}

extern "C" void vdp_setup() {
	init_userspace_fabgl();
	setup();
}

extern "C" void vdp_loop() {
	while (!is_fabgl_terminating) {
		loop();
	}
}

extern "C" void vdp_shutdown() {
	is_fabgl_terminating = true;
}

extern "C" void dump_vdp_mem_stats() {
	malloc_wrapper_dump_stats();	
}
