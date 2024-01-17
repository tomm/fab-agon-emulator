//
// Title:			Agon Video BIOS - Audio class
// Author:			Dean Belfield
// Contributors:	Steve Sims (enhancements for more sophisticated audio support)
//                  S0urceror (PlatformIO compatibility)
// Created:			05/09/2022
// Last Updated:	08/10/2023
//
// Modinfo:

#include "audio_channel.h"
#include "audio_sample.h"
#include "audio_enhanced_samples_generator.h"
#include "ay_3_8910_noise.h"

extern std::unordered_map<uint8_t, std::shared_ptr<audio_sample>> samples;	// Storage for the sample data
// negative values for waveforms indicate a sample number
extern fabgl::SoundGenerator		SoundGenerator;		// The audio class
extern void audioTaskAbortDelay(uint8_t channel);

audio_channel::audio_channel(uint8_t channel) {
	this->_channel = channel;
	this->_state = AUDIO_STATE_IDLE;
	this->_volume = 0;
	this->_frequency = 750;
	this->_duration = -1;
	setWaveform(AUDIO_WAVE_DEFAULT, nullptr);
	//debug_log("audio_driver: init %d\n\r", this->_channel);
	//debug_log("free mem: %d\n\r", heap_caps_get_free_size(MALLOC_CAP_8BIT));
}

audio_channel::~audio_channel() {
	//debug_log("audio_driver: deiniting %d\n\r", this->_channel);
	if (this->_waveform) {
		this->_waveform->enable(false);
		SoundGenerator.detach(this->_waveform.get());
	}
	//debug_log("audio_driver: deinit %d\n\r", this->_channel);
}

uint8_t audio_channel::play_note(uint8_t volume, uint16_t frequency, int32_t duration) {
	switch (this->_state) {
		case AUDIO_STATE_IDLE:
		case AUDIO_STATE_RELEASE:
			this->_volume = volume;
			this->_frequency = frequency;
			this->_duration = duration == 65535 ? -1 : duration;
			this->_state = AUDIO_STATE_PENDING;
			//debug_log("audio_driver: play_note %d,%d,%d,%d\n\r", this->_channel, this->_volume, this->_frequency, this->_duration);
			return 1;
	}
	return 0;
}

uint8_t audio_channel::getStatus() {
	uint8_t status = 0;
	if (this->_waveform && this->_waveform->enabled()) {
		status |= AUDIO_STATUS_ACTIVE;
		if (this->_duration == -1) {
			status |= AUDIO_STATUS_INDEFINITE;
		}
	}
	switch (this->_state) {
		case AUDIO_STATE_PENDING:
		case AUDIO_STATE_PLAYING:
		case AUDIO_STATE_PLAY_LOOP:
			status |= AUDIO_STATUS_PLAYING;
			break;
	}
	if (this->_volumeEnvelope) {
		status |= AUDIO_STATUS_HAS_VOLUME_ENVELOPE;
	}
	if (this->_frequencyEnvelope) {
		status |= AUDIO_STATUS_HAS_FREQUENCY_ENVELOPE;
	}

	//debug_log("audio_driver: getStatus %d\n\r", status);
	return status;
}

void audio_channel::setWaveform(int8_t waveformType, std::shared_ptr<audio_channel> channelRef) {
	std::unique_ptr<fabgl::WaveformGenerator> newWaveform = nullptr;

	switch (waveformType) {
		case AUDIO_WAVE_SAWTOOTH:
			newWaveform = make_unique_psram<SawtoothWaveformGenerator>();
			break;
		case AUDIO_WAVE_SQUARE:
			newWaveform = make_unique_psram<SquareWaveformGenerator>();
			break;
		case AUDIO_WAVE_SINE:
			newWaveform = make_unique_psram<SineWaveformGenerator>();
			break;
		case AUDIO_WAVE_TRIANGLE:
			newWaveform = make_unique_psram<TriangleWaveformGenerator>();
			break;
		case AUDIO_WAVE_NOISE:
			newWaveform = make_unique_psram<NoiseWaveformGenerator>();
			break;
		case AUDIO_WAVE_VICNOISE:
			newWaveform = make_unique_psram<VICNoiseGenerator>();
			break;
		case AUDIO_WAVE_AY_3_8910_NOISE:
			newWaveform = make_unique_psram<AY_3_8910_NoiseGenerator>();
			break;
		default:
			// negative values indicate a sample number
			if (waveformType < 0) {
				uint8_t sampleNum = -waveformType - 1;
				if (sampleNum < MAX_AUDIO_SAMPLES) {
					//debug_log("audio_driver: using sample %d for waveform (%d)\n\r", waveformType, sampleNum);
					if (samples.find(sampleNum) != samples.end()) {
						auto sample = samples.at(sampleNum);
						newWaveform = make_unique_psram<EnhancedSamplesGenerator>(sample);
						// remove this channel from other samples
						for (auto samplePair : samples) {
							if (samplePair.second) {
								samplePair.second->channels.erase(_channel);
							}
						}
						sample->channels[_channel] = channelRef;
					} else {
						//debug_log("audio_driver: sample %d not found\n\r", waveformType);
					}
				}
			} else {
				//debug_log("audio_driver: unknown waveform type %d\n\r", waveformType);
			}
			waveformType = AUDIO_WAVE_SAMPLE;
			break;
	}

	if (newWaveform != nullptr) {
		//debug_log("audio_driver: setWaveform %d\n\r", waveformType);
		if (this->_state != AUDIO_STATE_IDLE) {
			//debug_log("audio_driver: aborting current playback\n\r");
			// some kind of playback is happening, so abort any current task delay to allow playback to end
			this->_state = AUDIO_STATE_ABORT;
			audioTaskAbortDelay(this->_channel);
			waitForAbort();
		}
		if (this->_waveform) {
			//debug_log("audio_driver: detaching old waveform\n\r");
			SoundGenerator.detach(this->_waveform.get());
		}
		this->_waveform = std::move(newWaveform);
		_waveformType = waveformType;
		SoundGenerator.attach(this->_waveform.get());
		//debug_log("audio_driver: setWaveform %d done\n\r", waveformType);
	}
}

void audio_channel::setVolume(uint8_t volume) {
	//debug_log("audio_driver: setVolume %d\n\r", volume);

	if (this->_waveform) {
		waitForAbort();
		switch (this->_state) {
			case AUDIO_STATE_IDLE:
				if (volume > 0) {
					// new note playback
					this->_volume = volume;
					this->_duration = -1;	// indefinite duration
					this->_state = AUDIO_STATE_PENDING;
				}
				break;
			case AUDIO_STATE_PLAY_LOOP:
				// we are looping, so an envelope may be active
				if (volume == 0) {
					// silence whilst looping always stops playback - curtail duration
					this->_duration = millis() - this->_startTime;
					// if there's a volume envelope, just allow release to happen, otherwise...
					if (!this->_volumeEnvelope) {
						this->_volume = 0;
					}
				} else {
					// Change base volume level, so next loop iteration will use it
					this->_volume = volume;
				}
				break;
			case AUDIO_STATE_PENDING:
			case AUDIO_STATE_RELEASE:
				// Set level so next loop will pick up the new volume
				this->_volume = volume;
				break;
			default:
				// All other states we'll set volume immediately
				this->_volume = volume;
				this->_waveform->setVolume(volume);
				if (volume == 0) {
					// we're going silent, so abort any current playback
					this->_state = AUDIO_STATE_ABORT;
					audioTaskAbortDelay(this->_channel);
				}
				break;
		}	
	}
}

void audio_channel::setFrequency(uint16_t frequency) {
	//debug_log("audio_driver: setFrequency %d\n\r", frequency);
	this->_frequency = frequency;

	if (this->_waveform) {
		waitForAbort();
		switch (this->_state) {
			case AUDIO_STATE_PENDING:
			case AUDIO_STATE_PLAY_LOOP:
			case AUDIO_STATE_RELEASE:
				// Do nothing as next loop will pick up the new frequency
				break;
			default:
				this->_waveform->setFrequency(frequency);
		}
	}
}

void audio_channel::setVolumeEnvelope(std::unique_ptr<VolumeEnvelope> envelope) {
	this->_volumeEnvelope = std::move(envelope);
	if (envelope && this->_state == AUDIO_STATE_PLAYING) {
		// swap to looping
		this->_state = AUDIO_STATE_PLAY_LOOP;
		audioTaskAbortDelay(this->_channel);
	}
}

void audio_channel::setFrequencyEnvelope(std::unique_ptr<FrequencyEnvelope> envelope) {
	this->_frequencyEnvelope = std::move(envelope);
	if (envelope && this->_state == AUDIO_STATE_PLAYING) {
		// swap to looping
		this->_state = AUDIO_STATE_PLAY_LOOP;
		audioTaskAbortDelay(this->_channel);
	}
}

void audio_channel::waitForAbort() {
	while (this->_state == AUDIO_STATE_ABORT) {
		// wait for abort to complete
		vTaskDelay(1);
	}
}

uint8_t audio_channel::getVolume(uint32_t elapsed) {
	if (this->_volumeEnvelope) {
		return this->_volumeEnvelope->getVolume(this->_volume, elapsed, this->_duration);
	}
	return this->_volume;
}

uint16_t audio_channel::getFrequency(uint32_t elapsed) {
	if (this->_frequencyEnvelope) {
		return this->_frequencyEnvelope->getFrequency(this->_frequency, elapsed, this->_duration);
	}
	return this->_frequency;
}

bool audio_channel::isReleasing(uint32_t elapsed) {
	if (this->_volumeEnvelope) {
		return this->_volumeEnvelope->isReleasing(elapsed, this->_duration);
	}
	if (this->_duration == -1) {
		return false;
	}
	return elapsed >= this->_duration;
}

bool audio_channel::isFinished(uint32_t elapsed) {
	if (this->_volumeEnvelope) {
		return this->_volumeEnvelope->isFinished(elapsed, this->_duration);
	}
	if (this->_duration == -1) {
		return false;
	}
	return (elapsed >= this->_duration);
}

void audio_channel::loop() {
	switch (this->_state) {
		case AUDIO_STATE_PENDING:
			//debug_log("audio_driver: play %d,%d,%d,%d\n\r", this->_channel, this->_volume, this->_frequency, this->_duration);
			// we have a new note to play
			this->_startTime = millis();
			// set our initial volume and frequency
			this->_waveform->setVolume(this->getVolume(0));
			if (this->_waveformType == AUDIO_WAVE_SAMPLE) {
				// hack to ensure samples always start from beginning
				this->_waveform->setFrequency(-1);
			} else {
				this->_waveform->setFrequency(this->getFrequency(0));
			}
			this->_waveform->enable(true);
			// if we have an envelope then we loop, otherwise just delay for duration			
			if (this->_volumeEnvelope || this->_frequencyEnvelope) {
				this->_state = AUDIO_STATE_PLAY_LOOP;
			} else {
				this->_state = AUDIO_STATE_PLAYING;
				// if delay value is negative then this delays for a super long time
				vTaskDelay(pdMS_TO_TICKS(this->_duration));
			}
			break;
		case AUDIO_STATE_PLAYING:
			if (this->_duration >= 0) {
				// simple playback - delay until we have reached our duration
				uint32_t elapsed = millis() - this->_startTime;
				//debug_log("audio_driver: elapsed %d\n\r", elapsed);
				if (elapsed >= this->_duration) {
					this->_waveform->enable(false);
					//debug_log("audio_driver: end\n\r");
					this->_state = AUDIO_STATE_IDLE;
				} else {
					//debug_log("audio_driver: loop (%d)\n\r", this->_duration - elapsed);
					vTaskDelay(pdMS_TO_TICKS(this->_duration - elapsed));
				}
			} else {
				// our duration is indefinite, so delay for a long time
				//debug_log("audio_driver: loop (indefinite playback)\n\r");
				vTaskDelay(pdMS_TO_TICKS(-1));
			}
			break;
		// loop and release states used for envelopes
		case AUDIO_STATE_PLAY_LOOP: {
			uint32_t elapsed = millis() - this->_startTime;
			if (isReleasing(elapsed)) {
				//debug_log("audio_driver: releasing...\n\r");
				this->_state = AUDIO_STATE_RELEASE;
			}
			// update volume and frequency as appropriate
			if (this->_volumeEnvelope)
				this->_waveform->setVolume(this->getVolume(elapsed));
			if (this->_frequencyEnvelope)
				this->_waveform->setFrequency(this->getFrequency(elapsed));
			break;
		}
		case AUDIO_STATE_RELEASE: {
			uint32_t elapsed = millis() - this->_startTime;
			// update volume and frequency as appropriate
			if (this->_volumeEnvelope)
				this->_waveform->setVolume(this->getVolume(elapsed));
			if (this->_frequencyEnvelope)
				this->_waveform->setFrequency(this->getFrequency(elapsed));

			if (isFinished(elapsed)) {
				this->_waveform->enable(false);
				//debug_log("audio_driver: end (released)\n\r");
				this->_state = AUDIO_STATE_IDLE;
			}
			break;
		}
		case AUDIO_STATE_ABORT:
			this->_waveform->enable(false);
			//debug_log("audio_driver: abort\n\r");
			this->_state = AUDIO_STATE_IDLE;
			break;
	}
}