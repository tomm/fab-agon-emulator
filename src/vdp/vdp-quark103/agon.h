//
// Title:	        Agon Video BIOS - Function prototypes
// Author:        	Dean Belfield
// Created:       	05/09/2022
// Last Updated:    23/03/2023
//
// Modinfo:
// 04/03/2023:      Added LOGICAL_SCRW and LOGICAL_SCRH
// 17/03/2023:		Added PACKET_RTC, EPOCH_YEAR, MAX_SPRITES, MAX_BITMAPS
// 21/03/2023:		Added PACKET_KEYSTATE
// 22/03/2023:		Added VDP codes
// 23/03/2023:		Increased baud rate to 1152000

#pragma once

#define	ESPSerial Serial2

#define USE_HWFLOW				1		// Flow Control: 1 = enable hardware flow control to eZ80

#define EPOCH_YEAR				1980	// 1-byte dates are offset from this (for FatFS)
#define MAX_SPRITES				256		// Maximum number of sprites
#define MAX_BITMAPS				256		// Maximum number of bitmaps

#define UART_BR					1152000	// Max baud rate; previous stable value was 384000
#define UART_NA					-1
#define UART_TX					2
#define UART_RX					34
#define UART_RTS  	 		 	13 		// The ESP32 RTS pin (eZ80 CTS)
#define UART_CTS  	  			14		// The ESP32 CTS pin (eZ80 RTS)

#define UART_RX_SIZE			256		// The RX buffer size
#define UART_RX_THRESH			128		// Point at which RTS is toggled

#define GPIO_ITRP				17		// VSync Interrupt Pin - for reference only

// Commands for VDU 23, 0, n
//
#define VDP_GP					0x80	// General poll data
#define VDP_KEYCODE				0x81	// Keyboard data
#define VDP_CURSOR				0x82	// Cursor positions
#define VDP_SCRCHAR				0x83	// Character read from screen
#define VDP_SCRPIXEL			0x84	// Pixel read from screen
#define VDP_AUDIO				0x85	// Audio acknowledgement
#define VDP_MODE				0x86	// Get screen dimensions
#define VDP_RTC					0x87	// RTC
#define VDP_KEYSTATE			0x88	// Keyboard repeat rate and LED status
#define VDP_LOGICALCOORDS		0xC0	// Switch BBC Micro style logical coords on and off
#define VDP_TERMINALMODE		0xFF	// Switch to terminal mode

// And the corresponding return packets
// By convention, these match their VDP counterpart, but with the top bit reset
//
#define PACKET_GP				0x00	// General poll data
#define PACKET_KEYCODE			0x01	// Keyboard data
#define PACKET_CURSOR			0x02	// Cursor positions
#define PACKET_SCRCHAR			0x03	// Character read from screen
#define PACKET_SCRPIXEL			0x04	// Pixel read from screen
#define PACKET_AUDIO			0x05	// Audio acknowledgement
#define PACKET_MODE				0x06	// Get screen dimensions
#define PACKET_RTC				0x07	// RTC
#define PACKET_KEYSTATE			0x08	// Keyboard repeat rate and LED status

#define AUDIO_CHANNELS			3		// Number of audio channels
#define PLAY_SOUND_PRIORITY 	3		// Sound driver task priority with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest

#define LOGICAL_SCRW            1280    // As per the BBC Micro standard
#define LOGICAL_SCRH            1024

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE	0
#else
#define ARDUINO_RUNNING_CORE	1
#endif

// Function Prototypes
//
void debug_log(const char *format, ...);