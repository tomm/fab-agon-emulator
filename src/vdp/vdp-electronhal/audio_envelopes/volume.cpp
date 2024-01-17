//
// Title:			Agon Video BIOS - Audio class
// Author:			Dean Belfield
// Contributors:	Steve Sims (enhancements for more sophisticated audio support)
//                  S0urceror (PlatformIO compatibility)
// Created:			05/09/2022
// Last Updated:	08/10/2023
//
// Modinfo:

#include <fabgl.h>
#include "volume.h"

ADSRVolumeEnvelope::ADSRVolumeEnvelope(uint16_t attack, uint16_t decay, uint8_t sustain, uint16_t release)
	: _attack(attack), _decay(decay), _sustain(sustain), _release(release)
{
	// attack, decay, release are time values in milliseconds
	// sustain is 0-255, centered on 127, and is the relative sustain level
	//debug_log("audio_driver: ADSRVolumeEnvelope: attack=%d, decay=%d, sustain=%d, release=%d\n\r", this->_attack, this->_decay, this->_sustain, this->_release);
}

uint8_t ADSRVolumeEnvelope::getVolume(uint8_t baseVolume, uint32_t elapsed, int32_t duration) {
	// returns volume for the given elapsed time
	// baseVolume is the level the attack phase should reach
	// sustain volume level is calculated relative to baseVolume
	// volume for fab-gl is 0-127 but accepts higher values, so we're not clamping
	// a duration of -1 means we're playing forever
	auto phaseTime = elapsed;
	if (phaseTime < this->_attack) {
		return (phaseTime * baseVolume) / this->_attack;
	}
	phaseTime -= this->_attack;
	uint8_t sustainVolume = baseVolume * this->_sustain / 127;
	if (phaseTime < this->_decay) {
		return map(phaseTime, 0, this->_decay, baseVolume, sustainVolume);
	}
	phaseTime -= this->_decay;
	int32_t sustainDuration = duration < 0 ? elapsed : duration - (this->_attack + this->_decay);
	if (sustainDuration < 0) sustainDuration = 0;
	if (phaseTime < sustainDuration) {
		return sustainVolume;
	}
	phaseTime -= sustainDuration;
	if (phaseTime < this->_release) {
		return map(phaseTime, 0, this->_release, sustainVolume, 0);
	}
	return 0;
}

bool ADSRVolumeEnvelope::isReleasing(uint32_t elapsed, int32_t duration) {
	if (duration < 0) return false;
	auto minDuration = this->_attack + this->_decay;
	if (duration < minDuration) duration = minDuration;

	return (elapsed >= duration);
}

bool ADSRVolumeEnvelope::isFinished(uint32_t elapsed, int32_t duration) {
	if (duration < 0) return false;
	auto minDuration = this->_attack + this->_decay;
	if (duration < minDuration) duration = minDuration;

	return (elapsed >= duration + this->_release);
}