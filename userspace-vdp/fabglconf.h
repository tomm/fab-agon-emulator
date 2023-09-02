/*
  Created by Fabrizio Di Vittorio (fdivitto2013@gmail.com) - <http://www.fabgl.com>
  Copyright (c) 2019-2022 Fabrizio Di Vittorio.
  All rights reserved.


* Please contact fdivitto2013@gmail.com if you need a commercial license.


* This library and related software is available under GPL v3.

  FabGL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  FabGL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with FabGL.  If not, see <http://www.gnu.org/licenses/>.
 */


#pragma once



/**
 * @file
 *
 * @brief This file contains FabGL library configuration settings, like
 *        number of supported colors, maximum usable memory, terminal fonts, etc...
 *
 */


#include <esp_wifi.h>



/** Defines ESP32 XTAL frequency in Hz. */
#define FABGLIB_XTAL 40000000


/** Blink (cursor, text blink, ...) period in ms. */
#define FABGLIB_DEFAULT_BLINK_PERIOD_MS 500


/** Size of display controller primitives queue */
#define FABGLIB_DEFAULT_DISPLAYCONTROLLER_QUEUE_SIZE 1024


#define FABGLIB_TERMINAL_FLOWCONTROL_RXFIFO_MIN_THRESHOLD 20

#define FABGLIB_TERMINAL_FLOWCONTROL_RXFIFO_MAX_THRESHOLD 300


/** Size (in bytes) of primitives dynamic buffers. Used by primitives like drawPath and fillPath to contain path points. */
#define FABGLIB_PRIMITIVES_DYNBUFFERS_SIZE 512


/** Number of characters the terminal can "write" without pause (increase if you have loss of characters in serial port). */
#define FABGLIB_DEFAULT_TERMINAL_INPUT_QUEUE_SIZE 1024


/** Number of characters the terminal can store from keyboard. */
#define FABGLIB_TERMINAL_OUTPUT_QUEUE_SIZE 32


#define FABGLIB_TERMINAL_XOFF_THRESHOLD (FABGLIB_DEFAULT_TERMINAL_INPUT_QUEUE_SIZE / 2)
#define FABGLIB_TERMINAL_XON_THRESHOLD  (FABGLIB_DEFAULT_TERMINAL_INPUT_QUEUE_SIZE / 4)


/** Stack size of the task that processes Terminal input stream. */
#define FABGLIB_DEFAULT_TERMINAL_INPUT_CONSUMER_TASK_STACK_SIZE 2048


/** Priority of the task that processes Terminal input stream. */
#define FABGLIB_CHARS_CONSUMER_TASK_PRIORITY 5


/** Stack size of the task that reads keys from keyboard and send ANSI/VT codes to output stream in Terminal */
#define FABGLIB_DEFAULT_TERMINAL_KEYBOARD_READER_TASK_STACK_SIZE 2048


/** Priority of the task that reads keys from keyboard and send ANSI/VT codes to output stream in Terminal */
#define FABGLIB_KEYBOARD_READER_TASK_PRIORITY 5


/** Stack size of the task that converts scancodes to Virtual Keys Keyboard */
#define FABGLIB_DEFAULT_SCODETOVK_TASK_STACK_SIZE 1500


/** Priority of the task that converts scancodes to Virtual Keys Keyboard */
#define FABGLIB_SCODETOVK_TASK_PRIORITY 5


/** Defines the underline position starting from character bottom (0 = bottom of the character). */
#define FABGLIB_UNDERLINE_POSITION 0


/** Optional feature. If enabled terminal fonts are cached in RAM for better performance. */
#define FABGLIB_CACHE_FONT_IN_RAM 0


/** Optional feature. Enables Keyboard.virtualKeyToString() method */
#define FABGLIB_HAS_VirtualKeyO_STRING 1


/** Optional feature. Use b/a coeff to fine tune frequency. Unfortunately output is not very stable when enabled! */
#define FABGLIB_USE_APLL_AB_COEF 0


/** Maximum number of allowed parameters in CSI escape sequence. */
#define FABGLIB_MAX_CSI_PARAMS 12


/** Maximum chars in DCS escape sequence. */
#define FABGLIB_MAX_DCS_CONTENT 12


/** To reduce memory overhead the viewport is allocated as few big buffers. This parameter defines the maximum number of these big buffers. */
#define FABGLIB_VIEWPORT_MEMORY_POOL_COUNT 128


/** Size (in bytes) of largest block to maintain free */
#define FABGLIB_MINFREELARGESTBLOCK 40000


/** Size of virtualkey queue */
#define FABGLIB_KEYBOARD_VIRTUALKEY_QUEUE_SIZE 32


/** Size of mouse events queue */
#define FABGLIB_MOUSE_EVENTS_QUEUE_SIZE 64


/** Core to use for CPU intensive tasks like VGA signals generation in VGATextController or VGAXXController */
#define FABGLIB_VIDEO_CPUINTENSIVE_TASKS_CORE (WIFI_TASK_CORE_ID ^ 1) // using the same core of WiFi may cause flickering


/** Stack size of primitives drawing task of paletted based VGA controllers */
#define FABGLIB_VGAPALETTEDCONTROLLER_PRIMTASK_STACK_SIZE 1200


/** Priority of primitives drawing task of paletted based VGA controllers */
#define FABGLIB_VGAPALETTEDCONTROLLER_PRIMTASK_PRIORITY 5



// debug options
#define FABGLIB_TERMINAL_DEBUG_REPORT                0  // this must be enabled to make below settings active
#define FABGLIB_TERMINAL_DEBUG_REPORT_IN_CODES       0
#define FABGLIB_TERMINAL_DEBUG_REPORT_INQUEUE_CODES  0  // use as alternative to FABGLIB_TERMINAL_DEBUG_REPORT_IN_CODES when write() is called inside isr
#define FABGLIB_TERMINAL_DEBUG_REPORT_OUT_CODES      0
#define FABGLIB_TERMINAL_DEBUG_REPORT_ESC            0
#define FABGLIB_TERMINAL_DEBUG_REPORT_DESCS          0
#define FABGLIB_TERMINAL_DEBUG_REPORT_DESCSALL       0
#define FABGLIB_TERMINAL_DEBUG_REPORT_OSC_CONTENT    0
#define FABGLIB_TERMINAL_DEBUG_REPORT_UNSUPPORT      1
#define FABGLIB_TERMINAL_DEBUG_REPORT_ERRORS         1
#define FABGLIB_VGAXCONTROLLER_PERFORMANCE_CHECK     0
#define FABGLIB_CVBSCONTROLLER_PERFORMANCE_CHECK     0


/************ Preset Resolution Modelines ************/

#define VGA_320x200_60HzD "\"320x200@60HzD\" 25.175 320 328 376 400 200 226 227 262 -HSync -VSync DoubleScan"

/** Modeline for 320x200@70Hz resolution - the same of VGA_640x200_70Hz with horizontal halved */
#define VGA_320x200_70Hz "\"320x200@70Hz\" 12.5875 320 328 376 400 200 206 207 224 -HSync -VSync DoubleScan"

/** Modeline for 320x200@75Hz resolution */
#define VGA_320x200_75Hz "\"320x200@75Hz\" 12.93 320 352 376 408 200 208 211 229 -HSync -VSync DoubleScan"

/** Modeline for 320x200@75Hz retro resolution */
#define VGA_320x200_75HzRetro "\"320x200@75Hz\" 12.93 320 352 376 408 200 208 211 229 -HSync -VSync DoubleScan MultiScanBlank"

/** Modeline for 320x240@60Hz resolution */
#define QVGA_320x240_60Hz "\"320x240@60Hz\" 12.6 320 328 376 400 240 245 246 262 -HSync -VSync DoubleScan"

/** Modeling for 320x256@60Hz resolution (1/4 of 640x512@60Hz resolution, can do 64 colors */
#define SVGA_320x256_60Hz "\"320x256@60Hz\" 27     320 332 360 424 256 257 258 267 -HSync -VSync QuadScan"

/** Modeline for 400x300@60Hz resolution */
#define VGA_400x300_60Hz "\"400x300@60Hz\" 20 400 420 484 528 300 300 302 314 -HSync -VSync DoubleScan"

/** Modeline for 480x300@75Hz resolution */
#define VGA_480x300_75Hz "\"480x300@75Hz\" 31.22 480 504 584 624 300 319 322 333 -HSync -VSync DoubleScan"

/** Modeline for 512x384@60Hz resolution */
#define VGA_512x384_60Hz "\"512x384@60Hz\" 32.5 512 524 592 672 384 385 388 403 -HSync -VSync DoubleScan"

/** Modeline for 640x480@60Hz resolution */
#define VGA_640x480_60Hz "\"640x480@60Hz\" 25.175 640 656 752 800 480 490 492 525 -HSync -VSync"

/** Modeline for 640x512@60Hz resolution (for pixel perfect 1280x1024 double scan resolution) */
#define QSVGA_640x512_60Hz "\"640x512@60Hz\" 54     640 664 720 844 512 513 515 533 -HSync -VSync DoubleScan"

/** Modeline for 800x600@60Hz resolution */
#define SVGA_800x600_60Hz "\"800x600@60Hz\" 40 800 840 968 1056 600 601 605 628 -HSync -VSync"

/** Modeline for 960x540@60Hz resolution */
#define SVGA_960x540_60Hz "\"960x540@60Hz\" 37.26 960 976 1008 1104 540 542 548 563 +hsync +vsync"

/** Modeline for 1024x768@60Hz resolution */
#define SVGA_1024x768_60Hz "\"1024x768@60Hz\" 65 1024 1048 1184 1344 768 771 777 806 -HSync -VSync"

/** Modeline for 1024x768@70Hz resolution */
#define SVGA_1024x768_70Hz "\"1024x768@70Hz\" 75 1024 1048 1184 1328 768 771 777 806 -HSync -VSync"

/** Modeline for 1024x768@75Hz resolution */
#define SVGA_1024x768_75Hz "\"1024x768@75Hz\" 78.80 1024 1040 1136 1312 768 769 772 800 +HSync +VSync"

/** Modeline for 1280x720@60Hz resolution */
#define SVGA_1280x720_60Hz "\"1280x720@60Hz\" 74.48 1280 1468 1604 1664 720 721 724 746 +hsync +vsync"

