#pragma once
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
extern void do_mouse();
extern bool processTerminal();
extern void vdu(byte c);
extern void vdu_sys();
extern void audio_driver(void *parameters);
extern void send_packet(uint8_t code, uint16_t len, uint8_t data[]);
extern char get_screen_char(int px, int py);
extern fabgl::Point scale(fabgl::Point p);
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
extern void vdu_sys_udg(char mode);
extern void vdu_sys_video_kblayout();
extern void vdu_sys_audio();
extern void vdu_sys_video_time();
extern void vdu_sys_keystate();
extern uint8_t play_note(uint8_t channel, uint8_t volume, uint16_t frequency, uint16_t duration);
extern void printFmt(const char *format, ...);

extern fabgl::SoundGenerator		*soundGenerator;		// The audio class
extern std::mutex soundGeneratorMutex;
