#ifndef __PPI_8255_H
#define __PPI_8255_H

#include <stdint.h>

class PPI8255
{
    private:
        uint8_t portA,portB,portC,control;
        uint16_t rows[8];
    public:
        PPI8255 ();
        void write (uint8_t port, uint8_t value);
        uint8_t read (uint8_t port);
        void record_keypress (uint8_t ascii,uint8_t modifier,uint8_t vk,uint8_t down);
};

#endif // __PPI_8255_H