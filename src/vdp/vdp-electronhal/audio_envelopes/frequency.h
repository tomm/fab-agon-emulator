//
// Title:	        Audio Frequency Envelope support
// Author:        	Steve Sims
// Created:       	13/08/2023
// Last Updated:	13/08/2023

#ifndef ENVELOPE_FREQUENCY_H
#define ENVELOPE_FREQUENCY_H

#include <memory>
#include <vector>

class FrequencyEnvelope {
	public:
		virtual uint16_t getFrequency(uint16_t baseFrequency, uint32_t elapsed, int32_t duration);
		virtual bool isFinished(uint32_t elapsed, int32_t duration);
};

struct FrequencyStepPhase {
	int16_t adjustment;		// change of frequency per step
	uint16_t number;		// number of steps
};

class SteppedFrequencyEnvelope : public FrequencyEnvelope {
	public:
		SteppedFrequencyEnvelope(std::shared_ptr<std::vector<FrequencyStepPhase>> phases, uint16_t stepLength, bool repeats, bool cumulative, bool restrict);
		uint16_t getFrequency(uint16_t baseFrequency, uint32_t elapsed, int32_t duration);
		bool isFinished(uint32_t elapsed, int32_t duration);
	private:
		std::shared_ptr<std::vector<FrequencyStepPhase>> _phases;
		uint16_t _stepLength;
		uint32_t _totalSteps;
		uint32_t _totalAdjustment;
		uint32_t _totalLength;
		bool _repeats;
		bool _cumulative;
		bool _restrict;
};



#endif // ENVELOPE_FREQUENCY_H
