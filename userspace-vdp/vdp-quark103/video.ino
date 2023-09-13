//
// Title:	        Agon Video BIOS
// Author:        	Dean Belfield
// Contributors:	Jeroen Venema (Sprite Code, VGA Mode Switching)
//					Damien Guard (Fonts)
//					Igor Chaves Cananea (VGA Mode Switching)
// Created:       	22/03/2022
// Last Updated:	18/04/2023
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

#include "fabgl.h"
#include "HardwareSerial.h"
#include "ESP32Time.h"

#define VERSION			1
#define REVISION		3
#define RC				0

#define	DEBUG			0						// Serial Debug Mode: 1 = enable
#define SERIALKB		0						// Serial Keyboard: 1 = enable (Experimental)

fabgl::PS2Controller		PS2Controller;		// The keyboard class
fabgl::Canvas *				Canvas;				// The canvas class
fabgl::SoundGenerator		SoundGenerator;		// The audio class

fabgl::VGA2Controller		VGAController2;		// VGA class - 2 colours
fabgl::VGA4Controller		VGAController4;		// VGA class - 4 colours
fabgl::VGA8Controller		VGAController8;		// VGA class - 8 colours
fabgl::VGA16Controller		VGAController16;	// VGA class - 16 colours
fabgl::VGAController		VGAController64;	// VGA class - 64 colours

fabgl::VGABaseController *	VGAController;		// Pointer to the current VGA controller class (one of the above)

fabgl::Terminal				Terminal;			// Used for CP/M mode

#include "agon.h"								// Configuration file
#include "agon_fonts.h"							// The Acorn BBC Micro Font
#include "agon_audio.h"							// The Audio class
#include "agon_palette.h"						// Colour lookup table

int			VGAColourDepth;						// Number of colours per pixel (2, 4,8, 16 or 64)
int         charX, charY;						// Current character (X, Y) coordinates
Point		origin;								// Screen origin
Point       p1, p2, p3;							// Coordinate store for lot
RGB888      gfg;								// Graphics colour
RGB888      tfg, tbg;							// Text foreground and background colour
bool		cursorEnabled = true;				// Cursor visibility
bool		logicalCoords = true;				// Use BBC BASIC logical coordinates
double		logicalScaleX;						// Scaling factor for logical coordinates
double		logicalScaleY;
int         count = 0;							// Generic counter, incremented every iteration of loop
uint8_t		numsprites = 0;						// Number of sprites on stage
uint8_t 	current_sprite = 0; 				// Current sprite number
uint8_t 	current_bitmap = 0;					// Current bitmap number
Bitmap		bitmaps[MAX_BITMAPS];				// Bitmap object storage
Sprite		sprites[MAX_SPRITES];				// Sprite object storage
byte 		keycode = 0;						// Last pressed key code
byte 		modifiers = 0;						// Last pressed key modifiers
bool		terminalMode = false;				// Terminal mode
int			videoMode;							// Current video mode
bool 		pagedMode = false;					// Is output paged or not? Set by VDU 14 and 15
int			pagedModeCount = 0;					// Scroll counter for paged mode
int			kbRepeatDelay = 500;				// Keyboard repeat delay ms (250, 500, 750 or 1000)		
int			kbRepeatRate = 100;					// Keyboard repeat rate ms (between 33 and 500)
bool 		initialised = false;				// Is the system initialised yet?
bool		doWaitCompletion;					// For vdu function
uint8_t		palette[64];						// Storage for the palette

audio_channel *	audio_channels[AUDIO_CHANNELS];	// Storage for the channel data

ESP32Time	rtc(0);								// The RTC

#if DEBUG == 1 || SERIALKB == 1
HardwareSerial DBGSerial(0);
#endif 

void setup() {
	disableCore0WDT(); delay(200);								// Disable the watchdog timers
	disableCore1WDT(); delay(200);
	#if DEBUG == 1 || SERIALKB == 1
	DBGSerial.begin(500000, SERIAL_8N1, 3, 1);
	#endif 
	ESPSerial.end();
 	ESPSerial.setRxBufferSize(UART_RX_SIZE);					// Can't be called when running
 	ESPSerial.begin(UART_BR, SERIAL_8N1, UART_RX, UART_TX);
	#if USE_HWFLOW == 1
	ESPSerial.setHwFlowCtrlMode(HW_FLOWCTRL_RTS, 64);			// Can be called whenever
	ESPSerial.setPins(UART_NA, UART_NA, UART_CTS, UART_RTS);	// Must be called after begin
	#else 
	pinMode(UART_RTS, OUTPUT);
	pinMode(UART_CTS, INPUT);	
	setRTSStatus(true);
	#endif
	wait_eZ80();
 	PS2Controller.begin(PS2Preset::KeyboardPort0, KbdMode::CreateVirtualKeysQueue);
	PS2Controller.keyboard()->setLayout(&fabgl::UKLayout);
	PS2Controller.keyboard()->setCodePage(fabgl::CodePages::get(1252));
	PS2Controller.keyboard()->setTypematicRateAndDelay(kbRepeatRate, kbRepeatDelay);
	init_audio();
	copy_font();
  	set_mode(1);
	boot_screen();
}

// The main loop
//
void loop() {
	bool cursorVisible = false;
	bool cursorState = false;

	while(true) {
		count++;
		if ((count & 0x7f) == 0) delay(1 /* -TM- ms */);

		if(terminalMode) {
			do_keyboard_terminal();
			continue;
		}
    	cursorVisible = ((count & 0xFFFF) == 0);
    	if(cursorVisible) {
      		cursorState = !cursorState;
      		do_cursor();
    	}
    	do_keyboard();
    	if(ESPSerial.available() > 0) {
			#if USE_HWFLOW == 0
			if(ESPSerial.available() > UART_RX_THRESH) {
				setRTSStatus(false);		
			}
			#endif 
      		if(cursorState) {
 	    		cursorState = false;
        		do_cursor();
      		}
      		byte c = ESPSerial.read();
      		vdu(c);
    	}
		#if USE_HWFLOW == 0
		else {
			setRTSStatus(true);
		}
		#endif
  	}
}

// The boot screen
//
void boot_screen() {
  	printFmt("Agon Quark VDP Version %d.%02d", VERSION, REVISION);
	#if RC > 0
	  	printFmt(" RC%d", RC);
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

// Read an unsigned byte from the serial port, with a timeout
// Returns:
// - Byte value (0 to 255) if value read, otherwise -1
//
int readByte_t() {
	int	i;
	unsigned long t = millis();

	while(millis() - t < 1000) {
		if(ESPSerial.available() > 0) {
			return ESPSerial.read();
		}
	}
	return -1;
}

// Read an unsigned word from the serial port, with a timeout
// Returns:
// - Word value (0 to 65535) if 2 bytes read, otherwise -1
//
int	readWord_t() {
	int	l = readByte_t();
	if(l >= 0) {
		int h = readByte_t();
		if(h >= 0) {
			return (h << 8) | l;
		}
	}
	return -1;
}

// Read an unsigned byte from the serial port (blocking)
//
byte readByte_b() {
  	while(ESPSerial.available() == 0);
  	return ESPSerial.read();
}

// Read an unsigned word from the serial port (blocking)
//
uint32_t readLong_b() {
  uint32_t temp;
  temp  =  readByte_b();		// LSB;
  temp |= (readByte_b() << 8);
  temp |= (readByte_b() << 16);
  temp |= (readByte_b() << 24);
  return temp;
}

// Copy the AGON font data from Flash to RAM
//
void copy_font() {
	memcpy(fabgl::FONT_AGON_DATA + 256, fabgl::FONT_AGON_BITMAP, sizeof(fabgl::FONT_AGON_BITMAP));
}

// Set the RTS line value
//
void setRTSStatus(bool value) {
	digitalWrite(UART_RTS, value ? LOW : HIGH);		// Asserts when LOW
}

// Initialise the sound driver
//
void init_audio() {
	for(int i = 0; i < AUDIO_CHANNELS; i++) {
		init_audio_channel(i);
	}
}
void init_audio_channel(int channel) {
  	xTaskCreatePinnedToCore(audio_driver,  "audio_driver",
    	4096,					// This stack size can be checked & adjusted by reading the Stack Highwater
        &channel,				// Parameters
        PLAY_SOUND_PRIORITY,	// Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
        NULL,
    	ARDUINO_RUNNING_CORE
	);
}

// The music driver task
//
void audio_driver(void * parameters) {
	int channel = *(int *)parameters;

	audio_channels[channel] = new audio_channel(channel);
	task_loop {
		audio_channels[channel]->loop();
		vTaskDelay(1);
	}
}

// Wait for eZ80 to initialise
//
void wait_eZ80() {
	debug_log("wait_eZ80: Start\n\r");
	while(!initialised) {
    	if(ESPSerial.available() > 0) {
			#if USE_HWFLOW == 0
			if(ESPSerial.available() > UART_RX_THRESH) {
				setRTSStatus(false);		
			}
			#endif 		
			byte c = ESPSerial.read();	// Only handle VDU 23 packets
			if(c == 23) {
				vdu_sys();
			}
		}
	}
	debug_log("wait_eZ80: End\n\r");
}
	
// Send the cursor position back to MOS
//
void sendCursorPosition() {
	byte packet[] = {
		charX / Canvas->getFontInfo()->width,
		charY / Canvas->getFontInfo()->height,
	};
	send_packet(PACKET_CURSOR, sizeof packet, packet);	
}

// Send a character back to MOS
//
void sendScreenChar(int x, int y) {
	int	px = x * Canvas->getFontInfo()->width;
	int py = y * Canvas->getFontInfo()->height;
	char c = get_screen_char(px, py);
	byte packet[] = {
		c,
	};
	send_packet(PACKET_SCRCHAR, sizeof packet, packet);
}

// Send a pixel value back to MOS
//
void sendScreenPixel(int x, int y) {
	RGB888	pixel;
	byte 	pixelIndex = 0;
	Point	p = translate(scale(x, y));
	//
	// Do some bounds checking first
	//
	if(p.X >= 0 && p.Y >= 0 && p.X < Canvas->getWidth() && p.Y < Canvas->getHeight()) {
		pixel = Canvas->getPixel(p.X, p.Y);
		for(byte i = 0; i < VGAColourDepth; i++) {
			if(colourLookup[palette[i]] == pixel) {
				pixelIndex = i;
				break;
			}
		}
	}	
	byte packet[] = {
		pixel.R,	// Send the colour components
		pixel.G,
		pixel.B,
		pixelIndex,	// And the pixel index in the palette
	};
	send_packet(PACKET_SCRPIXEL, sizeof packet, packet);	
}

// Send an audio acknowledgement
//
void sendPlayNote(int channel, int success) {
	byte packet[] = {
		channel,
		success,
	};
	send_packet(PACKET_AUDIO, sizeof packet, packet);	
}

// Send MODE information (screen details)
//
void sendModeInformation() {
	int w = Canvas->getWidth();
	int h = Canvas->getHeight();
	byte packet[] = {
		w & 0xFF,	 						// Width in pixels (L)
		(w >> 8) & 0xFF,					// Width in pixels (H)
		h & 0xFF,							// Height in pixels (L)
		(h >> 8) & 0xFF,					// Height in pixels (H)
		w / Canvas->getFontInfo()->width ,	// Width in characters (byte)
		h / Canvas->getFontInfo()->height ,	// Height in characters (byte)
		VGAColourDepth,						// Colour depth
	};
	send_packet(PACKET_MODE, sizeof packet, packet);
}

// Send TIME information (from ESP32 RTC)
//
void sendTime() {
	byte packet[] = {
		rtc.getYear() - EPOCH_YEAR,			// 0 - 255
		rtc.getMonth(),						// 0 - 11
		rtc.getDay(),						// 1 - 31
		rtc.getDayofYear(),					// 0 - 365
		rtc.getDayofWeek(),					// 0 - 6
		rtc.getHour(true),					// 0 - 23
		rtc.getMinute(),					// 0 - 59
		rtc.getSecond(),					// 0 - 59
	};
	send_packet(PACKET_RTC, sizeof packet, packet);
}

// Send the keyboard state
//
void sendKeyboardState() {
	bool numLock;
	bool capsLock;
	bool scrollLock;
	PS2Controller.keyboard()->getLEDs(&numLock, &capsLock, &scrollLock);
	byte packet[] = {
		kbRepeatDelay & 0xFF,
		(kbRepeatDelay >> 8) & 0xFF,
		kbRepeatRate & 0xFF,
		(kbRepeatRate >> 8) & 0xFF,
		scrollLock | (capsLock << 1) | (numLock << 2)
	};
	send_packet(PACKET_KEYSTATE, sizeof packet, packet);
}

// Send a general poll
//
void sendGeneralPoll() {
	byte b = readByte_b();
	byte packet[] = {
		b,
	};
	send_packet(PACKET_GP, sizeof packet, packet);
	initialised = true;	
}

// Clear the screen
// 
void cls() {
	int i;

	if(Canvas) {
		Canvas->setPenColor(tfg);
 		Canvas->setBrushColor(tbg);	
		Canvas->clear();
	}
	if(numsprites) {
		if(VGAController) {
			VGAController->removeSprites();
			if(Canvas) {
				Canvas->clear();
			}
		}
		numsprites = 0;
	}
	charX = 0;
	charY = 0;
	pagedModeCount = 0;
}

// Clear the graphics area
//
void clg() {
	if(Canvas) {
		Canvas->clear();
	}
}

// Try and match a character
//
char get_screen_char(int px, int py) {
	RGB888	pixel;
	uint8_t	charRow;
	uint8_t	charData[8];

	// Do some bounds checking first
	//
	if(px < 0 || py < 0 || px >= Canvas->getWidth() - 8 || py >= Canvas->getHeight() - 8) {
		return 0;
	}

	// Now scan the screen and get the 8 byte pixel representation in charData
	//
	for(int y = 0; y < 8; y++) {
		charRow = 0;
		for(int x = 0; x < 8; x++) {
			pixel = Canvas->getPixel(px + x, py + y);
			if(pixel == tfg) {
				charRow |= (0x80 >> x);
			}
		}
		charData[y] = charRow;
	}
	//
	// Finally try and match with the character set array
	//
	for(int i = 32; i < 128; i++) {
		if(cmp_char(charData, &fabgl::FONT_AGON_DATA[i * 8], 8)) {	
			return i;		
		}
	}
	return 0;
}

bool cmp_char(uint8_t * c1, uint8_t *c2, int len) {
	for(int i = 0; i < len; i++) {
		if(*c1++ != *c2++) {
			return false;
		}
	}
	return true;
}

// Switch to terminal mode
//
void switchTerminalMode() {
	cls();
  	delete Canvas;
	Terminal.begin(VGAController);	
	Terminal.connectSerialPort(ESPSerial);
	Terminal.enableCursor(true);
	terminalMode = true;
}

// Get controller
// Parameters:
// - colours: Number of colours per pixel (2, 4, 8, 16 or 64)
// Returns:
// - A singleton instance of a VGAController class
//
fabgl::VGABaseController * get_VGAController(int colours) {
	switch(colours) {
		case  2: return VGAController2.instance();
		case  4: return VGAController4.instance();
		case  8: return VGAController8.instance();
		case 16: return VGAController16.instance();
		case 64: return VGAController64.instance();
	}
	return nullptr;
}

// Set a palette item
// Parameters:
// - l: The logical colour to change
// - c: The new colour
// 
void setPaletteItem(int l, RGB888 c) {
	if(l < VGAColourDepth) {
		switch(VGAColourDepth) {
			case 2: VGAController2.setPaletteItem(l, c); break;
			case 4: VGAController4.setPaletteItem(l, c); break;
			case 8: VGAController8.setPaletteItem(l, c); break;
			case 16: VGAController16.setPaletteItem(l, c); break;
		}
	}
}

// Reset the palette and set the foreground and background drawing colours
// Parameters:
// - colou: Array of indexes into colourLookup table
// - sizeOfArray: Size of passed colours array
//
void resetPalette(const uint8_t colours[]) {
	for(int i = 0; i < 64; i++) {
		uint8_t c = colours[i%VGAColourDepth];
		palette[i] = c;
		setPaletteItem(i, colourLookup[c]);
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
int change_resolution(int colours, char * modeLine) {
	fabgl::VGABaseController * controller = get_VGAController(colours);

	if(controller == nullptr) {						// If controller is null, then an invalid # of colours was passed
		return 1;									// So return the error
	}
  	delete Canvas;									// Delete the canvas

	VGAColourDepth = colours;						// Set the number of colours per pixel
	if(VGAController != controller) {				// Is it a different controller?
		if(VGAController) {							// If there is an existing controller running then
			VGAController->end();					// end it
		}
		VGAController = controller;					// Switch to the new controller
		VGAController->begin();						// And spin it up
	}
	if(modeLine) {									// If modeLine is not a null pointer then
		VGAController->setResolution(modeLine);		// Set the resolution
	}
	VGAController->enableBackgroundPrimitiveExecution(true);
	VGAController->enableBackgroundPrimitiveTimeout(false);

  	Canvas = new fabgl::Canvas(VGAController);		// Create the new canvas
	//
	// Check whether the selected mode has enough memory for the vertical resolution
	//
	if(VGAController->getScreenHeight() != VGAController->getViewPortHeight()) {
		return 2;
	}
	return 0;										// Return with no errors
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
int change_mode(int mode) {
	int errVal = -1;

	cls();
	if(mode != videoMode) {
		switch(mode) {
			case 0:
				errVal = change_resolution(2, SVGA_1024x768_60Hz);
				break;
			case 1:
				errVal = change_resolution(16, VGA_512x384_60Hz);
				break;
			case 2:
				errVal = change_resolution(64, VGA_320x200_75Hz);
				break;
			case 3:
				errVal = change_resolution(16, VGA_640x480_60Hz);
				break;
		}
		if(errVal != 0) {
			return errVal;
		}
	}
	switch(VGAColourDepth) {
		case  2: resetPalette(defaultPalette02); break;
		case  4: resetPalette(defaultPalette04); break;
		case  8: resetPalette(defaultPalette08); break;
		case 16: resetPalette(defaultPalette10); break;
		case 64: resetPalette(defaultPalette40); break;
	}
 	gfg = colourLookup[0x3F];
	tfg = colourLookup[0x3F];
	tbg = colourLookup[0x00];
  	Canvas->selectFont(&fabgl::FONT_AGON);
  	Canvas->setGlyphOptions(GlyphOptions().FillBackground(true));
  	Canvas->setPenWidth(1);
	origin = Point(0,0);
	p1 = Point(0,0);
	p2 = Point(0,0);
	p3 = Point(0,0);
	logicalScaleX = LOGICAL_SCRW / (double)Canvas->getWidth();
	logicalScaleY = LOGICAL_SCRH / (double)Canvas->getHeight();
	cursorEnabled = true;
	sendModeInformation();
	debug_log("do_modeChange: canvas(%d,%d), scale(%f,%f)\n\r", Canvas->getWidth(), Canvas->getHeight(), logicalScaleX, logicalScaleY);
	return 0;
}

// Change the video mode
// If there is an error, restore the last mode
// Parameters:
// - mode: The video mode
// 
void set_mode(int mode) {
	int errVal = change_mode(mode);
	if(errVal != 0) {
		debug_log("set_mode: error %d\n\r", errVal);
		change_mode(videoMode);
		return;
	}
	videoMode = mode;
}

void print(char const * text) {
	for(int i = 0; i < strlen(text); i++) {
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

// Handle the keyboard: CP/M Terminal Mode
// 
void do_keyboard_terminal() {
  	fabgl::Keyboard* kb = PS2Controller.keyboard();
	fabgl::VirtualKeyItem item;

	// Read the keyboard and transmit to the Z80
	//
	if(kb->getNextVirtualKey(&item, 0)) {
		if(item.down) {
			ESPSerial.write(item.ASCII);
		}
	}

	// Wait for the response and write to the screen
	//
	while(ESPSerial.available()) {
		Terminal.write(ESPSerial.read());
	}
}

// Wait for shift key to be released, then pressed (for paged mode)
// 
void wait_shiftkey() {
  	fabgl::Keyboard* kb = PS2Controller.keyboard();
	fabgl::VirtualKeyItem item;

	// Wait for shift to be released
	//
	do {
		kb->getNextVirtualKey(&item, 0);
	} while(item.SHIFT);

	// And pressed again
	//
	do {
		kb->getNextVirtualKey(&item, 0);
		if(item.ASCII == 27) {	// Check for ESC
			byte packet[] = {
				item.ASCII,
				0,
				item.vk,
				item.down,
			};
			send_packet(PACKET_KEYCODE, sizeof packet, packet);
			return;
		}
	} while(!item.SHIFT);
}

// Handle the keyboard: BBC VDU Mode
//
void do_keyboard() {
  	fabgl::Keyboard* kb = PS2Controller.keyboard();
	fabgl::VirtualKeyItem item;

	#if SERIALKB == 1
	if(DBGSerial.available()) {
		byte packet[] = {
			DBGSerial.read(),
			0,
		};
		send_packet(PACKET_KEYCODE, sizeof packet, packet);
		return;
	}
	#endif
	
	if(kb->getNextVirtualKey(&item, 0)) {
		if(item.down) {
			switch(item.vk) {
				case fabgl::VK_LEFT:
					keycode = 0x08;
					break;
				case fabgl::VK_TAB:
					keycode = 0x09;
					break;
				case fabgl::VK_RIGHT:
					keycode = 0x15;
					break;
				case fabgl::VK_DOWN:
					keycode = 0x0A;
					break;
				case fabgl::VK_UP:
					keycode = 0x0B;
					break;
				case fabgl::VK_BACKSPACE:
					keycode = 0x7F;
					break;
				default:
					keycode = item.ASCII;	
					break;
			}
			// Pack the modifiers into a byte
			//
			modifiers = 
				item.CTRL		<< 0 |
				item.SHIFT		<< 1 |
				item.LALT		<< 2 |
				item.RALT		<< 3 |
				item.CAPSLOCK	<< 4 |
				item.NUMLOCK	<< 5 |
				item.SCROLLLOCK << 6 |
				item.GUI		<< 7
			;
		}
		// Handle some control keys
		//
		switch(keycode) {
			case 14: pagedMode = true; break;
			case 15: pagedMode = false; break;
		}
		// Create and send the packet back to MOS
		//
		byte packet[] = {
			keycode,
			modifiers,
			item.vk,
			item.down,
		};
		send_packet(PACKET_KEYCODE, sizeof packet, packet);
	}
}

// Send a packet of data to the MOS
//
void send_packet(byte code, byte len, byte data[]) {
	ESPSerial.write(code + 0x80);
	ESPSerial.write(len);
	for(int i = 0; i < len; i++) {
		ESPSerial.write(data[i]);
	}
}

// Render a cursor at the current screen position
//
void do_cursor() {
	if(cursorEnabled) {
  		int w = Canvas->getFontInfo()->width;
  		int h = Canvas->getFontInfo()->height;
	  	int x = charX; 
	  	int y = charY;
	  	Canvas->swapRectangle(x, y, x + w - 1, y + h - 1);
	}
}

// Scale and Translate a point
//
Point scale(Point p) {
	return scale(p.X, p.Y);
}
Point scale(int X, int Y) {
	if(logicalCoords) {
		return Point((double)X / logicalScaleX, (double)Y / logicalScaleY);
	}
	return Point(X, Y);
}

Point translate(Point p) {
	return translate(p.X, p.Y);
}
Point translate(int X, int Y) {
	if(logicalCoords) {
		return Point(origin.X + X, (Canvas->getHeight() - 1) - (origin.Y + Y));
	}
	return Point(origin.X + X, origin.Y + Y);
}

void vdu(byte c) {
	if(c >= 0x20 && c != 0x7F) {
		Canvas->setPenColor(tfg);
		Canvas->setBrushColor(tbg);
 		Canvas->drawChar(charX, charY, c);
   		cursorRight();
	}
	else {
		doWaitCompletion = false;
		switch(c) {
			case 0x08:  // Cursor Left
				cursorLeft();
				break;
			case 0x09:  // Cursor Right
				cursorRight();
				break;
			case 0x0A:  // Cursor Down
				cursorDown();
				break;
			case 0x0B:  // Cursor Up
				cursorUp();
				break;
			case 0x0C:  // CLS
				cls();
				break;
			case 0x0D:  // CR
				cursorHome();
				break;
			case 0x0E:	// Paged mode ON
				pagedMode = true;
				break;
			case 0x0F:	// Paged mode OFF
				pagedMode = false;
				break;
			case 0x10:	// CLG
				clg();
				break;
			case 0x11:	// COLOUR
				vdu_colour();
				break;
			case 0x12:  // GCOL
				vdu_gcol();
				break;
			case 0x13:	// Define Logical Colour
				vdu_palette();
				break;
			case 0x16:  // Mode
				vdu_mode();
				break;
			case 0x17:  // VDU 23
				vdu_sys();
				break;
			case 0x19:  // PLOT
				vdu_plot();
				break;
			case 0x1D:	// VDU_29
				vdu_origin();
			case 0x1E:  // Home
				cursorHome();
				break;
			case 0x1F:	// TAB(X,Y)
				cursorTab();
				break;
			case 0x7F:  // Backspace
				cursorLeft();
				Canvas->drawChar(charX, charY, ' ');
				break;
		}
		if(doWaitCompletion) {
			Canvas->waitCompletion(false);
		}
	}
}

// Handle the cursor
//
void cursorLeft() {
	charX -= Canvas->getFontInfo()->width;
  	if(charX < 0) {
    	charX = 0;
  	}
}

void cursorRight() {
  	charX += Canvas->getFontInfo()->width;
  	if(charX >= Canvas->getWidth()) {
    	cursorHome();
    	cursorDown();
  	}
}

void cursorDown() {
	int fh = Canvas->getFontInfo()->height;
	int ch = Canvas->getHeight();
	int pl = ch / fh;

	charY += fh;
	if(pagedMode) {
		pagedModeCount++;
		if(pagedModeCount >= pl) {
			pagedModeCount = 0;
			wait_shiftkey();
		}
	}
	if(charY >= ch) {
		charY -= fh;
		Canvas->scroll(0, -fh);
	}
}

void cursorUp() {
  	charY -= Canvas->getFontInfo()->height;
  	if(charY < 0) {
		charY = 0;
  	}
}

void cursorHome() {
  	charX = 0;
}

void cursorTab() {
	int x = readByte_t();
	if(x >= 0) {
		int y = readByte_t();
		if(y >= 0) {
			charX = x * Canvas->getFontInfo()->width;
			charY = y * Canvas->getFontInfo()->height;
		}
	}
}

// Handle MODE
//
void vdu_mode() {
  	int mode = readByte_t();
	debug_log("vdu_mode: %d\n\r", mode);
	if(mode >= 0) {
	  	set_mode(mode);
	}
}

// Handle VDU 29
//
void vdu_origin() {
	int x = readWord_t();
	if(x >= 0) {
		int y = readWord_t();
		if(y >= 0) {
			origin = scale(x, y);
			debug_log("vdu_origin: %d,%d\n\r", origin.X, origin.Y);
		}
	}
}

// Handle COLOUR
// 
void vdu_colour() {
	int		colour = readByte_t();
	byte	c = palette[colour%VGAColourDepth];

	if(colour >= 0 && colour < 64) {
		tfg = colourLookup[c];
		debug_log("vdu_colour: tfg %d = %02X : %02X,%02X,%02X\n\r", colour, c, tfg.R, tfg.G, tfg.B);
	}
	else if(colour >= 128 && colour < 192) {
		tbg = colourLookup[c];
		debug_log("vdu_colour: tbg %d = %02X : %02X,%02X,%02X\n\r", colour, c, tbg.R, tbg.G, tbg.B);	
	}
	else {
		debug_log("vdu_colour: invalid colour %d\n\r");
	}
}

// Handle GCOL
// 
void vdu_gcol() {
	int	mode = readByte_t();
	if(mode >= 0) {
		int	colour = readByte_t();
		if(colour >= 0) {
			gfg = colourLookup[palette[colour%VGAColourDepth]];
			debug_log("vdu_gcol: gfg %d = %d,%d,%d\n\r", colour, gfg.R, gfg.G, gfg.B);
		}
	}
}

// Handle palette
//
void vdu_palette() {
	int l = readByte_t(); if(l == -1) return; // Logical colour
	int p = readByte_t(); if(p == -1) return; // Physical colour
	int r = readByte_t(); if(r == -1) return; // The red component
	int g = readByte_t(); if(g == -1) return; // The green component
	int b = readByte_t(); if(b == -1) return; // The blue component

	RGB888 col;				// The colour to set

	if(VGAColourDepth < 64) {			// If it is a paletted video mode
		if(p == 255) {					// If p = 255, then use the RGB values
			col = RGB888(r, g, b);
		}
		else if(p < 64) {				// If p < 64, then look the value up in the colour lookup table
			col = colourLookup[p];
		}
		else {				
			debug_log("vdu_palette: p=%d not supported\n\r", p);
			return;
		}
		setPaletteItem(l, col);
		doWaitCompletion = true;
		debug_log("vdu_palette: %d,%d,%d,%d,%d\n\r", l, p, r, g, b);
	}
	else {
		debug_log("vdu_palette: not supported in this mode\n\r");
	}
}

// Handle PLOT
//
void vdu_plot() {
  	int mode = readByte_t(); if(mode == -1) return;

	int x = readWord_t(); if(x == -1) return; else x = (short)x;
	int y = readWord_t(); if(y == -1) return; else y = (short)y;

  	p3 = p2;
  	p2 = p1;
	p1 = translate(scale(x, y));
	Canvas->setPenColor(gfg);
	debug_log("vdu_plot: mode %d, (%d,%d) -> (%d,%d)\n\r", mode, x, y, p1.X, p1.Y);
  	switch(mode) {
    	case 0x04: 			// Move 
      		Canvas->moveTo(p1.X, p1.Y);
      		break;
    	case 0x05: 			// Line
      		Canvas->lineTo(p1.X, p1.Y);
      		break;
		case 0x40 ... 0x47:	// Point
			Canvas->setPixel(p1.X, p1.Y);
			break;
    	case 0x50 ... 0x57: // Triangle
      		vdu_plot_triangle(mode);
      		break;
    	case 0x90 ... 0x97: // Circle
      		vdu_plot_circle(mode);
      		break;
  	}
	doWaitCompletion = true;
}

void vdu_plot_triangle(byte mode) {
  	Point p[3] = {
		p3,
		p2,
		p1, 
	};
  	Canvas->setBrushColor(gfg);
  	Canvas->fillPath(p, 3);
  	Canvas->setBrushColor(tbg);
}

void vdu_plot_circle(byte mode) {
  	int a, b, r;
  	switch(mode) {
    	case 0x90 ... 0x93: // Circle
      		r = 2 * (p1.X + p1.Y);
      		Canvas->drawEllipse(p2.X, p2.Y, r, r);
      		break;
    	case 0x94 ... 0x97: // Circle
      		a = p2.X - p1.X;
      		b = p2.Y - p1.Y;
      		r = 2 * sqrt(a * a + b * b);
      		Canvas->drawEllipse(p2.X, p2.Y, r, r);
      		break;
  	}
}

// Handle SYS
// VDU 23,mode
//
void vdu_sys() {
  	int mode = readByte_t();

	//
	// If mode is -1, then there's been a timeout
	//
	if(mode == -1) {
		return;
	}
	//
	// If mode < 32, then it's a system command
	//
	else if(mode < 32) {
  		switch(mode) {
		    case 0x00: {					// VDU 23, 0
      			vdu_sys_video();			// Video system control
			}	break;
			case 0x01: {					// VDU 23, 1
				int b = readByte_t();		// Cursor control
				if(b >= 0) {
					cursorEnabled = b;	
				}
			}	break;
			case 0x07: {					// VDU 23, 7
				vdu_sys_scroll();			// Scroll 
			}	break;
			case 0x1B: {					// VDU 23, 27
				vdu_sys_sprites();			// Sprite system control
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

// Define a UDG
// Parameters:
// - c: The character to redefine
//
void vdu_sys_udg(byte c) {
	uint8_t		buffer[8];
	int			b;

	for(int i = 0; i < 8; i++) {
		b = readByte_t();
		if(b == -1) {
			return;
		}
		buffer[i] = b;
	}	
	memcpy(&fabgl::FONT_AGON_DATA[c * 8], buffer, 8);
}

// VDU 23,0: VDP control
// These can send responses back; the response contains a packet # that matches the VDU command mode byte
//
void vdu_sys_video() {
  	int	mode = readByte_t();

  	switch(mode) {
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
			int x = readWord_t();		// Get character at screen position x, y
			int y = readWord_t();
			sendScreenChar(x, y);
		}	break;
		case VDP_SCRPIXEL: {			// VDU 23, 0, &84, x; y;
			int x = readWord_t();		// Get pixel value at screen position x, y
			int y = readWord_t();
			sendScreenPixel((short)x, (short)y);
		} 	break;		
		case VDP_AUDIO: {				// VDU 23, 0, &85, channel, waveform, volume, freq; duration;
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
		case VDP_LOGICALCOORDS: {		// VDU 23, 0, &C0, n
			int b = readByte_t();		// Set logical coord mode
			if(b >= 0) {
				logicalCoords = b;	
			}
		}	break;
		case VDP_TERMINALMODE: {		// VDU 23, 0, &FF
			switchTerminalMode(); 		// Switch to terminal mode
		}	break;
  	}
}

// Do some audio
//
void vdu_sys_audio() {
	int channel = readByte_t();		if(channel == -1) return;
	int waveform = readByte_t();	if(waveform == -1) return;
	int volume = readByte_t();		if(volume == -1) return;
	int frequency = readWord_t();	if(frequency == -1) return;
	int duration = readWord_t();	if(duration == -1) return;

	sendPlayNote(channel, play_note(channel, volume, frequency, duration));
}

// Handle the keystate
//
void vdu_sys_keystate() {
	int d = readWord_t(); if(d == -1) return;
	int r = readWord_t(); if(r == -1) return;
	int b = readByte_t(); if(b == -1) return;

	if(d >= 250 && d <= 1000) kbRepeatDelay = (d / 250) * 250;	// In steps of 250ms
	if(r >=  33 && r <=  500) kbRepeatRate  = r;

	if(b != 255) {
		PS2Controller.keyboard()->setLEDs(b & 4, b & 2, b & 1);
	}
	PS2Controller.keyboard()->setTypematicRateAndDelay(kbRepeatRate, kbRepeatDelay);
	debug_log("vdu_sys_video: keystate: d=%d, r=%d, led=%d\n\r", kbRepeatDelay, kbRepeatRate, b);
	sendKeyboardState();
}

// Set the keyboard layout
//
void vdu_sys_video_kblayout() {
	int	region = readByte_t();			// Fetch the region
	switch(region) {
		case 1:	PS2Controller.keyboard()->setLayout(&fabgl::USLayout); break;
		case 2:	PS2Controller.keyboard()->setLayout(&fabgl::GermanLayout); break;
		case 3:	PS2Controller.keyboard()->setLayout(&fabgl::ItalianLayout); break;
		case 4:	PS2Controller.keyboard()->setLayout(&fabgl::SpanishLayout); break;
		case 5: PS2Controller.keyboard()->setLayout(&fabgl::FrenchLayout); break;
		case 6:	PS2Controller.keyboard()->setLayout(&fabgl::BelgianLayout); break;
		case 7:	PS2Controller.keyboard()->setLayout(&fabgl::NorwegianLayout); break;
		case 8:	PS2Controller.keyboard()->setLayout(&fabgl::JapaneseLayout);break;
		default:
			PS2Controller.keyboard()->setLayout(&fabgl::UKLayout);
			break;
	}
}

// Handle time requests
//
void vdu_sys_video_time() {
	int mode = readByte_t();

	if(mode == 0) {
		sendTime();
	}
	else if(mode == 1) {
		int yr = readByte_t(); if(yr == -1) return;
		int mo = readByte_t(); if(mo == -1) return;
		int da = readByte_t(); if(da == -1) return;
		int ho = readByte_t(); if(ho == -1) return;	
		int mi = readByte_t(); if(mi == -1) return;
		int se = readByte_t(); if(se == -1) return;

		yr = EPOCH_YEAR + (int8_t)yr;

		if(yr >= 1970) {
			rtc.setTime(se, mi, ho, da, mo, yr);
		}
	}
}

// VDU 23,7: Scroll rectangle on screen
//
void vdu_sys_scroll() {
	int extent = readByte_t(); 		if(extent == -1) return;	// Extent (0 = current text window, 1 = entire screen)
	int direction = readByte_t();	if(direction == -1) return;	// Direction
	int movement = readByte_t();	if(movement == -1) return;	// Number of pixels to scroll

	switch(direction) {
		case 0:	// Right
			Canvas->scroll(movement, 0);
			break;
		case 1: // Left
			Canvas->scroll(-movement, 0);
			break;
		case 2: // Down
			Canvas->scroll(0, movement);
			break;
		case 3: // Up
			Canvas->scroll(0, -movement);
			break;
	}
	doWaitCompletion = true;
}

// Play a note
//
word play_note(byte channel, byte volume, word frequency, word duration) {
	if(channel >=0 && channel < AUDIO_CHANNELS) {
		return audio_channels[channel]->play_note(volume, frequency, duration);
	}
	return 1;
}

// Sprite Engine
//
void vdu_sys_sprites(void) {
    uint32_t	color;
    void *		dataptr;
    int16_t 	x, y;
    int16_t 	width, height;
    uint16_t 	n, temp;
    
    int cmd = readByte_t();

    switch(cmd) {
    	case 0: {	// Select bitmap
			int	rb = readByte_t();
			if(rb >= 0) {
        		current_bitmap = rb;
        		debug_log("vdu_sys_sprites: bitmap %d selected\n\r", current_bitmap);
			}
		}	break;
      	
		case 1: 	// Send bitmap data
      	case 2: {	// Define bitmap in single color
			int rw = readWord_t(); if(rw == -1) return;
			int rh = readWord_t(); if(rh == -1) return;

			width = rw;
			height = rh;
			//
        	// Clear out any old data first
			//
        	free(bitmaps[current_bitmap].data);
			//
        	// Allocate new heap data
			//
        	dataptr = (void *)heap_caps_malloc(sizeof(uint32_t)*width*height, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
        	bitmaps[current_bitmap].data = (uint8_t *)dataptr;

        	if(dataptr != NULL) {                  
				if(cmd == 1) {
					//
					// Read data to the new databuffer
					//
					for(n = 0; n < width*height; n++) ((uint32_t *)bitmaps[current_bitmap].data)[n] = readLong_b();  
					debug_log("vdu_sys_sprites: bitmap %d - data received - width %d, height %d\n\r", current_bitmap, width, height);
				}
				if(cmd == 2) {
					color = readLong_b();
					//
					// Define single color
					//
					for(n = 0; n < width*height; n++) ((uint32_t *)dataptr)[n] = color;            
					debug_log("vdu_sys_sprites: bitmap %d - set to solid color - width %d, height %d\n\r", current_bitmap, width, height);            
				}
				// Create bitmap structure
				//
				bitmaps[current_bitmap] = Bitmap(width,height,dataptr,PixelFormat::RGBA8888);
				bitmaps[current_bitmap].dataAllocated = false;
			}
	        else {
    	    	for(n = 0; n < width*height; n++) readLong_b(); // discard incoming data
        		debug_log("vdu_sys_sprites: bitmap %d - data discarded, no memory available - width %d, height %d\n\r", current_bitmap, width, height);
        	}
		}	break;
      	
		case 3: {	// Draw bitmap to screen (x,y)
			int	rx = readWord_t(); if(rx == -1) return; x = rx;
			int ry = readWord_t(); if(ry == -1) return; y = ry;

			if(bitmaps[current_bitmap].data) {
				Canvas->drawBitmap(x,y,&bitmaps[current_bitmap]);
				doWaitCompletion = true;
			}
			debug_log("vdu_sys_sprites: bitmap %d draw command\n\r", current_bitmap);
		}	break;

	   /*
		* Sprites
		* 
		* Sprite creation order:
		* 1) Create bitmap(s) for sprite, or re-use bitmaps already created
		* 2) Select the correct sprite ID (0-255). The GDU only accepts sequential sprite sets, starting from ID 0. All sprites must be adjacent to 0
		* 3) Clear out any frames from previous program definitions
		* 4) Add one or more frames to each sprite
		* 5) Activate sprite to the GDU
		* 6) Show sprites on screen / move them around as needed
		* 7) Refresh
		*/
    	case 4: {	// Select sprite
			int b = readByte_t(); if(b == -1) return;
			current_sprite = b;
			debug_log("vdu_sys_sprites: sprite %d selected\n\r", current_sprite);
		}	break;

      	case 5: {	// Clear frames
			sprites[current_sprite].visible = false;
			sprites[current_sprite].setFrame(0);
			sprites[current_sprite].clearBitmaps();
			debug_log("vdu_sys_sprites: sprite %d - all frames cleared\n\r", current_sprite);
		}	break;
        
      	case 6:	{	// Add frame to sprite
			int b = readByte_t(); if(b == -1) return; n = b;
			sprites[current_sprite].addBitmap(&bitmaps[n]);
			debug_log("vdu_sys_sprites: sprite %d - bitmap %d added as frame %d\n\r", current_sprite, n, sprites[current_sprite].framesCount-1);
		}	break;

      	case 7:	{	// Active sprites to GDU
			int b = readByte_t(); if(b == -1) return;
			/*
			* Sprites 0-(numsprites-1) will be activated on-screen
			* Make sure all sprites have at least one frame attached to them
			*/
			numsprites = b;
			if(numsprites) {
				VGAController->setSprites(sprites, numsprites);
			}
			else {
				VGAController->removeSprites();
			}
			doWaitCompletion = true;
			debug_log("vdu_sys_sprites: %d sprites activated\n\r", numsprites);
		}	break;

      	case 8: {	// Set next frame on sprite
			sprites[current_sprite].nextFrame();
			debug_log("vdu_sys_sprites: sprite %d next frame\n\r", current_sprite);
		}	break;

      	case 9:	{	// Set previous frame on sprite
			n = sprites[current_sprite].currentFrame;
			if(n) sprites[current_sprite].currentFrame = n-1; // previous frame
			else sprites[current_sprite].currentFrame = sprites[current_sprite].framesCount - 1;  // last frame
			debug_log("vdu_sys_sprites: sprite %d previous frame\n\r", current_sprite);
		}	break;

      	case 10: {	// Set current frame id on sprite
			int b = readByte_t(); if(b == -1) return;
			n = b;
			if(n < sprites[current_sprite].framesCount) sprites[current_sprite].currentFrame = n;
			debug_log("vdu_sys_sprites: sprite %d set to frame %d\n\r", current_sprite,n);
		}	break;

      	case 11: {	// Show sprite
        	sprites[current_sprite].visible = 1;
			debug_log("vdu_sys_sprites: sprite %d show cmd\n\r", current_sprite);
		}	break;

      	case 12: {	// Hide sprite
			sprites[current_sprite].visible = 0;
			debug_log("vdu_sys_sprites: sprite %d hide cmd\n\r", current_sprite);
		}	break;

      	case 13: {	// Move sprite to coordinate on screen
			int	rx = readWord_t(); if(rx == -1) return; x = rx;
			int ry = readWord_t(); if(ry == -1) return; y = ry;

			sprites[current_sprite].moveTo(x, y); 
			debug_log("vdu_sys_sprites: sprite %d - move to (%d,%d)\n\r", current_sprite, x, y);
		}	break;

      	case 14: {	// Move sprite by offset to current coordinate on screen
			int	rx = readWord_t(); if(rx == -1) return; x = rx;
			int ry = readWord_t(); if(ry == -1) return; y = ry;
			sprites[current_sprite].x += x;
			sprites[current_sprite].y += y;
			debug_log("vdu_sys_sprites: sprite %d - move by offset (%d,%d)\n\r", current_sprite, x, y);
		}	break;

	  	case 15: {	// Refresh
			if(numsprites) { 
				VGAController->refreshSprites();
			}
			debug_log("vdu_sys_sprites: perform sprite refresh\n\r");
		}	break;
		case 16: {	// Reset
			cls();
			for(n = 0; n < MAX_SPRITES; n++) {
				sprites[n].visible = false;
				sprites[current_sprite].setFrame(0);
				sprites[n].clearBitmaps();
			}
			for(n = 0; n < MAX_BITMAPS; n++) {
	        	free(bitmaps[n].data);
				bitmaps[n].dataAllocated = false;
			}
			doWaitCompletion = true;
			debug_log("vdu_sys_sprites: reset\n\r");
		}	break;
    }
}
