#pragma once

#include <stdint.h>
#include <deque>
#include <mutex>

struct HardwareSerial {
    int available();
    unsigned char read();
    void write(unsigned char);

    void end() {}
    void setRxBufferSize(int) {}
    void setHwFlowCtrlMode(int, int) {}
    void begin(int, int, int, int) {}
    void setPins(int, int, int, int) {}

    void writeToInQueue(uint8_t b);
    bool readFromOutQueue(uint8_t *out);

private:
    std::deque<uint8_t> m_buf_in;
    std::deque<uint8_t> m_buf_out;
    std::mutex m_lock_in;
    std::mutex m_lock_out;
};
