
/*
 * Title:           Electron - HAL
 *                  a playful alternative to Quark
 *                  quarks and electrons combined are matter.
 * Author:          Mario Smit (S0urceror)
*/

// MOS interoperability layer (not complete)

#include "mos.h"

#ifdef MOS_COMPATIBILITY

uint8_t col,row;

void mos_init ()
{
    col=1;
    row=1;
}

// Send a packet of data to the MOS
//
void mos_send_packet(byte code, byte len, byte data[]) {
    ez80_serial.write(code + 0x80);
    ez80_serial.write(len);
    for(int i = 0; i < len; i++) {
        ez80_serial.write(data[i]);
    }
}

void mos_send_general_poll ()
{
    byte uv = ez80_serial.read();
    byte packet[] = {
        uv
    };
    mos_send_packet(PACKET_GP, sizeof packet, packet);
}

void mos_send_vdp_mode ()
{
    byte packet[] = {
        0x80,	 						// Width in pixels (L)
        0x02,       					// Width in pixels (H)
        0xe0,							// Height in pixels (L)
        0x01,       					// Height in pixels (H)
        80 ,	                        // Width in characters (byte)
        60 ,	                        // Height in characters (byte)
        64,       						// Colour depth
    };
    mos_send_packet(PACKET_MODE, sizeof packet, packet);
}
void mos_send_cursor_pos ()
{
    byte packet[] = {
        (byte) (col),
        (byte) (row),
    };
    //hal_printf ("x:%d,y:%d",x,y);
    mos_send_packet(PACKET_CURSOR, sizeof packet, packet);	
}
void mos_send_character (byte ch) 
{
    byte packet[] = {
                    ch,
                    0,
                };
    // send to MOS on the EZ80
    mos_send_packet(PACKET_KEYCODE, sizeof packet, packet);
}
void mos_send_virtual_key (fabgl::VirtualKeyItem item)
{
    byte keycode = 0;						// Last pressed key code
    byte modifiers = 0;						// Last pressed key modifiers
    // send to MOS on the EZ80
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
    byte packet[] = {
        item.ASCII,
        modifiers,
        item.vk,
        item.down,
    };
    mos_send_packet(PACKET_KEYCODE, sizeof packet, packet);
}
void mos_col_left ()
{
    if (col>1)
        col--;
}
void mos_col_right ()
{
    col++;
}
void mos_set_column (uint8_t c)
{
    col = c;
}
void mos_handle_escape_code ()
{
    byte c;
    if ((c=ez80_serial.read())==0)
    {
        c=((byte) ez80_serial.read());
        switch (c)
        {
            case 0x80: // general poll
                mos_send_general_poll ();
                break;
            case 0x86: // VDP_MODE
                mos_send_vdp_mode ();
                break;
            case 0x82: // VDP_CURSOR
                mos_send_cursor_pos ();
                break;
            default:
                hal_printf ("VDP:0x%02X;",c);
                break;
        }
    }
    else
        hal_printf ("MOS:0x%02X;",c);

    
}

#endif