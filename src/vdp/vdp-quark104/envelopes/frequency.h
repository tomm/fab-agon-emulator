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
		virtual uint16_t getFrequency(uint16_t baseFrequency, uint32_t elapsed, int32_t duration) = 0;
		virtual bool isFinished(uint32_t elapsed, int32_t duration) = 0;
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

SteppedFrequencyEnvelope::SteppedFrequencyEnvelope(std::shared_ptr<std::vector<FrequencyStepPhase>> phases, uint16_t stepLength, bool repeats, bool cumulative, bool restrict)
	: _phases(phases), _stepLength(stepLength), _repeats(repeats), _cumulative(cumulative), _restrict(restrict)
{
	_totalSteps = 0;
	_totalLength = 0;
	_totalAdjustment = 0;

	for (auto phase : *this->_phases) {
		_totalSteps += phase.number;
		_totalLength += phase.number * _stepLength;
		_totalAdjustment += (phase.number * phase.adjustment);
	}

	debug_log("audio_driver: SteppedFrequencyEnvelope: totalSteps=%d, totalAdjustment=%d\n\r", this->_totalSteps, this->_totalAdjustment);
	debug_log("audio_driver: SteppedFrequencyEnvelope: stepLength=%d, repeats=%d, restricts=%d, totalLength=%d\n\r", this->_stepLength, this->_repeats, this->_restrict, _totalLength);
}

uint16_t SteppedFrequencyEnvelope::getFrequency(uint16_t baseFrequency, uint32_t elapsed, int32_t duration) {
	// returns frequency for the given elapsed time
	// a duration of -1 means we're playing forever
	auto currentStep = (elapsed / this->_stepLength) % this->_totalSteps;
	auto loopCount = elapsed / this->_totalLength;

	if (!_repeats && loopCount > 0) {
		// we're not repeating and we've finished the envelope
		return baseFrequency + this->_totalAdjustment;
	}

	// otherwise we need to calculate the frequency
	int32_t frequency = baseFrequency;

	if (_cumulative) {
		frequency += (loopCount * _totalAdjustment);
	}

	for (auto phase : *this->_phases) {
		if (currentStep < phase.number) {
			frequency += (currentStep * phase.adjustment);
			break;
		} else {
			frequency += (phase.number * phase.adjustment);
		}

		currentStep -= phase.number;
	}

	if (_restrict) {
		if (frequency < 0) {
			return 0;
		} else if (frequency > 65535) {
			return 0;
		}
	}

	return frequency;
}

bool SteppedFrequencyEnvelope::isFinished(uint32_t elapsed, int32_t duration) {
	if (_repeats) {
		// a repeating frequency envelope never finishes
		return false;
	}

	return elapsed >= _totalLength;
}

#endif // ENVELOPE_FREQUENCY_H
