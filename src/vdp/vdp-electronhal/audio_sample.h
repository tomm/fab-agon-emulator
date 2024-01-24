#ifndef AUDIO_SAMPLE_H
#define AUDIO_SAMPLE_H

#include <memory>
#include <unordered_map>

#include "audio_channel.h"

class audio_sample {
	public:
	audio_sample(uint32_t length);
	~audio_sample();
	
	std::unordered_map<uint8_t, std::weak_ptr<audio_channel>> channels;	// Channels playing this sample
	int8_t *		data = nullptr;		// Pointer to the sample data
	uint32_t		length = 0;			// Length of the sample in bytes
};

#endif // AUDIO_SAMPLE_H
