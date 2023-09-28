//
// Title:			Agon Video BIOS - Audio class
// Author:			Dean Belfield
// Contributors:	Steve Sims (enhancements for more sophisticated audio support)
// Created:			05/09/2022
// Last Updated:	04/08/2023
//
// Modinfo:

#ifndef AGON_AUDIO_H
#define AGON_AUDIO_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <fabgl.h>

#include "audio_channel.h"
#include "audio_sample.h"
#include "types.h"

// audio channels and their associated tasks
std::unordered_map<uint8_t, std::shared_ptr<audio_channel>> audio_channels;
std::vector<TaskHandle_t, psram_allocator<TaskHandle_t>> audioHandlers;

std::unordered_map<uint8_t, std::shared_ptr<audio_sample>> samples;	// Storage for the sample data

fabgl::SoundGenerator		SoundGenerator;		// The audio class

// Audio channel driver task
//
void audio_driver(void * parameters) {
	uint8_t channel = *(uint8_t *)parameters;

	audio_channels[channel] = make_shared_psram<audio_channel>(channel);
	while (true) {
		audio_channels[channel]->loop();
		vTaskDelay(1);
	}
}

void init_audio_channel(uint8_t channel) {
	xTaskCreatePinnedToCore(audio_driver,  "audio_driver",
		4096,						// This stack size can be checked & adjusted by reading the Stack Highwater
		&channel,					// Parameters
		PLAY_SOUND_PRIORITY,		// Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
		&audioHandlers[channel],	// Task handle
		ARDUINO_RUNNING_CORE
	);
}

void audioTaskAbortDelay(uint8_t channel) {
	if (audioHandlers[channel]) {
		xTaskAbortDelay(audioHandlers[channel]);
	}
}

void audioTaskKill(uint8_t channel) {
	if (audioHandlers[channel]) {
		vTaskDelete(audioHandlers[channel]);
		audioHandlers[channel] = nullptr;
		audio_channels.erase(channel);
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
	for (uint8_t i = 0; i < AUDIO_CHANNELS; i++) {
		init_audio_channel(i);
	}
	SoundGenerator.play(true);
}

// Channel enabled?
//
bool channelEnabled(uint8_t channel) {
	return channel < MAX_AUDIO_CHANNELS && audio_channels[channel];
}

// Play a note
//
uint8_t play_note(uint8_t channel, uint8_t volume, uint16_t frequency, uint16_t duration) {
	if (channelEnabled(channel)) {
		return audio_channels[channel]->play_note(volume, frequency, duration);
	}
	return 1;
}

// Get channel status
//
uint8_t getChannelStatus(uint8_t channel) {
	if (channelEnabled(channel)) {
		return audio_channels[channel]->getStatus();
	}
	return -1;
}

// Set channel volume
//
void setVolume(uint8_t channel, uint8_t volume) {
	if (channelEnabled(channel)) {
		audio_channels[channel]->setVolume(volume);
	}
}

// Set channel frequency
//
void setFrequency(uint8_t channel, uint16_t frequency) {
	if (channelEnabled(channel)) {
		audio_channels[channel]->setFrequency(frequency);
	}
}

// Set channel waveform
//
void setWaveform(uint8_t channel, int8_t waveformType) {
	if (channelEnabled(channel)) {
		auto channelRef = audio_channels[channel];
		channelRef->setWaveform(waveformType, channelRef);
	}
}

// Clear a sample
//
uint8_t clearSample(uint8_t sampleIndex) {
	debug_log("clearSample: sample %d\n\r", sampleIndex);
	if (sampleIndex < MAX_AUDIO_SAMPLES) {
		if (samples.find(sampleIndex) == samples.end()) {
			debug_log("clearSample: sample %d not found\n\r", sampleIndex);
			return 0;
		}
		samples.erase(sampleIndex);
		debug_log("reset sample\n\r");
		return 1;
	}
	return 0;
}

#endif // AGON_AUDIO_H
