#include "fabgl.h"
#include "tms9918.h"
#include "hal.h"
#include "globals.h"

const int scanlinesPerCallback=2;
static void IRAM_ATTR drawScanline(void * arg, uint8_t * dest, int scanLine)
{
    TMS9918* vdp = (TMS9918*) arg;
    auto width  = ((fabgl::VGADirectController*)display)->getScreenWidth();
    auto height = ((fabgl::VGADirectController*)display)->getScreenHeight();

    // draws "scanlinesPerCallback" scanlines every time drawScanline() is called
    for (int i = 0; i < scanlinesPerCallback; ++i) 
    {
        vdp->draw_screen (dest,scanLine);
        
        // go to next scanline
        ++scanLine;
        dest += width;
    }

    // give signal to main loop to continue
    //
    if (scanLine == height) 
    {
        // signal end of screen
        vTaskNotifyGiveFromISR(mainTaskHandle, NULL);
    }
}

TMS9918::TMS9918 ()
{
    status_register = 0;
    mode = 0b111;
    read_address = 0;
    read_address_start = 0;
    write_address = 0;
    write_address_start = 0;
    mem16kb = true;
    port99_write = false;
    data_register = 0;
    screen_enable = true;
    interrupt_enable = true;
    sprite_size_16 = false;
    sprite_magnify = false;
    nametable = 0;
    colortable = 0;
    patterntable = 0;
    sprite_attribute_table = 0;
    sprite_pattern_table = 0;
    textmode_colors = 0;
    display = nullptr;
    display_enabled = true;
    memory_bytes_read = 0;
    memory_bytes_written = 0;
    frame_counter = 0;
    screen2_debug_view = 0;

    memset (memory,0,sizeof(memory));
}
void TMS9918::init_mode ()
{
    switch (mode) 
    {
        case 0b000 :
            if (display_enabled)
                set_display_direct ();
            else
                hal_terminal_printf (" Graphics I Mode\r\n");
            break;
        case 0b100 :
            if (display_enabled)
                set_display_direct ();
            else
                hal_terminal_printf (" Graphics II Mode\r\n");
            break;
        case 0b010 :
            if (!display_enabled)
                hal_terminal_printf (" Multicolor Mode\r\n");
            break;
        case 0b001 :
            if (display_enabled)
                set_display_direct ();
            else
                hal_terminal_printf (" Text Mode\r\n");
            break;
        default:
            break;
    }
}
void TMS9918::write_register (uint8_t reg, uint8_t value)
{
    uint8_t newmode;
    uint8_t oldmode;

    // store raw info
    registers[reg] = value;
    if (!display_enabled)
        hal_terminal_printf ("R%d=0x%02X\r\n",reg,value);
    // extract info
    switch (reg)
    {
        case 0:
            newmode = value & 0b00001110;
            newmode = newmode << 1;
            oldmode = mode;
            // clear all upper mode bits
            mode = mode & 0b00000011;
            // set new upper mode bits
            mode = mode | newmode;
            // have we selected a new mode?
            if (oldmode!=mode) 
                init_mode ();
            break;
        case 1:
            // handle the mode changes
            //////////////////////////
            newmode = value & 0b00011000;
            newmode = newmode >> 3;
            newmode = ((newmode & 1) << 1) + ((newmode & 2) >> 1); // rotate bits
            oldmode = mode;
            // clear two lower mode bits
            mode = mode & 0b00011100;
            // set new lower mode bits
            mode = mode | newmode;
            // have we selected a new mode?
            if (oldmode!=mode) 
                init_mode ();
            // handle the other bits
            ////////////////////////
            mem16kb =           (value & 0b10000000) > 0;
            screen_enable =     (value & 0b01000000) > 0;
            interrupt_enable =  (value & 0b00100000) > 0;
            sprite_size_16 =    (value & 0b00000010) > 0;
            sprite_magnify =    (value & 0b00000001) > 0;
            
            if (!display_enabled)
            {    
                if (mem16kb)            hal_terminal_printf(" 16kB VRAM\r\n");
                if (screen_enable)      hal_terminal_printf(" screen enabled\r\n");
                if (interrupt_enable)   hal_terminal_printf(" VDP interrupts enabled\r\n");
                if (sprite_size_16)     hal_terminal_printf(" 16x16 sprites\r\n");
                else                    hal_terminal_printf(" 8x8 sprites\r\n");
                if (sprite_magnify)     hal_terminal_printf(" magnified sprites\r\n");
            }
            break;
        case 2:
            nametable = value;
            nametable = nametable << 10;
            nametable = nametable & 0x3fff;
            if (!display_enabled)
                hal_terminal_printf(" nametable: %04X\r\n",nametable);
            break;
        case 3:
            if (mode!=0b100) 
            {
                // not screen 2
                colortable = value;
                colortable = colortable << 6;
                colortable = colortable & 0x3fff;
            }
            else
            {
                // screen 2
                if (value&0b10000000) {
                    colortable = 0x2000;
                }
            }
            if (!display_enabled)
            {
                if (mode==0b100)
                    hal_terminal_printf(" colortable: %04X %s\r\n",colortable,(value&0b01100000)==0b01100000?"(3x)":"(special)");
                else
                    hal_terminal_printf(" colortable: %04X\r\n",colortable);
            }
            break;
        case 4:
            if (mode!=0b100) 
            {
                // not screen 2
                patterntable = value;
                patterntable = patterntable << 11;
                patterntable = patterntable & 0x3fff;
            }
            else 
            {
                // mode 0b100 = screen 2
                if (value&0b100) {
                    patterntable = 0x2000;
                }
            }
            if (!display_enabled)
            {
                if (mode==0b100)
                    hal_terminal_printf(" patterntable: %04X %s\r\n",patterntable,(value&0b011)==0b011?"(3x)":"(special)");
                else
                    hal_terminal_printf(" patterntable: %04X\r\n",patterntable);
            }
            break;
        case 5:
            sprite_attribute_table = value;
            sprite_attribute_table = sprite_attribute_table << 7;
            sprite_attribute_table = sprite_attribute_table & 0x3fff;
            if (!display_enabled)
                hal_terminal_printf(" sprite_attribute_table: %04X\r\n",sprite_attribute_table);
            break;
        case 6:
            sprite_pattern_table = value;
            sprite_pattern_table = sprite_pattern_table << 11;
            sprite_pattern_table = sprite_pattern_table & 0x3fff;
            if (!display_enabled)
                hal_terminal_printf(" sprite_pattern_table: %04X\r\n",sprite_pattern_table);
            break;
        case 7:
            textmode_colors = value;
            if (!display_enabled)
                hal_terminal_printf(" textmode colors: %02X\r\n",textmode_colors);
            break;
        default:
            break;
    }              
}
void TMS9918::write (uint8_t port, uint8_t value)
{
    if (port==0x00) 
    {
        // if (!display_enabled)
        // {
        //     if (write_address==write_address_start)
        //         hal_terminal_printf (">%02X",value);
        //     else
        //     {
        //         hal_terminal_printf (" %02X",value);
        //         if ((write_address-write_address_start) % 16 == 0) {
        //             hal_terminal_printf ("\r\n");
        //         }
        //     }
        // }
        memory[write_address]=value;
        write_address = (write_address + 1) & 0x3fff;
        memory_bytes_written++;
    }
    if (port==0x01) 
    {
        if (port99_write == false)
        {
            port99_write = true;
            data_register = value;
        } 
        else 
        {
            // reset port99_write
            port99_write = false;
            // write register
            if ((value & 0b11000000) == 0b10000000)
                write_register (value & 0b00000111,data_register);
            // vram write address
            if ((value & 0b11000000) == 0b01000000)
            {
                // if (!display_enabled && memory_bytes_written>0)
                // {
                //     //hal_terminal_printf ("\r\n");
                //     hal_terminal_printf("VDP WR %04X - %04X (%04X)\r\n",write_address_start,write_address,memory_bytes_written);
                //     memory_bytes_written = 0;
                // }
                uint16_t temp = value & 0b00111111;
                temp = temp << 8;
                write_address = temp + data_register;
                write_address_start = write_address;
            }
            // vram read address
            if ((value & 0b11000000) == 0b00000000)
            {
                // if (!display_enabled && memory_bytes_read>0)
                // {
                //     hal_terminal_printf("VDP RD %04X - %04X (%04X)\r\n",read_address_start,read_address,memory_bytes_read);
                //     memory_bytes_read = 0;
                // }
                uint16_t temp = value & 0b00111111;
                temp = temp << 8;
                read_address = temp + data_register;
                read_address_start = read_address;
            }
        }
    }
}
uint8_t TMS9918::read (uint8_t port)
{
    uint8_t val=0;
    if (port==0x00) 
    {
        val = memory[read_address];
        read_address = (read_address + 1) & 0x3fff;
        memory_bytes_read++;
        // hal_terminal_printf ("read %02X from %04X\r\n",val, read_address);
    }
    if (port==0x01) 
    {
        val = status_register;
    }
    return val;
}

void TMS9918::set_display (fabgl::VGADirectController* dsply)
{
    display = dsply;

    if (display!=nullptr)
    {
        display->setScanlinesPerCallBack(scanlinesPerCallback);
        display->setDrawScanlineCallback(drawScanline,this);    
    }
}

void TMS9918::draw_screen0 (uint8_t* dest,int scanline)
{
    uint16_t nametable_idx;
    uint8_t  pattern;
    uint16_t pattern_idx;
    uint8_t  pixel_pattern;
    uint8_t  fgcolor;
    uint8_t  bgcolor;
    uint16_t posX,posY;
    uint8_t  character;
    uint8_t  pixel;

    // background color that shines through color 0
    bgcolor = display->createRawPixel(colors[textmode_colors & 0x0f]); // lower nibble;
    fgcolor = display->createRawPixel(colors[(textmode_colors & 0xf0) >> 4]); // high nibble;
    memset(dest, bgcolor, screen_width);

    if (!screen_enable)
         return;

    // we're drawing 320x192
    if (scanline<screen_border_vert || scanline >= (screen_height - screen_border_vert)) 
    {
        #ifdef SHOW_FRAME_RATE
        if (scanline==0)
            frame_counter++;
        if (frame_counter==60)
            frame_counter=0;
        VGA_PIXELINROW(dest, screen_border_horz+frame_counter*((screen_width-(screen_border_horz*2))/60)) = display->createRawPixel(colors[15]);
        #endif
        return;
    }
    posY = scanline - screen_border_vert;

    // draw 40 chars * 6 pixels for the posY specified
    //
    for (character=0;character<40;character++)
    {
        // get pattern and color idx
        nametable_idx = ((posY / 8) * 32) + character;
        pattern = memory [nametable + nametable_idx];
        pattern_idx = pattern * 8 + (posY % 8);
        // get pixel pattern
        pixel_pattern = memory[patterntable + pattern_idx];
        // calculate position of character on the posY
        posX = screen_border_horz + character * 6; // shift 32 pixels to the right to make 256 pixels center on our 320 width
        for (pixel=0;pixel<6;pixel++)
        {
            // check every bit in the pixel_pattern
            if (pixel_pattern & (1<<(7-pixel)))
            {
                VGA_PIXELINROW(dest, posX+pixel) = fgcolor;
            }
        }
    }
}

void TMS9918::draw_screen1 (uint8_t* dest,int scanline)
{
    uint16_t nametable_idx;
    uint8_t  pattern;
    uint16_t pattern_idx;
    uint8_t  pixel_pattern;
    uint16_t color_idx;
    uint8_t  color_byte;
    uint8_t  fgcolor;
    uint8_t  bgcolor;
    uint16_t posX,posY;
    uint8_t  character;
    uint8_t  pixel;

    uint16_t sprite_attributes;
    uint8_t sprite_Y;
    uint8_t sprite_X;
    uint8_t sprite_pattern_nr;
    uint8_t sprite_atts;
    uint8_t sprite_color_idx;
    fabgl::RGB222 sprite_color;
    uint8_t sprite_pixels_size;
    uint16_t sprite_pattern_address;

    // background color that shines through color 0
    bgcolor = display->createRawPixel(colors[textmode_colors & 0x0f]); // lower nibble;
    memset(dest, bgcolor, screen_width);

    if (!screen_enable)
         return;

    // we're drawing 320x192
    if (scanline<screen_border_vert || scanline >= (screen_height - screen_border_vert)) 
    {
        #ifdef SHOW_FRAME_RATE
        if (scanline==0)
            frame_counter++;
        if (frame_counter==60)
            frame_counter=0;
        VGA_PIXELINROW(dest, screen_border_horz+frame_counter*((screen_width-(screen_border_horz*2))/60)) = display->createRawPixel(colors[15]);
        #endif
        return;
    }
    posY = scanline - screen_border_vert;

    // draw 32 chars * 8 pixels for the posY specified
    //
    for (character=0;character<32;character++)
    {
        // get pattern and color idx
        nametable_idx = ((posY / 8) * 32) + character;
        pattern = memory [nametable + nametable_idx];
        pattern_idx = pattern * 8 + (posY % 8);
        color_idx = pattern/8;
        // get pixel pattern
        pixel_pattern = memory[patterntable + pattern_idx];
        // get colors
        color_byte = memory[colortable + color_idx];
        // get RGB222 values of MSX palet
        fgcolor = display->createRawPixel(colors[color_byte >> 4]);  // upper nibble
        bgcolor = display->createRawPixel(colors[color_byte & 0x0f]);// lower nibble
        // calculate position of character on the posY
        posX = screen_border_horz + character * 8; // shift 32 pixels to the right to make 256 pixels center on our 320 width
        for (pixel=0;pixel<8;pixel++)
        {
            // check every bit in the pixel_pattern
            if (pixel_pattern & (1<<(7-pixel)))
            {
                VGA_PIXELINROW(dest, posX+pixel) = fgcolor;
            }
            else
            {
                if ((color_byte & 0x0f) != 0)
                    // when not transparent have a pixel with the bgcolor
                    VGA_PIXELINROW(dest, posX+pixel) = bgcolor;
            }
        }
    }

    // sprites
    for (int sprite=0;sprite<32;sprite++)
    {
        // get sprite attributes
        sprite_attributes = sprite_attribute_table + sprite * 4;
        sprite_Y =          memory[sprite_attributes+0];
        sprite_X =          memory[sprite_attributes+1];
        sprite_pattern_nr = memory[sprite_attributes+2];
        sprite_atts =       memory[sprite_attributes+3];

        // do we need to continue with lower priority sprites?
        if (sprite_Y == 208)
            break; // last sprite in the series
        
        // sprite is actually 1 pixel lower
        sprite_Y++;

        // get derived values
        sprite_color_idx =  sprite_atts & 0xf;
        sprite_color =      colors[sprite_color_idx];
        fgcolor = display->createRawPixel(sprite_color); 
        sprite_pixels_size= sprite_size_16?16:8;

        // resize sprite when magnified
        if (sprite_magnify)
            sprite_pixels_size = sprite_pixels_size * 2;
        
        // is sprite visible on this scanline?
        if (posY < sprite_Y || 
            posY >= sprite_Y+sprite_pixels_size)
            continue; // not visible

        // is sprite transparent colour?
        if (sprite_color_idx == 0)
            continue; // not visible
        
        // draw sprite pixels
        //
        // two segments of 8 pixels when we have a 16x16 sprite, or one segment of  pixels when 8x8
        for (uint8_t hor_sprite_segment = 0; hor_sprite_segment<(sprite_size_16?2:1); hor_sprite_segment++)
        {
            // get start address of pattern in memory
            sprite_pattern_address = sprite_pattern_table + (sprite_pattern_nr * 8) + (posY - sprite_Y) + (16 * hor_sprite_segment);
            // get pixel pattern
            pixel_pattern = memory[sprite_pattern_address];
            // calculate position of sprite on the posY
            posX = screen_border_horz + sprite_X + hor_sprite_segment*(sprite_pixels_size/2) + 1; 
            for (pixel=0;pixel<8;pixel++)
            {
                if (posX+pixel < screen_width-screen_border_horz)
                {
                    // check every bit in the pixel_pattern
                    if (pixel_pattern & (1<<(7-pixel)))
                        VGA_PIXELINROW(dest, posX+pixel) = fgcolor;
                }
            }
        }
    }    
}

void TMS9918::draw_screen2 (uint8_t* dest,int scanline)
{
    uint16_t nametable_idx;
    uint8_t  pattern;
    uint16_t pattern_idx;
    uint8_t  pixel_pattern;
    uint16_t color_idx;
    uint8_t  color_byte;
    uint8_t  fgcolor;
    uint8_t  bgcolor;
    uint16_t posX,posY;
    uint8_t  character;
    uint8_t  pixel;

    uint16_t sprite_attributes;
    uint8_t sprite_Y;
    uint8_t sprite_X;
    uint8_t sprite_pattern_nr;
    uint8_t sprite_atts;
    uint8_t sprite_color_idx;
    fabgl::RGB222 sprite_color;
    uint8_t sprite_pixels_size;
    uint16_t sprite_pattern_address;

    // background color that shines through color 0
    bgcolor = display->createRawPixel(colors[textmode_colors & 0x0f]); // lower nibble;
    memset(dest, bgcolor, screen_width);

    if (!screen_enable)
         return;

    // we're drawing 320x192
    if (scanline<screen_border_vert || scanline >= (screen_height - screen_border_vert)) 
    {
        #ifdef SHOW_FRAME_RATE
        if (scanline==0)
            frame_counter++;
        if (frame_counter==60)
            frame_counter=0;
        VGA_PIXELINROW(dest, screen_border_horz+frame_counter*((screen_width-(screen_border_horz*2))/60)) = display->createRawPixel(colors[15]);
        #endif
        return;
    }
    posY = scanline - screen_border_vert;

    // in graphics II only
    uint16_t _patterntable = patterntable;
    uint16_t _colortable = colortable;
    uint16_t third = posY/64;
    
    // support for alternative pattern table modes in screen 2
    switch (registers[4]&0b011)
    {
        case 0b11:
            // every 1/3 of the screen is drawn by a separate table
            _patterntable += 0x800 * third;
            break;
        case 0b10:
            // top+bottom in one table and middle in a separate table
            if (third==1)
                _patterntable += 0x800;
            break;
        case 0b01:
            // top+middle in one table and bottom in a separate table
            if (third==2)
                _patterntable += 0x800;
            break;
        default:
            // otherwise top, middle + bottom in one pattern table
            break;
    }
    // support for alternative color table modes in screen 2
    switch (registers[3]&0b01100000)
    {
        case 0b01100000:
            // every 1/3 of the screen is drawn by a separate table
            _colortable += 0x800 * third;
            break;
        case 0b00100000:
            // top+bottom in one table and middle in a separate table
            if (third==1)
                _colortable += 0x800;
            break;
        case 0b01000000:
            // top+middle in one table and bottom in a separate table
            if (third==2)
                _colortable += 0x800;
            break;
        defaut:
            // otherwise top, middle + bottom in one color table
            break;
    }

    // draw 32 chars * 8 pixels for the posY specified
    //
    for (character=0;character<32;character++)
    {
        // get pattern and color idx
        nametable_idx = ((posY / 8) * 32) + character;
        pattern = memory [nametable + nametable_idx];
        pattern_idx = pattern * 8 + (posY % 8);
        color_idx = pattern * 8 + (posY % 8);
        // get pixel pattern
        pixel_pattern = memory[_patterntable + pattern_idx];
        // get colors
        color_byte = memory[_colortable + color_idx];
        // get RGB222 values of MSX palet
        fgcolor = display->createRawPixel(colors[color_byte >> 4]);  // upper nibble
        bgcolor = display->createRawPixel(colors[color_byte & 0x0f]);// lower nibble
        // calculate position of character on the posY
        posX = screen_border_horz + character * 8; // shift 32 pixels to the right to make 256 pixels center on our 320 width
        for (pixel=0;pixel<8;pixel++)
        {
            // check every bit in the pixel_pattern
            if (pixel_pattern & (1<<(7-pixel)))
            {
                VGA_PIXELINROW(dest, posX+pixel) = fgcolor;
            }
            else
            {
                if ((color_byte & 0x0f) != 0)
                    // when not transparent have a pixel with the bgcolor
                    VGA_PIXELINROW(dest, posX+pixel) = bgcolor;
            }
        }
    }

    // sprites
    for (int sprite=0;sprite<32;sprite++)
    {
        // get sprite attributes
        sprite_attributes = sprite_attribute_table + sprite * 4;
        sprite_Y =          memory[sprite_attributes+0];
        sprite_X =          memory[sprite_attributes+1];
        sprite_pattern_nr = memory[sprite_attributes+2];
        sprite_atts =       memory[sprite_attributes+3];

        // do we need to continue with lower priority sprites?
        if (sprite_Y == 208)
            break; // last sprite in the series
        
        // sprite is actually 1 pixel lower
        sprite_Y++;

        // get derived values
        sprite_color_idx =  sprite_atts & 0xf;
        sprite_color =      colors[sprite_color_idx];
        fgcolor = display->createRawPixel(sprite_color); 
        sprite_pixels_size= sprite_size_16?16:8;

        // resize sprite when magnified
        if (sprite_magnify)
            sprite_pixels_size = sprite_pixels_size * 2;
        
        // is sprite visible on this scanline?
        if (posY < sprite_Y || 
            posY >= sprite_Y+sprite_pixels_size)
            continue; // not visible

        // is sprite transparent colour?
        if (sprite_color_idx == 0)
            continue; // not visible
        
        // draw sprite pixels
        //
        // two segments of 8 pixels when we have a 16x16 sprite, or one segment of  pixels when 8x8
        for (uint8_t hor_sprite_segment = 0; hor_sprite_segment<(sprite_size_16?2:1); hor_sprite_segment++)
        {
            // get start address of pattern in memory
            sprite_pattern_address = sprite_pattern_table + (sprite_pattern_nr * 8) + (posY - sprite_Y) + (16 * hor_sprite_segment);
            // get pixel pattern
            pixel_pattern = memory[sprite_pattern_address];
            // calculate position of sprite on the posY
            posX = screen_border_horz + sprite_X + hor_sprite_segment*(sprite_pixels_size/2) + 1; 
            for (pixel=0;pixel<8;pixel++)
            {
                if (posX+pixel < screen_width-screen_border_horz)
                {
                    // check every bit in the pixel_pattern
                    if (pixel_pattern & (1<<(7-pixel)))
                        VGA_PIXELINROW(dest, posX+pixel) = fgcolor;
                }
            }
        }
    }
}
void TMS9918::draw_screen2_patterns (uint8_t* dest,int scanline)
{
    // draw the patterns in the patterntable in white and black
    uint16_t nametable_idx;
    uint8_t  pattern;
    uint16_t pattern_idx;
    uint8_t  pixel_pattern;
    uint16_t color_idx;
    uint8_t  color_byte;
    uint8_t  fgcolor;
    uint8_t  bgcolor;
    uint16_t posX,posY;
    uint8_t  character;
    uint8_t  pixel;

    uint16_t sprite_attributes;
    uint8_t sprite_Y;
    uint8_t sprite_X;
    uint8_t sprite_pattern_nr;
    uint8_t sprite_atts;
    uint8_t sprite_color_idx;
    fabgl::RGB222 sprite_color;
    uint8_t sprite_pixels_size;
    uint16_t sprite_pattern_address;

    // background color that shines through color 0
    bgcolor = display->createRawPixel(colors[textmode_colors & 0x0f]); // lower nibble;
    fgcolor = display->createRawPixel(colors[15]);
    memset(dest, bgcolor, screen_width);

    if (!screen_enable)
         return;

    // we're drawing 320x192
    if (scanline<screen_border_vert || scanline >= (screen_height - screen_border_vert)) 
    {
        #ifdef SHOW_FRAME_RATE
        if (scanline==0)
            frame_counter++;
        if (frame_counter==60)
            frame_counter=0;
        VGA_PIXELINROW(dest, screen_border_horz+frame_counter*((screen_width-(screen_border_horz*2))/60)) = display->createRawPixel(colors[15]);
        #endif
        return;
    }
    posY = scanline - screen_border_vert;

    // in graphics II only
    uint16_t _patterntable = patterntable;
    uint16_t third = posY/64;
    
    // support for alternative pattern table modes in screen 2
    switch (registers[4]&0b011)
    {
        case 0b11:
            // every 1/3 of the screen is drawn by a separate table
            _patterntable += 0x800 * third;
            break;
        case 0b10:
            // top+bottom in one table and middle in a separate table
            if (third==1)
                _patterntable += 0x800;
            break;
        case 0b01:
            // top+middle in one table and bottom in a separate table
            if (third==2)
                _patterntable += 0x800;
            break;
        default:
            // otherwise top, middle + bottom in one pattern table
            break;
    }

    // draw 32 chars * 8 pixels for the posY specified
    //
    for (character=0;character<32;character++)
    {
        // get pattern and color idx
        pattern = ((posY / 8) * 32) + character;
        pattern_idx = pattern * 8 + (posY % 8);
        // get pixel pattern
        pixel_pattern = memory[_patterntable + pattern_idx];
        // calculate position of character on the posY
        posX = screen_border_horz + character * 8; // shift 32 pixels to the right to make 256 pixels center on our 320 width
        for (pixel=0;pixel<8;pixel++)
        {
            // check every bit in the pixel_pattern
            if (pixel_pattern & (1<<(7-pixel)))
            {
                VGA_PIXELINROW(dest, posX+pixel) = fgcolor;
            }
            else
            {
                if ((color_byte & 0x0f) != 0)
                    // when not transparent have a pixel with the bgcolor
                    VGA_PIXELINROW(dest, posX+pixel) = bgcolor;
            }
        }
    }
}

void TMS9918::draw_screen2_colours (uint8_t* dest,int scanline)
{
    // draw patterns with the colours in the colour table
    uint16_t nametable_idx;
    uint8_t  pattern;
    uint16_t pattern_idx;
    uint8_t  pixel_pattern;
    uint16_t color_idx;
    uint8_t  color_byte;
    uint8_t  fgcolor;
    uint8_t  bgcolor;
    uint16_t posX,posY;
    uint8_t  character;
    uint8_t  pixel;

    uint16_t sprite_attributes;
    uint8_t sprite_Y;
    uint8_t sprite_X;
    uint8_t sprite_pattern_nr;
    uint8_t sprite_atts;
    uint8_t sprite_color_idx;
    fabgl::RGB222 sprite_color;
    uint8_t sprite_pixels_size;
    uint16_t sprite_pattern_address;

    // background color that shines through color 0
    bgcolor = display->createRawPixel(colors[textmode_colors & 0x0f]); // lower nibble;
    memset(dest, bgcolor, screen_width);

    if (!screen_enable)
         return;

    // we're drawing 320x192
    if (scanline<screen_border_vert || scanline >= (screen_height - screen_border_vert)) 
    {
        #ifdef SHOW_FRAME_RATE
        if (scanline==0)
            frame_counter++;
        if (frame_counter==60)
            frame_counter=0;
        VGA_PIXELINROW(dest, screen_border_horz+frame_counter*((screen_width-(screen_border_horz*2))/60)) = display->createRawPixel(colors[15]);
        #endif
        return;
    }
    posY = scanline - screen_border_vert;

    // in graphics II only
    uint16_t _patterntable = patterntable;
    uint16_t _colortable = colortable;
    uint16_t third = posY/64;
    
    // support for alternative pattern table modes in screen 2
    switch (registers[4]&0b011)
    {
        case 0b11:
            // every 1/3 of the screen is drawn by a separate table
            _patterntable += 0x800 * third;
            break;
        case 0b10:
            // top+bottom in one table and middle in a separate table
            if (third==1)
                _patterntable += 0x800;
            break;
        case 0b01:
            // top+middle in one table and bottom in a separate table
            if (third==2)
                _patterntable += 0x800;
            break;
        default:
            // otherwise top, middle + bottom in one pattern table
            break;
    }
    // support for alternative color table modes in screen 2
    switch (registers[3]&0b01100000)
    {
        case 0b01100000:
            // every 1/3 of the screen is drawn by a separate table
            _colortable += 0x800 * third;
            break;
        case 0b00100000:
            // top+bottom in one table and middle in a separate table
            if (third==1)
                _colortable += 0x800;
            break;
        case 0b01000000:
            // top+middle in one table and bottom in a separate table
            if (third==2)
                _colortable += 0x800;
            break;
        defaut:
            // otherwise top, middle + bottom in one color table
            break;
    }

    // draw 32 chars * 8 pixels for the posY specified
    //
    for (character=0;character<32;character++)
    {
        // get pattern and color idx
        pattern = ((posY / 8) * 32) + character;
        pattern_idx = pattern * 8 + (posY % 8);
        color_idx = pattern * 8 + (posY % 8);
        // get pixel pattern
        pixel_pattern = memory[_patterntable + pattern_idx];
        // get colors
        color_byte = memory[_colortable + color_idx];
        // get RGB222 values of MSX palet
        fgcolor = display->createRawPixel(colors[color_byte >> 4]);  // upper nibble
        bgcolor = display->createRawPixel(colors[color_byte & 0x0f]);// lower nibble
        // calculate position of character on the posY
        posX = screen_border_horz + character * 8; // shift 32 pixels to the right to make 256 pixels center on our 320 width
        for (pixel=0;pixel<8;pixel++)
        {
            // check every bit in the pixel_pattern
            if (pixel_pattern & (1<<(7-pixel)))
            {
                VGA_PIXELINROW(dest, posX+pixel) = fgcolor;
            }
            else
            {
                if ((color_byte & 0x0f) != 0)
                    // when not transparent have a pixel with the bgcolor
                    VGA_PIXELINROW(dest, posX+pixel) = bgcolor;
            }
        }
    }
}
void TMS9918::draw_screen2_nametable_indexes (uint8_t* dest,int scanline)
{
    // draw all 768 name table indexes
    uint16_t nametable_idx;
    uint8_t  pattern;
    uint16_t pattern_idx;
    uint8_t  pixel_pattern;
    uint16_t color_idx;
    uint8_t  color_byte;
    uint8_t  fgcolor;
    uint8_t  bgcolor;
    uint16_t posX,posY;
    uint8_t  character;
    uint8_t  pixel;

    uint16_t sprite_attributes;
    uint8_t sprite_Y;
    uint8_t sprite_X;
    uint8_t sprite_pattern_nr;
    uint8_t sprite_atts;
    uint8_t sprite_color_idx;
    fabgl::RGB222 sprite_color;
    uint8_t sprite_pixels_size;
    uint16_t sprite_pattern_address;

    // background color that shines through color 0
    bgcolor = display->createRawPixel(colors[textmode_colors & 0x0f]); // lower nibble;
    memset(dest, bgcolor, screen_width);

    if (!screen_enable)
         return;

    // we're drawing 320x192
    if (scanline<screen_border_vert || scanline >= (screen_height - screen_border_vert)) 
    {
        #ifdef SHOW_FRAME_RATE
        if (scanline==0)
            frame_counter++;
        if (frame_counter==60)
            frame_counter=0;
        VGA_PIXELINROW(dest, screen_border_horz+frame_counter*((screen_width-(screen_border_horz*2))/60)) = display->createRawPixel(colors[15]);
        #endif
        return;
    }
    posY = scanline - screen_border_vert;

    // in graphics II only
    uint16_t _patterntable = patterntable;
    uint16_t _colortable = colortable;
    uint16_t third = posY/64;
    
    // support for alternative pattern table modes in screen 2
    switch (registers[4]&0b011)
    {
        case 0b11:
            // every 1/3 of the screen is drawn by a separate table
            _patterntable += 0x800 * third;
            break;
        case 0b10:
            // top+bottom in one table and middle in a separate table
            if (third==1)
                _patterntable += 0x800;
            break;
        case 0b01:
            // top+middle in one table and bottom in a separate table
            if (third==2)
                _patterntable += 0x800;
            break;
        default:
            // otherwise top, middle + bottom in one pattern table
            break;
    }
    // support for alternative color table modes in screen 2
    switch (registers[3]&0b01100000)
    {
        case 0b01100000:
            // every 1/3 of the screen is drawn by a separate table
            _colortable += 0x800 * third;
            break;
        case 0b00100000:
            // top+bottom in one table and middle in a separate table
            if (third==1)
                _colortable += 0x800;
            break;
        case 0b01000000:
            // top+middle in one table and bottom in a separate table
            if (third==2)
                _colortable += 0x800;
            break;
        defaut:
            // otherwise top, middle + bottom in one color table
            break;
    }

    // draw 32 chars * 8 pixels for the posY specified
    //
    for (character=0;character<32;character++)
    {
        // get pattern and color idx
        nametable_idx = ((posY / 8) * 32) + character;
        pattern = memory [nametable + nametable_idx];
        pattern_idx = pattern * 8 + (posY % 8);
        color_idx = pattern * 8 + (posY % 8);
        // get pixel pattern
        pixel_pattern = memory[_patterntable + pattern_idx];
        // get colors
        color_byte = memory[_colortable + color_idx];
        // get RGB222 values of MSX palet
        fgcolor = display->createRawPixel(RGB888(pattern,pattern,pattern));  // upper nibble
        bgcolor = display->createRawPixel(RGB888(255-pattern,255-pattern,255-pattern));// lower nibble
        // calculate position of character on the posY
        posX = screen_border_horz + character * 8; // shift 32 pixels to the right to make 256 pixels center on our 320 width
        for (pixel=0;pixel<8;pixel++)
        {
            // check every bit in the pixel_pattern
            if (pixel_pattern & (1<<(7-pixel)))
            {
                VGA_PIXELINROW(dest, posX+pixel) = fgcolor;
            }
            else
            {
                VGA_PIXELINROW(dest, posX+pixel) = bgcolor;
            }
        }
    }
}

void TMS9918::draw_screen3 (uint8_t* dest,int scanline)
{
}

void TMS9918::draw_screen (uint8_t* dest,int scanline)
{
    // not set up yet
    if (display==nullptr)
        return;

    switch (mode) 
    {
        case 0b001 :
            draw_screen0 (dest,scanline);       // Text Mode
            break;
        case 0b000 :
            draw_screen1 (dest,scanline);       // Graphics I Mode
            break;
        case 0b100 :
            switch (screen2_debug_view)
            {
                case 0:
                    draw_screen2 (dest,scanline);       // Graphics II Mode
                    break;
                case 1:
                    draw_screen2_patterns (dest,scanline);
                    break;
                case 2:
                    draw_screen2_colours (dest,scanline);
                    break;
                case 3:
                    draw_screen2_nametable_indexes (dest,scanline);
                    break;
            }
            
            break;
        case 0b010 :
            draw_screen3 (dest,scanline);       // Multicolor Mode
            break;
    }
}

void TMS9918::toggle_enable ()
{
    display_enabled = !display_enabled;
    hal_terminal_printf ("\r\n(display %s)\r\n*",display_enabled?"enabled":"disabled");
}

void TMS9918::cycle_screen2_debug ()
{
    screen2_debug_view++;
    if (screen2_debug_view>3)
        screen2_debug_view=0;
    hal_terminal_printf ("\r\n(screen2 debug %d)\r\n*",screen2_debug_view);
}