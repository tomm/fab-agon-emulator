#ifndef __TMS9918_H_
#define __TMS9918_H_

#include <stdint.h>

class TMS9918
{
    public:
        TMS9918 ();

        void write (uint8_t port, uint8_t value);
        uint8_t read (uint8_t port);
        void draw_screen (uint8_t* dest,int scanline);
        void set_display (fabgl::VGADirectController* dsply);
        void toggle_enable ();
        void cycle_screen2_debug ();

    private:
        int screen2_debug_view;
        bool display_enabled;
        uint8_t registers [8];
        uint8_t memory [16*1024];
        uint8_t data_register;
        uint8_t status_register;
        bool port99_write;
        uint16_t write_address;
        uint16_t read_address;
        uint8_t mode;
        bool mem16kb;
        bool screen_enable;
        bool interrupt_enable;
        bool sprite_size_16;
        bool sprite_magnify;
        uint16_t nametable;
        uint16_t colortable;    
        uint16_t patterntable;
        uint16_t sprite_attribute_table;
        uint16_t sprite_pattern_table;
        uint8_t textmode_colors;
        fabgl::VGADirectController* display;
        const uint16_t screen_width=320;
        const uint16_t screen_height=240;    
        const uint16_t vdp_width=256;
        const uint16_t vdp_height=192;
        const uint8_t  screen_border_vert=(screen_height - vdp_height)/2;
        const uint8_t  screen_border_horz=(screen_width - vdp_width)/2;
        uint16_t memory_bytes_written;
        uint16_t memory_bytes_read;
        uint16_t write_address_start;
        uint16_t read_address_start;
        uint8_t frame_counter;

        const fabgl::RGB222 colors [16] = {
            RGB222(0x00, 0x00, 0x00), //0 - transparent
            RGB222(0x00, 0x00, 0x00), //1 - black
            RGB222(0x02, 0x03, 0x02), //2 - medium green
            RGB222(0x01, 0x03, 0x01), //3 - light green
            RGB222(0x00, 0x00, 0x02), //4 - dark blue
            RGB222(0x01, 0x01, 0x03), //5 - light blue
            RGB222(0x02, 0x00, 0x00), //6 - dark red
            RGB222(0x01, 0x02, 0x03), //7 - cyan
            RGB222(0x03, 0x01, 0x01), //8 - medium red
            RGB222(0x03, 0x02, 0x02), //9 - light red
            RGB222(0x02, 0x02, 0x00), //A - dark yellow
            RGB222(0x03, 0x03, 0x01), //B - light yellow
            RGB222(0x00, 0x02, 0x00), //C - dark green
            RGB222(0x02, 0x01, 0x02), //D - magenta
            RGB222(0x02, 0x02, 0x02), //E - gray
            RGB222(0x03, 0x03, 0x03)  //F - white
        };
        void init_mode ();
        void write_register (uint8_t reg, uint8_t value);
        void draw_screen0 (uint8_t* dest,int scanline);
        void draw_screen1 (uint8_t* dest,int scanline);
        void draw_screen2 (uint8_t* dest,int scanline);
        void draw_screen2_patterns (uint8_t* dest,int scanline);
        void draw_screen2_colours (uint8_t* dest,int scanline);
        void draw_screen2_nametable_indexes (uint8_t* dest,int scanline);
        void draw_screen3 (uint8_t* dest,int scanline);
};

#endif // __TMS9918_H_