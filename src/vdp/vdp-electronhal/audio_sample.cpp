//
// Title:			Agon Video BIOS - Audio class
// Author:			Dean Belfield
// Contributors:	Steve Sims (enhancements for more sophisticated audio support)
//                  S0urceror (PlatformIO compatibility)
// Created:			05/09/2022
// Last Updated:	08/10/2023
//
// Modinfo:

#include "audio_sample.h"

audio_sample::audio_sample(uint32_t length) :
    length(length)    
{
    data = (int8_t *) PreferPSRAMAlloc(length);
    if (!data) {
        //("audio_sample: failed to allocate %d bytes\n\r", length);
    }
}

audio_sample::~audio_sample() {
	// iterate over channels
	for (auto channelPair : this->channels) {
		auto channelRef = channelPair.second;
		if (!channelRef.expired()) {
			auto channel = channelRef.lock();
			//debug_log("audio_sample: removing sample from channel %d\n\r", channel->channel());
			// Remove sample from channel
			channel->setWaveform(AUDIO_WAVE_DEFAULT, nullptr);
		}
	}

	if (this->data) {
		free(this->data);
	}

	//debug_log("audio_sample cleared\n\r");
}
