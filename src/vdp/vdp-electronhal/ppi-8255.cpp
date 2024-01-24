#include "ppi-8255.h"
#include "hal.h"
#include <fabutils.h>

PPI8255::PPI8255 ()
{
    for (int i=0;i<(sizeof (rows)/sizeof (uint16_t));i++)
        rows [i]=0xffff;
}

void PPI8255::write (uint8_t port, uint8_t value)
{
    switch (port)
    {
        case 0: 
            portA=value;
            // hal_printf ("PPI write port A: %02x\r\n",value);
            break;
        case 1: 
            portB=value;
            // hal_printf ("PPI write port B: %02x\r\n",value);
            break;
        case 2: 
            portC=value;
            // hal_printf ("PPI write port C: %02x\r\n",value);
            break;
        case 3: 
            control=value;
            // hal_printf ("PPI write control port: %02x\r\n",value);
            break;
    }
}
uint8_t PPI8255::read (uint8_t port)
{
    // hal_printf ("PPI read port %c\r\n",port+'A'); 
    switch (port)
    {
        case 0: 
            return uint8_t (rows[portC] & 0x00ff);
        case 1: 
            return uint8_t ((rows[portC] & 0xff00) >> 8);
        case 2: 
            return 0xff; // return portC if you want to emulate keyboard scanning, return 0xff if you just want keypads
        case 3: 
            return 0xff;
    }
    return 0xff;
}

// SG-1000
// PPI	Port	A							Port	B		
// Rows	D0	D1	D2	D3	D4	D5	D6	D7	D0	D1	D2	D3
// 0	'1'	'Q'	'A'	'Z'	ED	','	'K'	'I'	'8'	—	—	—
// 1	'2'	'W'	'S'	'X'	SPC	'.'	'L'	'O'	'9'	—	—	—
// 2	'3'	'E'	'D'	'C'	HC	'/'	';'	'P'	'0'	—	—	—
// 3	'4'	'R'	'F'	'V'	ID	PI	':'	'@'	'-'	—	—	—
// 4	'5'	'T'	'G'	'B'	—	DA	']'	'['	'^'	—	—	—
// 5	'6'	'Y'	'H'	'N'	—	LA	CR	—	YEN	—	—	FNC
// 6	'7'	'U'	'J'	'M'	—	RA	UA	—	BRK	GRP	CTL	SHF
// 7	1U	1D	1L	1R	1TL	1TR	2U	2D	2L	2R	2TL	2TR

void PPI8255::record_keypress (uint8_t ascii,uint8_t modifier,uint8_t vk,uint8_t down)
{
    // assume SG1000 for now
    // only supporting row 7
    if (down!=1)
    {
        // up, make bits 1
        switch (vk)
        {
            case fabgl::VK_UP:
                rows[7]=rows[7]|0b0000000000000001;
                break;
            case fabgl::VK_DOWN:
                rows[7]=rows[7]|0b0000000000000010;
                break;
            case fabgl::VK_LEFT:
                rows[7]=rows[7]|0b0000000000000100;
                break;
            case fabgl::VK_RIGHT:
                rows[7]=rows[7]|0b0000000000001000;
                break;
            case fabgl::VK_SPACE:
                rows[7]=rows[7]|0b0000000000010000;
                break;
            case fabgl::VK_LSHIFT:
            case fabgl::VK_RSHIFT:
                rows[7]=rows[7]|0b0000000000100000;
                break;
        }
    }
    else
    {
        // down, make bits 0
        switch (vk)
        {
            case fabgl::VK_UP:
                rows[7]=rows[7]&0b1111111111111110;
                break;
            case fabgl::VK_DOWN:
                rows[7]=rows[7]&0b1111111111111101;
                break;
            case fabgl::VK_LEFT:
                rows[7]=rows[7]&0b1111111111111011;
                break;
            case fabgl::VK_RIGHT:
                rows[7]=rows[7]&0b1111111111110111;
                break;
            case fabgl::VK_SPACE:
                rows[7]=rows[7]&0b1111111111101111;
                break;
            case fabgl::VK_LSHIFT:
            case fabgl::VK_RSHIFT:
                rows[7]=rows[7]&0b1111111111011111;
                break;
        }
    }
}