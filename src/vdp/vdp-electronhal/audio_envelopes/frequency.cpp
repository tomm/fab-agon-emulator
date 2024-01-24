//
// Title:			Agon Video BIOS - Audio class
// Author:			Dean Belfield
// Contributors:	Steve Sims (enhancements for more sophisticated audio support)
//                  S0urceror (PlatformIO compatibility)
// Created:			05/09/2022
// Last Updated:	08/10/2023
//
// Modinfo:

#include "frequency.h"

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

	//debug_log("audio_driver: SteppedFrequencyEnvelope: totalSteps=%d, totalAdjustment=%d\n\r", this->_totalSteps, this->_totalAdjustment);
	//debug_log("audio_driver: SteppedFrequencyEnvelope: stepLength=%d, repeats=%d, restricts=%d, totalLength=%d\n\r", this->_stepLength, this->_repeats, this->_restrict, _totalLength);
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