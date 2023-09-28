#ifndef VDU_SYS_H
#define VDU_SYS_H

#include <fabgl.h>
#include <ESP32Time.h>

#include "agon.h"
#include "agon_keyboard.h"
#include "cursor.h"
#include "graphics.h"
#include "vdu_audio.h"
#include "vdu_buffered.h"
#include "vdu_sprites.h"

extern void switchTerminalMode();				// Switch to terminal mode

bool			initialised = false;			// Is the system initialised yet?
ESP32Time		rtc(0);							// The RTC

// Buffer for serialised time
//
typedef union {
	struct {
		uint32_t month : 4;
		uint32_t day : 5;
		uint32_t dayOfWeek : 3;
		uint32_t dayOfYear : 9;
		uint32_t hour : 5;
		uint32_t minute : 6;
		uint8_t  second;
		uint8_t  year;
	};
	uint8_t packet[6];
} vdp_time_t;

// Wait for eZ80 to initialise
//
void VDUStreamProcessor::wait_eZ80() {
	debug_log("wait_eZ80: Start\n\r");
	while (!initialised) {
		if (byteAvailable()) {
			auto c = readByte();	// Only handle VDU 23 packets
			if (c == 23) {
				vdu_sys();
			}
		}
	}
	debug_log("wait_eZ80: End\n\r");
}

// Handle SYS
// VDU 23,mode
//
void VDUStreamProcessor::vdu_sys() {
	auto mode = readByte_t();

	//
	// If mode is -1, then there's been a timeout
	//
	if (mode == -1) {
		return;
	}
	//
	// If mode < 32, then it's a system command
	//
	else if (mode < 32) {
		switch (mode) {
			case 0x00: {					// VDU 23, 0
	  			vdu_sys_video();			// Video system control
			}	break;
			case 0x01: {					// VDU 23, 1
				auto b = readByte_t();		// Cursor control
				if (b >= 0) {
					enableCursor((bool) b);
				}
			}	break;
			case 0x07: {					// VDU 23, 7
				vdu_sys_scroll();			// Scroll 
			}	break;
			case 0x10: {					// VDU 23, 16
				vdu_sys_cursorBehaviour();	// Set cursor behaviour
			}	break;
			case 0x1B: {					// VDU 23, 27
				vdu_sys_sprites();			// Sprite system control
			}	break;
			case 0x1C: {					// VDU 23, 28
				vdu_sys_hexload();
			}	break;
		}
	}
	//
	// Otherwise, do
	// VDU 23,mode,n1,n2,n3,n4,n5,n6,n7,n8
	// Redefine character with ASCII code mode
	//
	else {
		vdu_sys_udg(mode);
	}
}

// VDU 23,0: VDP control
// These can send responses back; the response contains a packet # that matches the VDU command mode byte
//
void VDUStreamProcessor::vdu_sys_video() {
	auto mode = readByte_t();

	switch (mode) {
		case VDP_GP: {					// VDU 23, 0, &80
			sendGeneralPoll();			// Send a general poll packet
		}	break;
		case VDP_KEYCODE: {				// VDU 23, 0, &81, layout
			vdu_sys_video_kblayout();
		}	break;
		case VDP_CURSOR: {				// VDU 23, 0, &82
			sendCursorPosition();		// Send cursor position
		}	break;
		case VDP_SCRCHAR: {				// VDU 23, 0, &83, x; y;
			auto x = readWord_t();		// Get character at screen position x, y
			auto y = readWord_t();
			sendScreenChar(x, y);
		}	break;
		case VDP_SCRPIXEL: {			// VDU 23, 0, &84, x; y;
			auto x = readWord_t();		// Get pixel value at screen position x, y
			auto y = readWord_t();
			sendScreenPixel((short)x, (short)y);
		}	break;		
		case VDP_AUDIO: {				// VDU 23, 0, &85, channel, command, <args>
			vdu_sys_audio();
		}	break;
		case VDP_MODE: {				// VDU 23, 0, &86
			sendModeInformation();		// Send mode information (screen dimensions, etc)
		}	break;
		case VDP_RTC: {					// VDU 23, 0, &87, mode
			vdu_sys_video_time();		// Send time information
		}	break;
		case VDP_KEYSTATE: {			// VDU 23, 0, &88, repeatRate; repeatDelay; status
			vdu_sys_keystate();
		}	break;
		case VDP_BUFFERED: {			// VDU 23, 0, &A0, bufferId; command, <args>
			vdu_sys_buffered();
		}	break;
		case VDP_LOGICALCOORDS: {		// VDU 23, 0, &C0, n
			auto b = readByte_t();		// Set logical coord mode
			if (b >= 0) {
				setLogicalCoords((bool) b);	// (0 = off, 1 = on)
			}
		}	break;
		case VDP_LEGACYMODES: {			// VDU 23, 0, &C1, n
			auto b = readByte_t();		// Switch legacy modes on or off
			if (b >= 0) {
				setLegacyModes((bool) b);
			}
		}	break;
		case VDP_SWITCHBUFFER: {		// VDU 23, 0, &C3
			switchBuffer();
		}	break;
		case VDP_TERMINALMODE: {		// VDU 23, 0, &FF
			switchTerminalMode(); 		// Switch to terminal mode
		}	break;
  	}
}

// VDU 23, 0, &80, <echo>: Send a general poll/echo byte back to MOS
//
void VDUStreamProcessor::sendGeneralPoll() {
	auto b = readByte_b();
	uint8_t packet[] = {
		b,
	};
	send_packet(PACKET_GP, sizeof packet, packet);
	initialised = true;	
}

// VDU 23, 0, &81, <region>: Set the keyboard layout
//
void VDUStreamProcessor::vdu_sys_video_kblayout() {
	auto region = readByte_t();			// Fetch the region
	setKeyboardLayout(region);
}

// VDU 23, 0, &82: Send the cursor position back to MOS
//
void VDUStreamProcessor::sendCursorPosition() {
	uint8_t packet[] = {
		(uint8_t) (textCursor.X / fontW),
		(uint8_t) (textCursor.Y / fontH),
	};
	send_packet(PACKET_CURSOR, sizeof packet, packet);	
}
	
// VDU 23, 0, &83 Send a character back to MOS
//
void VDUStreamProcessor::sendScreenChar(uint16_t x, uint16_t y) {
	uint16_t px = x * fontW;
	uint16_t py = y * fontH;
	char c = getScreenChar(px, py);
	uint8_t packet[] = {
		c,
	};
	send_packet(PACKET_SCRCHAR, sizeof packet, packet);
}

// VDU 23, 0, &84: Send a pixel value back to MOS
//
void VDUStreamProcessor::sendScreenPixel(uint16_t x, uint16_t y) {
	RGB888 pixel = getPixel(x, y);
	uint8_t pixelIndex = getPaletteIndex(pixel);
	uint8_t packet[] = {
		pixel.R,	// Send the colour components
		pixel.G,
		pixel.B,
		pixelIndex,	// And the pixel index in the palette
	};
	send_packet(PACKET_SCRPIXEL, sizeof packet, packet);	
}

// VDU 23, 0, &87, 0: Send TIME information (from ESP32 RTC)
//
void VDUStreamProcessor::sendTime() {
	vdp_time_t	time;

	time.year = rtc.getYear() - EPOCH_YEAR;	// 0 - 255
	time.month = rtc.getMonth();			// 0 - 11
	time.day = rtc.getDay();				// 1 - 31
	time.dayOfYear = rtc.getDayofYear();	// 0 - 365
	time.dayOfWeek = rtc.getDayofWeek();	// 0 - 6
	time.hour = rtc.getHour(true);			// 0 - 23
	time.minute = rtc.getMinute();			// 0 - 59
	time.second = rtc.getSecond();			// 0 - 59

	send_packet(PACKET_RTC, sizeof time.packet, time.packet);
}

// VDU 23, 0, &86: Send MODE information (screen details)
//
void VDUStreamProcessor::sendModeInformation() {
	uint8_t packet[] = {
		(uint8_t) (canvasW & 0xFF),			// Width in pixels (L)
		(uint8_t) ((canvasW >> 8) & 0xFF),	// Width in pixels (H)
		(uint8_t) (canvasH & 0xFF),			// Height in pixels (L)
		(uint8_t) ((canvasH >> 8) & 0xFF),	// Height in pixels (H)
		(uint8_t) (canvasW / fontW),		// Width in characters (byte)
		(uint8_t) (canvasH / fontH),		// Height in characters (byte)
		getVGAColourDepth(),				// Colour depth
		videoMode,							// The video mode number
	};
	send_packet(PACKET_MODE, sizeof packet, packet);
}

// VDU 23, 0, &87, <mode>, [args]: Handle time requests
//
void VDUStreamProcessor::vdu_sys_video_time() {
	auto mode = readByte_t();

	if (mode == 0) {
		sendTime();
	}
	else if (mode == 1) {
		auto yr = readByte_t(); if (yr == -1) return;
		auto mo = readByte_t(); if (mo == -1) return;
		auto da = readByte_t(); if (da == -1) return;
		auto ho = readByte_t(); if (ho == -1) return;	
		auto mi = readByte_t(); if (mi == -1) return;
		auto se = readByte_t(); if (se == -1) return;

		yr = EPOCH_YEAR + (int8_t)yr;

		if (yr >= 1970) {
			rtc.setTime(se, mi, ho, da, mo, yr);
		}
	}
}

// Send the keyboard state
//
void VDUStreamProcessor::sendKeyboardState() {
	uint16_t	delay;
	uint16_t	rate;
	uint8_t		ledState;
	getKeyboardState(&delay, &rate, &ledState);
	uint8_t		packet[] = {
		(uint8_t) (delay & 0xFF),
		(uint8_t) ((delay >> 8) & 0xFF),
		(uint8_t) (rate & 0xFF),
		(uint8_t) ((rate >> 8) & 0xFF),
		ledState
	};
	send_packet(PACKET_KEYSTATE, sizeof packet, packet);
}

// VDU 23, 0, &88, delay; repeatRate; LEDs: Handle the keystate
// Send 255 for LEDs to leave them unchanged
//
void VDUStreamProcessor::vdu_sys_keystate() {
	auto delay = readWord_t(); if (delay == -1) return;
	auto rate = readWord_t(); if (rate == -1) return;
	auto ledState = readByte_t(); if (ledState == -1) return;

	setKeyboardState(delay, rate, ledState);
	debug_log("vdu_sys_video: keystate: delay=%d, rate=%d, led=%d\n\r", kbRepeatDelay, kbRepeatRate, ledState);
	sendKeyboardState();
}

// VDU 23,7: Scroll rectangle on screen
//
void VDUStreamProcessor::vdu_sys_scroll() {
	auto extent = readByte_t();		if (extent == -1) return;	// Extent (0 = text viewport, 1 = entire screen, 2 = graphics viewport)
	auto direction = readByte_t();	if (direction == -1) return;	// Direction
	auto movement = readByte_t();	if (movement == -1) return;	// Number of pixels to scroll

	// Extent matches viewport constant defs (plus 3=active)
	Rect * region = getViewport(extent);

	scrollRegion(region, direction, movement);
}


// VDU 23,16: Set cursor behaviour
// 
void VDUStreamProcessor::vdu_sys_cursorBehaviour() {
	auto setting = readByte_t();	if (setting == -1) return;
	auto mask = readByte_t();		if (mask == -1) return;

	setCursorBehaviour((uint8_t) setting, (uint8_t) mask);
}

// VDU 23, c, n1, n2, n3, n4, n5, n6, n7, n8: Redefine a display character
// Parameters:
// - c: The character to redefine
//
void VDUStreamProcessor::vdu_sys_udg(char c) {
	uint8_t		buffer[8];

	for (uint8_t i = 0; i < 8; i++) {
		auto b = readByte_t();
		if (b == -1) {
			return;
		}
		buffer[i] = b;
	}	

	redefineCharacter(c, buffer);
}

#endif // VDU_SYS_H
