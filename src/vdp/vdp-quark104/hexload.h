#ifndef HEXLOAD_H
#define HEXLOAD_H

#include <stdbool.h>
extern void printFmt(const char *format, ...);
extern HardwareSerial DBGSerial;

#define DEF_LOAD_ADDRESS 0x040000
#define DEF_U_BYTE  ((DEF_LOAD_ADDRESS >> 16) & 0xFF)

void VDUStreamProcessor::sendKeycodeByte(uint8_t b, bool waitforack) {
	uint8_t packet[] = {b,0};
	send_packet(PACKET_KEYCODE, sizeof packet, packet);                    
	if(waitforack) readByte_b();
}

// Receive a single iHex Nibble from the external Debug serial interface
uint8_t getIHexNibble(void) {
	uint8_t nibble, input;

	while(!DBGSerial.available());
	input = toupper(DBGSerial.read());

	if((input >= '0') && input <='9') nibble = input - '0';
	else nibble = input - 'A' + 10;
	// illegal characters will be dealt with by checksum later
	return nibble;
}

// Receive a byte from the external Debug serial interface as two iHex nibbles
uint8_t getIHexByte(void) {
	uint8_t value;
	value = getIHexNibble() << 4;
	value |= getIHexNibble();
	return value;  
}

void echo_checksum(uint8_t ihexchecksum, uint8_t ez80checksum) {
	if(ihexchecksum) printFmt("X");
	if(ez80checksum) printFmt("*");
	if((ihexchecksum == 0) && (ez80checksum == 0)) printFmt(".");
}

void VDUStreamProcessor::vdu_sys_hexload(void) {
	uint8_t u,h,l;
	uint8_t bytecount;
	uint8_t recordtype;
	uint8_t data;
	uint8_t ihexchecksum,ez80checksum;

	bool done,defaultaddress,ez80checksumerror;
	uint16_t errorcount;

	printFmt("Receiving Intel HEX records - VDP:%d 8N1\r\n\r\n", SERIALBAUDRATE);
	u = DEF_U_BYTE;
	errorcount = 0;
	done = false;
	defaultaddress = true;

	while(!done) {
		data = 0;
		while(data != ':') if(DBGSerial.available() > 0) data = DBGSerial.read(); // hunt for start of recordtype

		bytecount = getIHexByte();  // number of bytes in this record
		h = getIHexByte();      	// middle byte of address
		l = getIHexByte();      	// lower byte of address 
		recordtype = getIHexByte(); // record type

		ihexchecksum = bytecount + h + l + recordtype;  // init control checksum
		ez80checksum = 1 + u + h + l + bytecount; 		// to be transmitted as a potential packet to the ez80

		switch(recordtype) {
			case 0: // data record
				if(defaultaddress) {
					printFmt("\r\nAddress 0x%02x0000 (default)\r\n", DEF_U_BYTE);
					defaultaddress = false;
				}
				sendKeycodeByte(1, true);			// ez80 data-package start indicator
				sendKeycodeByte(u, true);      		// transmit full address in each package  
				sendKeycodeByte(h, true);
				sendKeycodeByte(l, true);
				sendKeycodeByte(bytecount, true);	// number of bytes to send in this package
				while(bytecount--) {
					data = getIHexByte();
					sendKeycodeByte(data, false);
					ihexchecksum += data;			// update ihexchecksum
					ez80checksum += data;			// update checksum from bytes sent to the ez80
				}
				ez80checksum += readByte_b();		// get feedback from ez80 - a 2s complement to the sum of all received bytes, total 0 if no errorcount      
				ihexchecksum += getIHexByte();		// finalize checksum with actual checksum byte in record, total 0 if no errorcount
				if(ihexchecksum || ez80checksum) errorcount++;
				echo_checksum(ihexchecksum,ez80checksum);
				break;
			case 1: // end of file record
				getIHexByte();
				sendKeycodeByte(0, true);       	// end transmission
				done = true;
				break;
			case 4: // extended linear address record, only update U byte for next transmission to the ez80
				defaultaddress = false;
				ihexchecksum += getIHexByte();		// ignore top byte of 32bit address, only using 24bit
				u = getIHexByte();
				ihexchecksum += u;
				ihexchecksum += getIHexByte();		// finalize checksum with actual checksum byte in record, total 0 if no errorcount
				if(ihexchecksum) errorcount++;
				echo_checksum(ihexchecksum,0);		// only echo local checksum errorcount, no ez80<=>ESP packets in this case
				printFmt("\r\nAddress 0x%02x0000\r\n", u);
				break;
			default: // ignore other (non I32Hex) records
				break;
		}
	}
	if(errorcount) {
		printFmt("\r\n%d error(s)\r\n",errorcount);
	}
	else {
		printFmt("\r\nOK\r\n");
	}
	printFmt("VDP done\r\n");   
}

#endif // HEXLOAD_H