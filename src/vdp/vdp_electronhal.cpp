#include "Arduino.h"

// VDP uses various functions before their definition, so here
// vdp-console8.h gives those functions prototypes to avoid errors
#include "vdp_electronhal.h"

fabgl::SoundGenerator *getVDPSoundGenerator() {
	return &SoundGenerator;
}

void noInterrupts ()
{

}
void delayMicroseconds (int msecs)
{

}
void interrupts ()
{

}
void pinMode (uint8_t pin, uint8_t mode)
{

}

#define vTaskNotifyGiveFromISR( xTaskToNotify, pxHigherPriorityTaskWoken ) 0
#define digitalRead(p) 0
typedef uint32_t u32_t;
typedef uint16_t u16_t;

#define INPUT 0
#define OUTPUT 1
#define ARDUINO_RUNNING_CORE 0

#include "vdp-electronhal/main.cpp"
#include "vdp-electronhal/sn76489an.cpp"
#include "vdp-electronhal/tms9918.cpp"
#include "vdp-electronhal/zdi.cpp"
#include "vdp-electronhal/updater.cpp"
#include "vdp-electronhal/ppi-8255.cpp"
#include "vdp-electronhal/mos.cpp"
#include "vdp-electronhal/hal.cpp"
#include "vdp-electronhal/eos.cpp"
#include "vdp-electronhal/ay_3_8910.cpp"
#include "vdp-electronhal/ay_3_8910_noise.cpp"
#include "vdp-electronhal/audio_driver.cpp"
#include "vdp-electronhal/audio_sample.cpp"
#include "vdp-electronhal/audio_enhanced_samples_generator.cpp"
#include "vdp-electronhal/audio_channel.cpp"
#include "vdp-electronhal/audio_envelopes/frequency.cpp"
#include "vdp-electronhal/audio_envelopes/volume.cpp"

uint8_t VolumeEnvelope::getVolume(uint8_t baseVolume, uint32_t elapsed, int32_t duration) {return 0;};
bool VolumeEnvelope::isReleasing(uint32_t elapsed, int32_t duration) {return false;};
bool VolumeEnvelope::isFinished(uint32_t elapsed, int32_t duration) {return false;};

uint16_t FrequencyEnvelope::getFrequency(uint16_t baseFrequency, uint32_t elapsed, int32_t duration) {return 0;};
bool FrequencyEnvelope::isFinished(uint32_t elapsed, int32_t duration) {return false;};