#include "Arduino.h"

// VDP uses various functions before their definition, so here
// vdp-console8.h gives those functions prototypes to avoid errors
#include "vdp_electron.h"

// Define soundGeneratorMutex, as this vdp doesn't actually have one
std::mutex soundGeneratorMutex;

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

#include "AgonElectronHAL/src/main.cpp"
#include "AgonElectronHAL/src/sn76489an.cpp"
#include "AgonElectronHAL/src/tms9918.cpp"
#include "AgonElectronHAL/src/zdi.cpp"
#include "AgonElectronHAL/src/updater.cpp"
#include "AgonElectronHAL/src/ppi-8255.cpp"
#include "AgonElectronHAL/src/mos.cpp"
#include "AgonElectronHAL/src/hal.cpp"
#include "AgonElectronHAL/src/eos.cpp"
#include "AgonElectronHAL/src/ay_3_8910.cpp"
#include "AgonElectronHAL/src/ay_3_8910_noise.cpp"
#include "AgonElectronHAL/src/audio_driver.cpp"
#include "AgonElectronHAL/src/audio_sample.cpp"
#include "AgonElectronHAL/src/audio_enhanced_samples_generator.cpp"
#include "AgonElectronHAL/src/audio_channel.cpp"
#include "AgonElectronHAL/src/audio_envelopes/frequency.cpp"
#include "AgonElectronHAL/src/audio_envelopes/volume.cpp"

uint8_t VolumeEnvelope::getVolume(uint8_t baseVolume, uint32_t elapsed, int32_t duration) {return 0;};
bool VolumeEnvelope::isReleasing(uint32_t elapsed, int32_t duration) {return false;};
bool VolumeEnvelope::isFinished(uint32_t elapsed, int32_t duration) {return false;};

uint16_t FrequencyEnvelope::getFrequency(uint16_t baseFrequency, uint32_t elapsed, int32_t duration) {return 0;};
bool FrequencyEnvelope::isFinished(uint32_t elapsed, int32_t duration) {return false;};