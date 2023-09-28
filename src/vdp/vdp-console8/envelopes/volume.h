//
// Title:			Audio Volume Envelope support
// Author:			Steve Sims
// Created:			06/08/2023
// Last Updated:	13/08/2023

#ifndef ENVELOPE_VOLUME_H
#define ENVELOPE_VOLUME_H

class VolumeEnvelope {
	public:
		virtual byte getVolume(byte baseVolume, word elapsed, long duration) = 0;
		virtual bool isReleasing(word elapsed, long duration) = 0;
		virtual bool isFinished(word elapsed, long duration) = 0;
};

class ADSRVolumeEnvelope : public VolumeEnvelope {
	public:
		ADSRVolumeEnvelope(word attack, word decay, byte sustain, word release);
		byte getVolume(byte baseVolume, word elapsed, long duration);
		bool isReleasing(word elapsed, long duration);
		bool isFinished(word elapsed, long duration);
	private:
		word _attack;
		word _decay;
		byte _sustain;
		word _release;
};

ADSRVolumeEnvelope::ADSRVolumeEnvelope(word attack, word decay, byte sustain, word release)
	: _attack(attack), _decay(decay), _sustain(sustain), _release(release)
{
	// attack, decay, release are time values in milliseconds
	// sustain is 0-255, centered on 127, and is the relative sustain level
	debug_log("audio_driver: ADSRVolumeEnvelope: attack=%d, decay=%d, sustain=%d, release=%d\n\r", this->_attack, this->_decay, this->_sustain, this->_release);
}

byte ADSRVolumeEnvelope::getVolume(byte baseVolume, word elapsed, long duration) {
	// returns volume for the given elapsed time
	// baseVolume is the level the attack phase should reach
	// sustain volume level is calculated relative to baseVolume
	// volume for fab-gl is 0-127 but accepts higher values, so we're not clamping
	// a duration of -1 means we're playing forever
	long phaseTime = elapsed;
	if (phaseTime < this->_attack) {
		return (phaseTime * baseVolume) / this->_attack;
	}
	phaseTime -= this->_attack;
	byte sustainVolume = baseVolume * this->_sustain / 127;
	if (phaseTime < this->_decay) {
		return map(phaseTime, 0, this->_decay, baseVolume, sustainVolume);
	}
	phaseTime -= this->_decay;
	long sustainDuration = duration < 0 ? elapsed : duration - (this->_attack + this->_decay);
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

bool ADSRVolumeEnvelope::isReleasing(word elapsed, long duration) {
	if (duration < 0) return false;
	word minDuration = this->_attack + this->_decay;
	if (duration < minDuration) duration = minDuration;

	return (elapsed >= duration);
}

bool ADSRVolumeEnvelope::isFinished(word elapsed, long duration) {
	if (duration < 0) return false;
	word minDuration = this->_attack + this->_decay;
	if (duration < minDuration) duration = minDuration;

	return (elapsed >= duration + this->_release);
}

#endif // ENVELOPE_VOLUME_H
