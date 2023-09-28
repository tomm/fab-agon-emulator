#ifndef AUDIO_SAMPLE_H
#define AUDIO_SAMPLE_H

#include <memory>
#include <unordered_map>

#include "types.h"
#include "audio_channel.h"

struct audio_sample {
	audio_sample(uint32_t length) : length(length) {
		data = (int8_t *) PreferPSRAMAlloc(length);
		if (!data) {
			debug_log("audio_sample: failed to allocate %d bytes\n\r", length);
		}
	}
	~audio_sample();
	uint32_t		length = 0;			// Length of the sample in bytes
	int8_t *		data = nullptr;		// Pointer to the sample data
	std::unordered_map<uint8_t, std::weak_ptr<audio_channel>> channels;	// Channels playing this sample
};

audio_sample::~audio_sample() {
	// iterate over channels
	for (auto channelPair : this->channels) {
		auto channelRef = channelPair.second;
		if (!channelRef.expired()) {
			auto channel = channelRef.lock();
			debug_log("audio_sample: removing sample from channel %d\n\r", channel->channel());
			// Remove sample from channel
			channel->setWaveform(AUDIO_WAVE_DEFAULT, nullptr);
		}
	}

	if (this->data) {
		free(this->data);
	}

	debug_log("audio_sample cleared\n\r");
}

#endif // AUDIO_SAMPLE_H
