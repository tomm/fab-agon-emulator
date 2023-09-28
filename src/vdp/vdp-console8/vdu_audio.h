//
// Title:			Audio VDU command support
// Author:			Steve Sims
// Created:			29/07/2023
// Last Updated:	29/07/2023

#ifndef VDU_AUDIO_H
#define VDU_AUDIO_H

#include <memory>
#include <vector>
#include <array>
#include <fabgl.h>

fabgl::SoundGenerator		SoundGenerator;		// The audio class

#include "agon.h"
#include "agon_audio.h"
#include "vdp_protocol.h"

// audio channels and their associated tasks
std::array<std::shared_ptr<audio_channel>, MAX_AUDIO_CHANNELS> audio_channels;
std::vector<TaskHandle_t> audioHandlers;

std::array<std::shared_ptr<audio_sample>, MAX_AUDIO_SAMPLES> samples;	// Storage for the sample data

// Audio channel driver task
//
void audio_driver(void * parameters) {
	int channel = *(int *)parameters;

	audio_channels.at(channel) = std::make_shared<audio_channel>(channel);
	task_loop {
		audio_channels[channel]->loop();
		vTaskDelay(1);
	}
}

void init_audio_channel(int channel) {
	xTaskCreatePinnedToCore(audio_driver,  "audio_driver",
		4096,						// This stack size can be checked & adjusted by reading the Stack Highwater
		&channel,					// Parameters
		PLAY_SOUND_PRIORITY,		// Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
		&audioHandlers[channel],	// Task handle
		ARDUINO_RUNNING_CORE
	);
}

void audioTaskAbortDelay(int channel) {
	if (audioHandlers[channel]) {
		xTaskAbortDelay(audioHandlers[channel]);
	}
}

void audioTaskKill(int channel) {
	if (audioHandlers[channel]) {
		vTaskDelete(audioHandlers[channel]);
		audioHandlers[channel] = nullptr;
		audio_channels[channel] = nullptr;
		debug_log("audioTaskKill: channel %d killed\n\r", channel);
	} else {
		debug_log("audioTaskKill: channel %d not found\n\r", channel);
	}
}

// Initialise the sound driver
//
void init_audio() {
	audioHandlers.reserve(MAX_AUDIO_CHANNELS);
	debug_log("init_audio: we have reserved %d channels\n\r", audioHandlers.capacity());
	for (int i = 0; i < AUDIO_CHANNELS; i++) {
		init_audio_channel(i);
	}
	SoundGenerator.play(true);
}

// Send an audio acknowledgement
//
void sendAudioStatus(int channel, int status) {
	byte packet[] = {
		channel,
		status,
	};
	send_packet(PACKET_AUDIO, sizeof packet, packet);
}

// Channel enabled?
//
bool channelEnabled(byte channel) {
	return channel >= 0 && channel < MAX_AUDIO_CHANNELS && audio_channels[channel];
}

// Play a note
//
word play_note(byte channel, byte volume, word frequency, word duration) {
	if (channelEnabled(channel)) {
		return audio_channels[channel]->play_note(volume, frequency, duration);
	}
	return 1;
}

// Get channel status
//
byte getChannelStatus(byte channel) {
	if (channelEnabled(channel)) {
		return audio_channels[channel]->getStatus();
	}
	return -1;
}

// Set channel volume
//
void setVolume(byte channel, byte volume) {
	if (channelEnabled(channel)) {
		audio_channels[channel]->setVolume(volume);
	}
}

// Set channel frequency
//
void setFrequency(byte channel, word frequency) {
	if (channelEnabled(channel)) {
		audio_channels[channel]->setFrequency(frequency);
	}
}

// Set channel waveform
//
void setWaveform(byte channel, int8_t waveformType) {
	if (channelEnabled(channel)) {
		auto channelRef = audio_channels[channel];
		channelRef->setWaveform(waveformType, channelRef);
	}
}

// TODO move sample management into agon_audio.h

// Clear a sample
//
byte clearSample(byte sampleIndex) {
	debug_log("clearSample: sample %d\n\r", sampleIndex);
	if (sampleIndex >= 0 && sampleIndex < MAX_AUDIO_SAMPLES) {
		if (!samples[sampleIndex]) {
			debug_log("clearSample: sample %d not found\n\r", sampleIndex);
			return 0;
		}
		samples[sampleIndex].reset();
		debug_log("reset sample\n\r");
		return 1;
	}
	return 0;
}

// Load a sample
//
byte loadSample(byte sampleIndex, int length) {
	debug_log("free mem: %d\n\r", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
	if (sampleIndex >= 0 && sampleIndex < MAX_AUDIO_SAMPLES) {
		debug_log("loadSample: sample %d - length %d\n\r", sampleIndex, length);
		// Clear out existing sample
		clearSample(sampleIndex);

		auto sample = new audio_sample();

		int8_t * data = (int8_t *) heap_caps_malloc(length, MALLOC_CAP_SPIRAM);
		sample->data = data;

		if (data) {
			// read data into buffer
			for (int n = 0; n < length; n++) sample->data[n] = readByte_b();
			sample->length = length;

			samples[sampleIndex].reset(sample);
			debug_log("vdu_sys_audio: sample %d - data loaded, length %d\n\r", sampleIndex, sample->length);
			return 1;
		} else {
			// Failed to allocate memory
			// discard incoming serial data
			discardBytes(length);
			debug_log("vdu_sys_audio: sample %d - data discarded, no memory available length %d\n\r", sampleIndex, length);
			return 0;
		}
	}
	// Invalid sample number - discard incoming serial data
	discardBytes(length);
	debug_log("vdu_sys_audio: sample %d - data discarded, invalid sample number\n\r", sampleIndex);
	return 0;
}

// Set channel volume envelope
//
void setVolumeEnvelope(byte channel, byte type) {
	if (channelEnabled(channel)) {
		switch (type) {
			case AUDIO_ENVELOPE_NONE:
				audio_channels[channel]->setVolumeEnvelope(nullptr);
				debug_log("vdu_sys_audio: channel %d - volume envelope disabled\n\r", channel);
				break;
			case AUDIO_ENVELOPE_ADSR:
				int attack = readWord_t();		if (attack == -1) return;
				int decay = readWord_t();		if (decay == -1) return;
				int sustain = readByte_t();		if (sustain == -1) return;
				int release = readWord_t();		if (release == -1) return;
				auto envelope = std::make_shared<ADSRVolumeEnvelope>(attack, decay, sustain, release);
				audio_channels[channel]->setVolumeEnvelope(envelope);
				break;
		}
	}
}

// Set channel frequency envelope
//
void setFrequencyEnvelope(byte channel, byte type) {
	if (channelEnabled(channel)) {
		switch (type) {
			case AUDIO_ENVELOPE_NONE:
				audio_channels[channel]->setFrequencyEnvelope(nullptr);
				debug_log("vdu_sys_audio: channel %d - frequency envelope disabled\n\r", channel);
				break;
			case AUDIO_FREQUENCY_ENVELOPE_STEPPED:
				int phaseCount = readByte_t();	if (phaseCount == -1) return;
				int control = readByte_t();		if (control == -1) return;
				int stepLength = readWord_t();	if (stepLength == -1) return;
				auto phases = std::make_shared<std::vector<FrequencyStepPhase>>();
				for (int n = 0; n < phaseCount; n++) {
					int adjustment = readWord_t();	if (adjustment == -1) return;
					int number = readWord_t();		if (number == -1) return;
					phases->push_back(FrequencyStepPhase { (int16_t)adjustment, (uint16_t)number });
				}
				bool repeats = control & AUDIO_FREQUENCY_REPEATS;
				bool cumulative = control & AUDIO_FREQUENCY_CUMULATIVE;
				bool restrict = control & AUDIO_FREQUENCY_RESTRICT;
				auto envelope = std::make_shared<SteppedFrequencyEnvelope>(phases, stepLength, repeats, cumulative, restrict);
				audio_channels[channel]->setFrequencyEnvelope(envelope);
				break;
		}
	}
}

// Audio VDU command support (VDU 23, 0, &85, <args>)
//
void vdu_sys_audio() {
	int channel = readByte_t();		if (channel == -1) return;
	int command = readByte_t();		if (command == -1) return;

	switch (command) {
		case AUDIO_CMD_PLAY: {
			int volume = readByte_t();		if (volume == -1) return;
			int frequency = readWord_t();	if (frequency == -1) return;
			int duration = readWord_t();	if (duration == -1) return;

			sendAudioStatus(channel, play_note(channel, volume, frequency, duration));
		}	break;

		case AUDIO_CMD_STATUS: {
			sendAudioStatus(channel, getChannelStatus(channel));
		}	break;

		case AUDIO_CMD_VOLUME: {
			int volume = readByte_t();		if (volume == -1) return;

			setVolume(channel, volume);
		}	break;

		case AUDIO_CMD_FREQUENCY: {
			int frequency = readWord_t();	if (frequency == -1) return;

			setFrequency(channel, frequency);
		}	break;

		case AUDIO_CMD_WAVEFORM: {
			int waveform = readByte_t();	if (waveform == -1) return;

			// set waveform, interpretting waveform number as a signed 8-bit value
			// to allow for negative values to be used as sample numbers
			setWaveform(channel, (int8_t) waveform);
		}	break;

		case AUDIO_CMD_SAMPLE: {
			// sample number is negative 8 bit number, and provided in channel number param
			int8_t sampleNum = -(int8_t)channel - 1;	// convert to positive, ranged from zero
			int action = readByte_t();		if (action == -1) return;

			switch (action) {
				case AUDIO_SAMPLE_LOAD: {
					// length as a 24-bit value
					int length = read24_t();		if (length == -1) return;

					sendAudioStatus(channel, loadSample(sampleNum, length));
				}	break;

				case AUDIO_SAMPLE_CLEAR: {
					debug_log("vdu_sys_audio: clear sample %d\n\r", channel);
					sendAudioStatus(channel, clearSample(sampleNum));
				}	break;

				case AUDIO_SAMPLE_DEBUG_INFO: {
					debug_log("Sample info: %d (%d)\n\r", (int8_t)channel, sampleNum);
					debug_log("  samples count: %d\n\r", samples.size());
					debug_log("  free mem: %d\n\r", heap_caps_get_free_size(MALLOC_CAP_8BIT));
					audio_sample* sample = samples[sampleNum].get();
					if (sample == nullptr) {
						debug_log("  sample is null\n\r");
						break;
					}
					debug_log("  length: %d\n\r", sample->length);
					if (sample->length > 0) {
						debug_log("  data first byte: %d\n\r", sample->data[0]);
					}
				} break;

				default: {
					debug_log("vdu_sys_audio: unknown sample action %d\n\r", action);
					sendAudioStatus(channel, 0);
				}
			}
		}	break;

		case AUDIO_CMD_ENV_VOLUME: {
			int type = readByte_t();		if (type == -1) return;

			setVolumeEnvelope(channel, type);
		}	break;

		case AUDIO_CMD_ENV_FREQUENCY: {
			int type = readByte_t();		if (type == -1) return;

			setFrequencyEnvelope(channel, type);
		}	break;

		case AUDIO_CMD_ENABLE: {
			if (channel >= 0 && channel < MAX_AUDIO_CHANNELS && audio_channels[channel] == nullptr) {
				init_audio_channel(channel);
			}
		}	break;

		case AUDIO_CMD_DISABLE: {
			if (channelEnabled(channel)) {
				audioTaskKill(channel);
			}
		}	break;

		case AUDIO_CMD_RESET: {
			if (channelEnabled(channel)) {
				audioTaskKill(channel);
				init_audio_channel(channel);
			}
		}	break;
	}
}

#endif // VDU_AUDIO_H
