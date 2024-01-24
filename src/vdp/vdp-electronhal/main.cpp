/*
 * Title:           Electron - HAL
 *                  a playful alternative to Quark
 *                  quarks and electrons combined are matter.
 * Author:          Mario Smit (S0urceror)
 * Credits:         Dean Belfield
 *                  Koenraad Van Nieuwenhove (Cocoacrumbs)
 * Created:         01/03/2023
 */

#include "fabgl.h"
#include "globals.h"
#include "hal.h"
#include "zdi.h"
#pragma message ("Building a Electron OS compatible version of Electron HAL")
#include "eos.h"
#include "tms9918.h"
#include "ay_3_8910.h"
#include "sn76489an.h"
#include "ppi-8255.h"
#include "audio_driver.h"
#include "updater.h"

fabgl::PS2Controller        ps2;
fabgl::VGABaseController*	display = nullptr;
fabgl::Terminal*            terminal;
TMS9918                     vdp;
AY_3_8910                   psg;
SN76489AN                   psg2;
PPI8255                     ppi;
bool                        ignore_escape = false;
bool                        display_mode_direct=false;
TaskHandle_t                mainTaskHandle;
vdu_updater                 updater;
                    

#define VDU_SYSTEM          0
#define VDU_GP              0x80
#define VDU_CURSOR          0x82
#define VDU_SCRCHAR         0x83
#define VDU_MODE            0x86
#define VDU_UPDATE          0xA1
#define OS_UNKNOWN          0
#define OS_MOS              1
#define OS_MOS_FULLDUPLEX   2
#define OS_ELECTRON         128
#define OS_ELECTRON_SG1000  129

uint8_t os_identifier   =   OS_UNKNOWN;

#define PACKET_GP       0x00	// General poll data
#define PACKET_KEYCODE  0x01
#define PACKET_CURSOR   0x02
#define PACKET_SCRCHAR  0x03
#define PACKET_MODE     0x06

void mos_send_packet(uint8_t code, uint16_t len, uint8_t data[]) {
	ez80_serial.write(code + 0x80);
	ez80_serial.write(len);
	for (int i = 0; i < len; i++) {
		ez80_serial.write(data[i]);
	}
}

void boot_screen()
{
    // initialize terminal
    terminal->write("\e[44;37m"); // background: blue, foreground: white
    terminal->write("\e[2J");     // clear screen
    terminal->write("\e[1;1H");   // move cursor to 1,1

    hal_printf("\r\nElectron - HAL - version %d.%d.%d\r\n",HAL_major,HAL_minor,HAL_revision);
}

void set_display_direct ()
{
    if (display_mode_direct)
        return;
    display_mode_direct = true;
    // destroy old display instance
    if (display)
    {
        terminal->deactivate();
        // destroy old display
        display->end();
        delete display;
        display = nullptr;
        // destroy old terminal
        //terminal->end();    
        // remove terminal from HAL
        hal_set_terminal (nullptr);
        // TODO: commented out below because deleting terminal turns off SoundGenerator
        //delete terminal;
    }
    // create new display instance
    display = new fabgl::VGADirectController ();

    // tell vdp to use the new display
    vdp.set_display((fabgl::VGADirectController*) display);

    // setup new display   
    display->begin();
    display->setResolution(QVGA_320x240_60Hz);

    //hal_hostpc_printf ("display set to DIRECT\r\n");
}

void set_display_normal (bool force)
{
    if (!force && !display_mode_direct)
        return;
        
    display_mode_direct = false;

    if (display)
    {
        display->end();
        delete display;
        display = nullptr;
        // tell vdp to not use the display
        vdp.set_display((fabgl::VGADirectController*) nullptr);
    }
    // create new display instance
    display = new fabgl::VGA16Controller ();
    display->begin();
    display->setResolution(VGA_640x480_60Hz);
    
    // setup terminal
    terminal = new fabgl::Terminal ();
    terminal->begin(display);
    terminal->enableCursor(true);

    // let hal know what our terminal is
    hal_set_terminal (terminal);

    // hello world
    boot_screen ();
}

void do_serial_hostpc ()
{
    byte ch;
    while (true)
    {
        // characters in the buffer?
        if((ch=hal_hostpc_serial_read())>0) 
        {
            if (!zdi_mode() && ch==CTRL_Z) 
            {
                // CTRL-Z?
                zdi_enter();
            }
            else if (ch==CTRL_Y)
            {
                // CTRL-Y
                vdp.toggle_enable ();
            }
            else if (ch==CTRL_X)
            {
                // CTRL-X
                //vdp.cycle_screen2_debug ();
                if (display_mode_direct)
                    set_display_normal ();
                else
                    set_display_direct ();
            }
            else
            {
                if (zdi_mode())
                    // handle keys on the ESP32
                    zdi_process_cmd (ch);
                else
                {
                    // MOS wants a whole packet for 1 character
                    // allows to detect special key combo's
                    if (os_identifier==OS_MOS || os_identifier==OS_MOS_FULLDUPLEX)
                    {
                        uint8_t packet[] = {
                            ch,
                            0,
                            0,
                            1,
                        };
                        mos_send_packet(PACKET_KEYCODE, sizeof packet, packet);
                    }
                    else 
                        // ElectronOS is simpler, just send the character
                        ez80_serial.write(ch);
                }
            }
        } 
        else
            break;
    }
}

void do_keys_ps2 ()
{
    fabgl::Keyboard *kb = ps2.keyboard();
    fabgl::VirtualKeyItem item;
    
    if(kb->getNextVirtualKey(&item, 0)) 
    {
        uint8_t modifier =  item.CTRL		<< 0 |
                                item.SHIFT		<< 1 |
                                item.LALT		<< 2 |
                                item.RALT		<< 3 |
                                item.CAPSLOCK	<< 4 |
                                item.NUMLOCK	<< 5 |
                                item.SCROLLLOCK << 6 |
                                item.GUI		<< 7;

        // MOS wants a whole packet
        // allows to detect special key combo's
        switch (os_identifier)
        {
            case OS_MOS:
            case OS_MOS_FULLDUPLEX:
            {
                uint8_t packet[] = {
                    item.ASCII,
                    modifier,
                    item.vk,
                    item.down,
                };
                mos_send_packet(PACKET_KEYCODE, sizeof packet, packet);
                break;
            }
            case OS_ELECTRON_SG1000:
            {
                ppi.record_keypress (item.ASCII,modifier,item.vk,item.down);
                break;
            }
            case OS_ELECTRON:
            {
                // ElectronOS gets just the ASCII code
                // only special keys get send differently
                if (item.ASCII!=0)
                {
                    if (item.down)
                        ez80_serial.write(item.ASCII);
                    if (item.ASCII==0x20) // space
                    {
                        // also send virtual keycode (for SNSMAT)
                        ez80_serial.write(0b11000000); // signals command mode
                        ez80_serial.write(fabgl::VK_SPACE);
                        ez80_serial.write(item.down?1:0);
                    }
                }
                else 
                {
                    // send virtual keycode (for SNSMAT)
                    ez80_serial.write(0b11000000); // signals command mode
                    ez80_serial.write(item.vk);
                    ez80_serial.write(item.down?1:0);
                }
                break;
            }
            
        }
    }
}

void setup()
{
    mainTaskHandle = xTaskGetCurrentTaskHandle();

    // Disable the watchdog timers
    disableCore0WDT(); delay(200);								
	disableCore1WDT(); delay(200);

    // setup connection from ESP to EZ80
    hal_ez80_serial_init();
    
    // setup keyboard/PS2
    ps2.begin(PS2Preset::KeyboardPort0, KbdMode::CreateVirtualKeysQueue);

    // setup VGA display
    set_display_normal (true);

    // setup audio driver
    init_audio_driver ();
    psg.init ();
    //psg2.init ();

    // setup serial to hostpc
    hal_hostpc_serial_init ();

    // boot screen and wait for ElectronOS on EZ80
    boot_screen();
    // if (!eos_wakeup ())
    // {
    //     hal_printf ("Electron - OS - not running\r\n");
    //     while (1)
    //         do_serial_hostpc(); // wait for ZDI mode hotkey to reprogram ElectronOS
    // }
}

// Minimal implementation just to make ElectronHAL somewhat compatible with Quark MOS
// ElectronOS has implemented PACKET_GP to make it operable with Quark VDP.
// ElectronOS sends a 2 while MOS sends a 1 in this phase. We use this to distinguish both
void process_vdu ()
{
    uint8_t mode,cmd,new_os_identifier;
    // read VDU mode ID
    ez80_serial.readBytes(&mode,1);
    switch (mode)
    {
        case VDU_SYSTEM:
            ez80_serial.readBytes(&cmd,1);
            switch (cmd)
            {
                case VDU_GP:
                    // read EZ80 OS identifier
                    // MOS always sends a 1
                    // ElectronOS sends a 2, and mimics this behaviour to be somewhat compatible with MOS/VDP
                    ez80_serial.readBytes(&new_os_identifier,1);
                    if (new_os_identifier!=os_identifier)
                    {
                        os_identifier = new_os_identifier;
                        if (os_identifier==OS_MOS) 
                        {
                            hal_ez80_serial_half_duplex ();
                            hal_printf ("MOS personality activated [HD]\r\n");
                        }
                        if (os_identifier==OS_MOS_FULLDUPLEX) 
                        {
                            hal_ez80_serial_full_duplex ();
                            hal_printf ("MOS personality activated [FD]\r\n");
                        }
                        if (os_identifier==OS_ELECTRON)
                        {
                            hal_ez80_serial_full_duplex ();
                            hal_printf ("ElectronOS personality activated\r\n");
                        }
                        if (os_identifier==OS_ELECTRON_SG1000)
                        {
                            hal_ez80_serial_full_duplex ();
                            hal_printf ("SG1000 personality activated\r\n");
                        }
                    }
                    mos_send_packet(PACKET_GP, sizeof(new_os_identifier), &new_os_identifier); // only MOS packet that ElectronOS understands
                    break;
                case VDU_CURSOR:
                {
                    uint8_t cursor_packet[] = { 1, 1 };
                    mos_send_packet(PACKET_CURSOR, sizeof(cursor_packet), cursor_packet);
                    break;
                }
                case VDU_SCRCHAR: 
                {				
                    // VDU 23, 0, &83, x; y;
                    uint16_t x,y;
                    uint8_t scrchar=0;
                    ez80_serial.readBytes((uint8_t*) &x,2);
                    ez80_serial.readBytes((uint8_t*) &y,2);
                    // if this is requested after unlocking the updater
                    // then it's verify the 'unlocked!' response
                    if (!updater.locked)
                        scrchar = updater.get_unlock_response(x-8);
                    uint8_t packet[] = {
                        scrchar,
                    };
	                mos_send_packet(PACKET_SCRCHAR, sizeof packet, packet);
                    break;
                }
                case VDU_MODE:
                {
                    int canvasW = 640;
                    int canvasH = 480;
                    int fontW = 8,fontH = 8;
                    uint8_t mode_packet[] = {
                        (uint8_t) (canvasW & 0xFF),			// Width in pixels (L)
                        (uint8_t) ((canvasW >> 8) & 0xFF),	// Width in pixels (H)
                        (uint8_t) (canvasH & 0xFF),			// Height in pixels (L)
                        (uint8_t) ((canvasH >> 8) & 0xFF),	// Height in pixels (H)
                        (uint8_t) (canvasW / fontW),		// Width in characters (byte)
                        (uint8_t) (canvasH / fontH),		// Height in characters (byte)
                        64,				// Colour depth
                        0,							// The video mode number
                    };
                    mos_send_packet(PACKET_MODE, sizeof mode_packet, mode_packet);
                    break;
                }
                case VDU_UPDATE:
                {
                    uint8_t update_mode;
                    ez80_serial.readBytes(&update_mode,1);
                    updater.command(update_mode);
                    break;
                }
                default:
                    hal_printf ("Unknown VDU system command:%d",cmd);
                    break;
            }
            break;       
        default:
            hal_printf ("Unknown VDU mode:%d",mode);
            break;
    }
}

void process_character (byte c)
{
    //hal_hostpc_printf ("%02X",c);
                    
    if (ignore_escape)
    {   
        // absorb 2 byte and 3 byte (0x78,0x79) escape codes
        if (!(c==0x78 || c==0x79))
            ignore_escape=false;
        return;
    }
    if (c>=0x20 && c<=0x7f) 
    {
        // normal ASCII?
        hal_printf ("%c",c);
    }
    else
    {
        // special character or ElectronOS escape code
        switch (c)
        {
            case '\r':
            case '\n':
            case 0x07: // BELL
                hal_printf ("%c",c);
                break;
            case 0x08: // backspace
                hal_printf ("\b \b");
                break;
            case 0x09: // TAB
                hal_printf ("%c",c);
                break;
            case 0x0c: // formfeed
                terminal->clear ();
                break;
            case CTRL_W:
                break;
            case CTRL_Z:
                break;
            case ESC:
                ignore_escape=true;
                break;
            default:
                hal_printf ("ERR:0x%02X;",c);
                break;
        }
    }
}

void process_virtual_io_cmd (uint8_t cmd)
{
    // strip high bit
    uint8_t subcmd = (cmd & 0b01100000) >> 5;
    uint8_t port,value;
    uint8_t normal_out_req[2],repeatable_io_req[3],fillable_io_req[4];
    int repeat_length,i;

    switch (subcmd)
    {
        case 0b00: // IO requests take 3 bytes
            switch (cmd&0b00000111)
            {
                case 0b000:
                    // OUTput, no repeat, no fill
                    // single OUTput
                    if (ez80_serial.readBytes (normal_out_req,2)==2)
                    {
                        port = normal_out_req[0];
                        value = normal_out_req[1];
                        
                        if (port==0x98 || port==0x99) // MSX TMS9918
                            vdp.write (port-0x98,value);
                        if (port==0xa0 || port==0xa1) // MSX AY-3-8910
                            psg.write (port,value);                            
                        if (port==0xBE || port==0xBF) // SG-1000 TMS9918
                            vdp.write (port-0xbe,value);
                        if (port==0x7e || port==0x7f) // SG-1000 SN76489AN
                            psg2.write (value);
                        if (port==0xdc || port==0xdd || port==0xde|| port==0xdf)
                            ppi.write (port-0xdc,value);
                        //hal_printf ("OUT (%02X), %02X\r\n",port,value);
                    }
                    break;
                case 0b010:
                    // OUTput, LDIRVM, multiple times, different values
                    if (ez80_serial.readBytes (repeatable_io_req,3)==3)
                    {
                        port = repeatable_io_req[0];
                        repeat_length = (repeatable_io_req[1]<<8) + repeatable_io_req[2];
                        //hal_printf ("%02X,%02X,%02X - OUT repeat %d times\r\n",repeatable_io_req[0],repeatable_io_req[1],repeatable_io_req[2],repeat_length);
                        for (i=0;i<repeat_length;i++)
                        {
                            if (ez80_serial.readBytes (&value,1)==1)
                            {
                                //hal_printf ("OUT (%02X), %02X\r\n",port,value);
                                if (port==0x98 || port==0x99)
                                    vdp.write (port-0x98,value);
                                // if (port==0xa0 || port==0xa1)
                                //     psg.write (port,value);
                            }
                        }
                        //hal_printf ("OUT (%02Xh), xx - %d repeated\r\n",port,i);
                    }
                    break;
                case 0b100:
                    // OUTput, FILL, multiple times, same value
                    if (ez80_serial.readBytes (fillable_io_req,4)==4)
                    {
                        port = fillable_io_req[0];
                        repeat_length = (fillable_io_req[1]<<8) + fillable_io_req[2];
                        value = fillable_io_req[3];
                        for (i=0;i<repeat_length;i++)
                        {
                            if (port==0x98 || port==0x99)
                                vdp.write (port-0x98,value);
                            // if (port==0xa0 || port==0xa1)
                            //     psg.write (port,value);
                        }
                        //hal_printf ("OUT (%02Xh), %02Xh - %d filled\r\n",port,value,i);
                    }
                    break;
                case 0b001:
                    // INput, single value
                    if (ez80_serial.readBytes (&port,1)==1)
                    {
                        if (port==0x98 || port==0x99)
                            value = vdp.read (port-0x98);
                        if (port==0xbe || port==0xbf)
                            value = vdp.read (port-0xbe);                            
                        if (port==0xa2)
                            value = psg.read (port);
                        if (port==0xdc || port==0xdd || port==0xde|| port==0xdf)
                            value = ppi.read (port-0xdc);

                        ez80_serial.write(value);
                        //hal_printf ("IN A,(%02X) => %02X\r\n",port,value);
                    }
                    break;
                case 0b011:
                    // INput, LDIRMV, multiple times, multiple value
                    if (ez80_serial.readBytes (repeatable_io_req,3)==3)
                    {
                        port = repeatable_io_req[0];
                        repeat_length = (repeatable_io_req[1]<<8) + repeatable_io_req[2];
                        //hal_printf ("%02Xh,%02Xh,%02Xh - IN repeat %d times\r\n",repeatable_io_req[0],repeatable_io_req[1],repeatable_io_req[2],repeat_length);
                        for (i=0;i<repeat_length;i++)
                        {
                            //hal_printf ("IN (%02Xh) => %02Xh\r\n",port,value);
                            if (port==0x98 || port==0x99)
                                value = vdp.read (port-0x98);
                            // if (port==0xa2)
                            //     value = psg.read (port);

                            ez80_serial.write(value);
                        }
                        //hal_printf ("IN (%02Xh), xx - %d repeated\r\n",port,i);
                    }
                    break;
            }
            break;
        default:
            hal_printf ("Unsupported in/out to: 0x%02X\r\n",cmd);
            break;
    }
}

void do_serial_ez80 ()
{
    int read_character;
    while (true)
    {
        // check if ez80 has send us something
        read_character = ez80_serial.read();
        if (read_character!=-1)
        {
            // read character
            uint8_t ch = (uint8_t) read_character;
            if (ch & 0x80 /*0b10000000*/)
                process_virtual_io_cmd (ch);
            else if (ch==CTRL_W)        
                process_vdu ();          
            else
                process_character (ch);
        }
        else
            break;
    }
}

void loop()
{
    do_keys_ps2();
    do_serial_hostpc();
    do_serial_ez80();
}
