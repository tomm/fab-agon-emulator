#ifndef __HAL_H_
#define __HAL_H_

#include "Arduino.h"
#include "HardwareSerial.h"
#include "fabgl.h"

#define ez80_serial Serial2

#define UART_BR         1152000
#define UART_NA         -1
#define UART_TX         2
#define UART_RX         34
#define UART_RTS        13 // The ESP32 RTS pin
#define UART_CTS        14 // The ESP32 CTS pin
#define UART_RX_THRESH	64 // Point at which RTS is toggled

#define GPIO_ITRP       17 // VSync Interrupt Pin - for reference only

#define ESC 0x1b
#define CTRL_X 0x18
#define CTRL_Y 0x19
#define CTRL_Z 0x1a

extern HardwareSerial hal_serial;

void hal_printf (const char* format, ...);
void hal_hostpc_printf (const char* format, ...);
void hal_terminal_printf (const char* format, ...);
void hal_hostpc_serial_init ();
void hal_set_terminal (fabgl::Terminal*);
char hal_hostpc_serial_read ();
void hal_ez80_serial_init ();
void hal_ez80_serial_half_duplex ();
void hal_ez80_serial_full_duplex ();

#endif