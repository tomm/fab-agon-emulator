#ifndef __SN76489AN_H_
#define __SN76489AN_H_

#include <stdint.h>

#define SG1000_MASTER_FREQUENCY 3579545
#define SG1000_MASTER_FREQUENCY_DIV (SG1000_MASTER_FREQUENCY/32)
#define FABGL_AMPLITUDE_MULTIPLIER (128/16)


class SN76489AN
{
    public:
        SN76489AN ();

        void init ();
        void write (uint8_t value);

    private:
        uint8_t     register_select;
        uint16_t    toneA;
        uint16_t    toneB;
        uint16_t    toneC;
        uint8_t     noise;
        uint8_t     amplA;
        uint8_t     amplB;
        uint8_t     amplC;
        uint8_t     amplNoise;
        uint8_t     fb;
        uint8_t     nf;
        bool        first_byte;

        void updateSound (uint8_t channel, uint8_t amp, uint16_t freq);
};

#endif // __SN76489AN_H_