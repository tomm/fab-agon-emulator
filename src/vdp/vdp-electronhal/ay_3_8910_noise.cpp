#include "ay_3_8910_noise.h"

AY_3_8910_NoiseGenerator::AY_3_8910_NoiseGenerator()
    : m_frequency (0) , 
      m_noise(0xFAB7), 
      m_counter (0)
{

}

void AY_3_8910_NoiseGenerator::setFrequency(int value)
{
    m_frequency = value;
}
uint16_t AY_3_8910_NoiseGenerator::frequency() 
{ 
    return m_frequency; 
}

// The Random Number Generator of the 8910 is a 17-bit shift register.
// The input to the shift register is bit0 XOR bit3 (bit0 is the
// output). Verified on real AY8910 and YM2149 chips.
//
// Fibonacci configuration:
//   random ^= ((random & 1) ^ ((random >> 3) & 1)) << 17;
//   random >>= 1;
// Galois configuration:
//   if (random & 1) random ^= 0x24000;
//   random >>= 1;
// or alternatively:
int AY_3_8910_NoiseGenerator::getSample()
{
    int sample;

    if (duration() == 0) {
    return 0;
    }

    if (m_counter>0)
    {
        m_counter--;
        sample = m_prevsample;
    }
    else 
    {
        m_counter = sampleRate()/m_frequency;

        // noise generator based on Galois LFSR
        // m_noise = (m_noise >> 1) ^ (-(m_noise & 1) & 0xB400u);
        m_noise = (m_noise >> 1) ^ ((m_noise & 1) << 13) ^ ((m_noise & 1) << 16);
        sample = m_noise&1 ? 127 : -128;
        m_prevsample = sample;
    }

    // process volume
    sample = sample * volume() / 127;

    decDuration();

    return sample;
}

