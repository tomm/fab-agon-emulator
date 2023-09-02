#include "HardwareSerial.h"
#include <stdio.h>
#include <mutex>

HardwareSerial Serial2;

extern "C" void z80_send_to_vdp(uint8_t b)
{
	Serial2.writeToInQueue(b);
}

extern "C" bool z80_recv_from_vdp(uint8_t *out)
{
	return Serial2.readFromOutQueue(out);
}

int HardwareSerial::available() {
	std::unique_lock<std::mutex> lock(m_lock_in);
	return m_buf_in.size();
}
unsigned char HardwareSerial::read() {
	std::unique_lock<std::mutex> lock(m_lock_in);
	uint8_t v = m_buf_in.front();
	m_buf_in.pop_front();
	return v;
}
bool HardwareSerial::readFromOutQueue(uint8_t *out) {
	std::unique_lock<std::mutex> lock(m_lock_out);
	if (m_buf_out.size()) {
		*out = m_buf_out.front();
		m_buf_out.pop_front();
		return true;
	} else {
		return false;
	}
}
void HardwareSerial::write(uint8_t c) {
	std::unique_lock<std::mutex> lock(m_lock_out);
	m_buf_out.push_back(c);
}
void HardwareSerial::writeToInQueue(uint8_t c) {
	std::unique_lock<std::mutex> lock(m_lock_in);
	m_buf_in.push_back(c);
}
