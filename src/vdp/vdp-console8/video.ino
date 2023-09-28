//
// Title:			Agon Video BIOS
// Author:			Dean Belfield
// Contributors:	Jeroen Venema (Sprite Code, VGA Mode Switching)
//					Damien Guard (Fonts)
//					Igor Chaves Cananea (vdp-gl maintenance)
//					Steve Sims (Refactoring, bug fixes, Console8)
// Created:			22/03/2022
// Last Updated:	13/08/2023
//
// Modinfo:
// 11/07/2022:		Baud rate tweaked for Agon Light, HW Flow Control temporarily commented out
// 26/07/2022:		Added VDU 29 support
// 03/08/2022:		Set codepage 1252, fixed keyboard mappings for AGON, added cursorTab, VDP serial protocol
// 06/08/2022:		Added a custom font, fixed UART initialisation, flow control
// 10/08/2022:		Improved keyboard mappings, added sprites, audio, new font
// 05/09/2022:		Moved the audio class to agon_audio.h, added function prototypes in agon.h
// 02/10/2022:		Version 1.00: Tweaked the sprite code, changed bootup title to Quark
// 04/10/2022:		Version 1.01: Can now change keyboard layout, origin and sprites reset on mode change, available modes tweaked
// 23/10/2022:		Version 1.02: Bug fixes, cursor visibility and scrolling
// 15/02/2023:		Version 1.03: Improved mode, colour handling and international support
// 04/03/2023:					+ Added logical screen resolution, sendScreenPixel now sends palette index as well as RGB values
// 09/03/2023:					+ Keyboard now sends virtual key data, improved VDU 19 to handle COLOUR l,p as well as COLOUR l,r,g,b
// 15/03/2023:					+ Added terminal support for CP/M, RTC support for MOS
// 21/03/2023:				RC2 + Added keyboard repeat delay and rate, logical coords now selectable
// 22/03/2023:					+ VDP control codes now indexed from 0x80, added paged mode (VDU 14/VDU 15)
// 23/03/2023:					+ Added VDP_GP
// 26/03/2023:				RC3 + Potential fixes for FabGL being overwhelmed by faster comms
// 27/03/2023:					+ Fix for sprite system crash
// 29/03/2023:					+ Typo in boot screen fixed
// 01/04/2023:					+ Added resetPalette to MODE, timeouts to VDU commands
// 08/04/2023:				RC4 + Removed delay in readbyte_t, fixed VDP_SCRCHAR, VDP_SCRPIXEL
// 12/04/2023:					+ Fixed bug in play_note
// 13/04/2023:					+ Fixed bootup fail with no keyboard
// 17/04/2023:				RC5 + Moved wait_completion in vdu so that it only executes after graphical operations
// 18/04/2023:					+ Minor tweaks to wait completion logic
// 12/05/2023:		Version 1.04: Now uses vdp-gl instead of FabGL, implemented GCOL mode, sendModeInformation now sends video mode
// 19/05/2023:					+ Added VDU 4/5 support
// 25/05/2023:					+ Added VDU 24, VDU 26 and VDU 28, fixed inverted text colour settings
// 30/05/2023:					+ Added VDU 23,16 (cursor movement control)
// 28/06/2023:					+ Improved get_screen_char, fixed vdu_textViewport, cursorHome, changed modeline for Mode 2
// 30/06/2023:					+ Fixed vdu_sys_sprites to correctly discard serial input if bitmap allocation fails
// 13/08/2023:					+ New video modes, mode change resets page mode

#include <fabgl.h>

#define VERSION			1
#define REVISION		4
#define RC				1

#define	DEBUG			1						// Serial Debug Mode: 1 = enable
#define SERIALKB		0						// Serial Keyboard: 1 = enable (Experimental)

fabgl::Terminal				Terminal;			// Used for CP/M mode

#include "agon.h"								// Configuration file
#include "agon_keyboard.h"						// Keyboard support
#include "agon_screen.h"						// Screen support
#include "graphics.h"							// Graphics support
#include "cursor.h"								// Cursor support
#include "vdp_protocol.h"						// VDP Protocol
#include "vdu.h"								// VDU functions
#include "vdu_audio.h"							// Audio support

bool			terminalMode = false;			// Terminal mode

#if DEBUG == 1 || SERIALKB == 1
HardwareSerial DBGSerial(0);
#endif 

void setup() {
	disableCore0WDT(); delay(200);				// Disable the watchdog timers
	disableCore1WDT(); delay(200);
	#if DEBUG == 1 || SERIALKB == 1
	DBGSerial.begin(500000, SERIAL_8N1, 3, 1);
	#endif 
	setupVDPProtocol();
	wait_eZ80();
	setupKeyboard();
	init_audio();
	copy_font();
	set_mode(1);
	boot_screen();
}

// The main loop
//
void loop() {
	int count = 0;						// Generic counter, incremented every loop iteration
	bool cursorVisible = false;
	bool cursorState = false;

	while (true) {
 		if ((count & 0x7f) == 0) delay(1 /* -TM- ms */);
 		count++;

		if (terminalMode) {
			do_keyboard_terminal();
			continue;
		}
		cursorVisible = ((count & 0xFFFF) == 0);
		if (cursorVisible) {
			cursorState = !cursorState;
			do_cursor();
		}
		do_keyboard();
		if (byteAvailable()) {
			if (cursorState) {
 				cursorState = false;
				do_cursor();
			}
			count = -1;
			byte c = readByte();
			vdu(c);
		}
	}
}

// Handle the keyboard: BBC VDU Mode
//
void do_keyboard() {
	byte keycode;
	byte modifiers;
	byte vk;
	byte down;
	if (getKeyboardKey(&keycode, &modifiers, &vk, &down)) {
		// Handle some control keys
		//
		switch (keycode) {
			case 14: setPagedMode(true); break;
			case 15: setPagedMode(false); break;
		}
		// Create and send the packet back to MOS
		//
		byte packet[] = {
			keycode,
			modifiers,
			vk,
			down,
		};
		send_packet(PACKET_KEYCODE, sizeof packet, packet);
	}
}

// Handle the keyboard: CP/M Terminal Mode
// 
void do_keyboard_terminal() {
	byte ascii;
	if (getKeyboardKey(&ascii)) {
		// send raw byte straight to z80
		writeByte(ascii);
	}

	// Write anything read from z80 to the screen
	//
	while (byteAvailable()) {
		Terminal.write(readByte());
	}
}

// The boot screen
//
void boot_screen() {
	printFmt("Agon Quark VDP Version %d.%02d", VERSION, REVISION);
	#if RC > 0
		printFmt(" RC%d-bb4cb374", RC);
	#endif
	printFmt("\n\r");
}

// Debug printf to PC
//
void debug_log(const char *format, ...) {
	#if DEBUG == 1
	va_list ap;
	va_start(ap, format);
	int size = vsnprintf(nullptr, 0, format, ap) + 1;
	if (size > 0) {
		va_end(ap);
		va_start(ap, format);
		char buf[size + 1];
		vsnprintf(buf, size, format, ap);
		DBGSerial.print(buf);
	}
	va_end(ap);
	#endif
}

// Switch to terminal mode
//
void switchTerminalMode() {
	cls(true);
	delete canvas;
	Terminal.begin(getVGAController());	
	Terminal.connectSerialPort(VDPSerial);
	Terminal.enableCursor(true);
	terminalMode = true;
}

void print(char const * text) {
	for (int i = 0; i < strlen(text); i++) {
		vdu(text[i]);
	}
}

void printFmt(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	int size = vsnprintf(nullptr, 0, format, ap) + 1;
	if (size > 0) {
		va_end(ap);
		va_start(ap, format);
		char buf[size + 1];
		vsnprintf(buf, size, format, ap);
		print(buf);
	}
	va_end(ap);
}

