#ifndef ENHANCED_SAMPLES_GENERATOR_H
#define ENHANCED_SAMPLES_GENERATOR_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <fabgl.h>

#include "audio_sample.h"

// Enhanced samples generator
//
class EnhancedSamplesGenerator : public WaveformGenerator {
	public:
		EnhancedSamplesGenerator(std::shared_ptr<audio_sample> sample);

		void setFrequency(int value);
		int getSample();
	
	private:
		std::weak_ptr<audio_sample> _sample;
		int _index;
};

#endif // ENHANCED_SAMPLES_GENERATOR_H
