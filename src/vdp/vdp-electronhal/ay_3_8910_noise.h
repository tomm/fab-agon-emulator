#ifndef __AY_3_8910_H
#define __AY_3_8910_H

#include "fabgl.h"

class AY_3_8910_NoiseGenerator : public fabgl::WaveformGenerator {
public:
  AY_3_8910_NoiseGenerator();

  void setFrequency(int value);
  uint16_t frequency();

  int getSample();

private:
  uint16_t m_frequency;
  uint32_t m_noise;
  uint16_t m_counter;
  int m_prevsample;
};

#endif // __AY_3_8910_H