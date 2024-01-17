#ifndef __AUDIO_DRIVER_H
#define __AUDIO_DRIVER_H

void init_audio_driver();
uint8_t play_note(uint8_t channel, uint8_t volume, uint16_t frequency, uint16_t duration);
void setVolume(uint8_t channel, uint8_t volume);
void setFrequency(uint8_t channel, uint16_t frequency);
void setWaveform(uint8_t channel, int8_t waveformType);


#endif // __AUDIO_DRIVER_H