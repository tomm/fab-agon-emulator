#pragma once
#include "dispdrivers/vgabasecontroller.h"
#include "fabgl.h"
#include "fake_fabgl.h"
#include "fabutils.h"
#include "vdp.h"

// defined in video.ino, but used before definition!
extern void wait_eZ80();
extern void init_audio();
extern void copy_font();
extern void boot_screen();
extern void do_keyboard_terminal();
extern void do_cursor();
extern void do_keyboard();
extern void vdu(byte c);
extern void vdu_sys();
extern void audio_driver(void *parameters);
extern void send_packet(byte code, byte len, byte data[]);
extern char get_screen_char(int px, int py);
extern fabgl::Point scale(fabgl::Point p);
extern fabgl::Point scale(int x, int y);
extern fabgl::Point translate(fabgl::Point p);
extern fabgl::Point translate(int X, int Y);
extern bool cmp_char(uint8_t *c1, uint8_t *c2, int len);
extern void cursorRight();
extern void cursorLeft();
extern void cursorDown();
extern void cursorUp();
extern void cursorHome();
extern void cursorTab();
extern void vdu_origin();
extern void vdu_colour();
extern void vdu_gcol();
extern void vdu_palette();
extern void vdu_plot();
extern void vdu_plot_triangle(byte mode);
extern void vdu_plot_circle(byte mode);
extern void vdu_mode();
extern void vdu_sys_video();
extern void vdu_sys_scroll();
extern void vdu_sys_sprites();
extern void vdu_sys_udg(byte mode);
extern void vdu_sys_video_kblayout();
extern void vdu_sys_audio();
extern void vdu_sys_video_time();
extern void vdu_sys_keystate();
extern word play_note(byte channel, byte volume, word frequency, word duration);
extern void printFmt(const char *format, ...);
extern void set_mode(int mode);
extern void init_audio_channel(int channel);

extern fabgl::SoundGenerator		SoundGenerator;		// The audio class
