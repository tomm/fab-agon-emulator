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

inline bool byteAvailable() {
	return VDPSerial.available() > 0;
}

inline uint8_t readByte() {
	return VDPSerial.read();
}

inline void writeByte(byte b) {
	VDPSerial.write(b);
}

// Read an unsigned byte from the serial port, with a timeout
// Returns:
// - Byte value (0 to 255) if value read, otherwise -1
//
int16_t readByte_t() {
	auto t = millis();

	while (millis() - t < 1000) {
		if (byteAvailable()) {
			return readByte();
		}
	}
	return -1;
}

// Read an unsigned word from the serial port, with a timeout
// Returns:
// - Word value (0 to 65535) if 2 bytes read, otherwise -1
//
uint32_t readWord_t() {
	auto l = readByte_t();
	if (l >= 0) {
		auto h = readByte_t();
		if (h >= 0) {
			return (h << 8) | l;
		}
	}
	return -1;
}

// Read an unsigned 24-bit value from the serial port, with a timeout
// Returns:
// - Value (0 to 16777215) if 3 bytes read, otherwise -1
//
uint32_t read24_t() {
	auto l = readByte_t();
	if (l >= 0) {
		auto m = readByte_t();
		if (m >= 0) {
			auto h = readByte_t();
			if (h >= 0) {
				return (h << 16) | (m << 8) | l;
			}
		}
	}
	return -1;
}

// Read an unsigned byte from the serial port (blocking)
//
uint8_t readByte_b() {
  	while (VDPSerial.available() == 0);
  	return readByte();
}

// Read an unsigned word from the serial port (blocking)
//
uint32_t readLong_b() {
  uint32_t temp;
  temp  =  readByte_b();		// LSB;
  temp |= (readByte_b() << 8);
  temp |= (readByte_b() << 16);
  temp |= (readByte_b() << 24);
  return temp;
}

// Discard a given number of bytes from input stream
//
void discardBytes(int length) {
	while (length > 0) {
		readByte_b();
		length--;
	}
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
