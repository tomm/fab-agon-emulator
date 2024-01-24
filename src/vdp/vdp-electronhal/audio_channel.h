#ifndef AUDIO_CHANNEL_H
#define AUDIO_CHANNEL_H

#include <memory>
#include <unordered_map>
#include <fabgl.h>

#include "audio_envelopes/volume.h"
#include "audio_envelopes/frequency.h"
#include "types.h"

#define AUDIO_STATE_IDLE		0		// Channel is idle/silent
#define AUDIO_STATE_PENDING		1		// Channel is pending (note will be played next loop call)
#define AUDIO_STATE_PLAYING		2		// Channel is playing a note (passive)
#define AUDIO_STATE_PLAY_LOOP	3		// Channel is in active note playing loop
#define AUDIO_STATE_RELEASE		4		// Channel is releasing a note
#define AUDIO_STATE_ABORT		5		// Channel is aborting a note

#define AUDIO_WAVE_DEFAULT		0		// Default waveform (Square wave)
#define AUDIO_WAVE_SQUARE		0		// Square wave
#define AUDIO_WAVE_TRIANGLE		1		// Triangle wave
#define AUDIO_WAVE_SAWTOOTH		2		// Sawtooth wave
#define AUDIO_WAVE_SINE			3		// Sine wave
#define AUDIO_WAVE_NOISE		4		// Noise (simple, no frequency support)
#define AUDIO_WAVE_VICNOISE		5		// VIC-style noise (supports frequency)
#define AUDIO_WAVE_AY_3_8910_NOISE		6		// MSX-style noise (support frequency)
#define AUDIO_WAVE_SAMPLE		8		// Sample playback (internally used, can't be passed as a parameter)

#define AUDIO_STATUS_ACTIVE		0x01	// Has an active waveform
#define AUDIO_STATUS_PLAYING	0x02	// Playing a note (not in release phase)
#define AUDIO_STATUS_INDEFINITE	0x04	// Indefinite duration sound playing
#define AUDIO_STATUS_HAS_VOLUME_ENVELOPE	0x08	// Channel has a volume envelope set
#define AUDIO_STATUS_HAS_FREQUENCY_ENVELOPE	0x10	// Channel has a frequency envelope set

#define MAX_AUDIO_SAMPLES		128		// Maximum number of audio samples

// The audio channel class
//
class audio_channel {	
	public:
		audio_channel(uint8_t channel);
		~audio_channel();
		uint8_t		play_note(uint8_t volume, uint16_t frequency, int32_t duration);
		uint8_t		getStatus();
		void		setWaveform(int8_t waveformType, std::shared_ptr<audio_channel> channelRef);
		void		setVolume(uint8_t volume);
		void		setFrequency(uint16_t frequency);
		void		setVolumeEnvelope(std::unique_ptr<VolumeEnvelope> envelope);
		void		setFrequencyEnvelope(std::unique_ptr<FrequencyEnvelope> envelope);
		void		loop();
		uint8_t		channel() { return _channel; }
	private:
		void		waitForAbort();
		uint8_t		getVolume(uint32_t elapsed);
		uint16_t	getFrequency(uint32_t elapsed);
		bool		isReleasing(uint32_t elapsed);
		bool		isFinished(uint32_t elapsed);
		uint8_t		_state;
		uint8_t		_channel;
		uint8_t		_volume;
		uint16_t	_frequency;
		int32_t		_duration;
		uint32_t	_startTime;
		uint8_t		_waveformType;
		std::unique_ptr<WaveformGenerator>	_waveform;
		std::unique_ptr<VolumeEnvelope>		_volumeEnvelope;
		std::unique_ptr<FrequencyEnvelope>	_frequencyEnvelope;
};

#endif // AUDIO_CHANNEL_H
