//
// Title:			Agon Video BIOS
// Author:			Dean Belfield
// Contributors:	Steve Sims (refactoring)
// Created:			25/08/2023
// Last Updated:	25/08/2023
//

#ifndef AGON_VDP_PROTOCOL_H
#define AGON_VDP_PROTOCOL_H

#include <memory>
#include <HardwareSerial.h>

#include "agon.h"								// Configuration file

#define VDPSerial Serial2

void setupVDPProtocol() {
	VDPSerial.end();
	VDPSerial.setRxBufferSize(UART_RX_SIZE);					// Can't be called when running
	VDPSerial.begin(UART_BR, SERIAL_8N1, UART_RX, UART_TX);
	VDPSerial.setHwFlowCtrlMode(HW_FLOWCTRL_RTS, 64);			// Can be called whenever
	VDPSerial.setPins(UART_NA, UART_NA, UART_CTS, UART_RTS);	// Must be called after begin
}

// TODO remove the following - it's only here for cursor.h to send escape key when doing paged mode handling

inline void writeByte(uint8_t b) {
	VDPSerial.write(b);
}

// Send a packet of data to the MOS
//
void send_packet(uint8_t code, uint16_t len, uint8_t data[]) {
	writeByte(code + 0x80);
	writeByte(len);
	for (int i = 0; i < len; i++) {
		writeByte(data[i]);
	}
}

#endif // AGON_VDP_PROTOCOL_H
