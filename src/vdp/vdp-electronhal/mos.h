#ifndef __MOS_H_
#define __MOS_H_

#ifdef MOS_COMPATIBILITY

#include "hal.h"

const byte PACKET_KEYCODE	= 0x01;	// Keyboard data
const byte PACKET_SCRCHAR   = 0x03;
const byte PACKET_MODE      = 0x06;
const byte PACKET_CURSOR    = 0x02;
const byte PACKET_GP        = 0x00; // general poll data

// MOS compatibility layer
void mos_init ();
void mos_send_packet(byte code, byte len, byte data[]);
void mos_send_general_poll ();
void mos_send_vdp_mode ();
void mos_send_cursor_pos ();
void mos_col_left ();
void mos_col_right ();
void mos_set_column (uint8_t col);
void mos_send_character (byte ch);
void mos_send_virtual_key (fabgl::VirtualKeyItem item);
void mos_handle_escape_code ();

#endif

#endif