//
// Title:			Audio Volume Envelope support
// Author:			Steve Sims
// Created:			06/08/2023
// Last Updated:	13/08/2023

#ifndef ENVELOPE_VOLUME_H
#define ENVELOPE_VOLUME_H

class VolumeEnvelope {
	public:
		virtual uint8_t getVolume(uint8_t baseVolume, uint32_t elapsed, int32_t duration);
		virtual bool isReleasing(uint32_t elapsed, int32_t duration);
		virtual bool isFinished(uint32_t elapsed, int32_t duration);
};

class ADSRVolumeEnvelope : public VolumeEnvelope {
	public:
		ADSRVolumeEnvelope(uint16_t attack, uint16_t decay, uint8_t sustain, uint16_t release);
		uint8_t getVolume(uint8_t baseVolume, uint32_t elapsed, int32_t duration);
		bool isReleasing(uint32_t elapsed, int32_t duration);
		bool isFinished(uint32_t elapsed, int32_t duration);
	private:
		uint16_t _attack;
		uint16_t _decay;
		uint8_t _sustain;
		uint16_t _release;
};

#endif // ENVELOPE_VOLUME_H
