//
// Title:	        Agon Video BIOS - Audio class
// Author:        	Dean Belfield
// Created:       	05/09/2022
// Last Updated:	05/09/2022
//
// Modinfo:

#pragma once

// The audio channel class
//
class audio_channel {	
	public:
		audio_channel(int channel);		
		word	play_note(byte volume, word frequency, word duration);
		void	loop();
	private:
		fabgl::WaveformGenerator *	_waveform;			
	 	byte _flag;
		byte _channel;
		byte _volume;
		word _frequency;
		word _duration;
};

audio_channel::audio_channel(int channel) {
	this->_channel = channel;
	this->_flag = 0;
	this->_waveform = new SawtoothWaveformGenerator();
	SoundGenerator.attach(_waveform);
	SoundGenerator.play(true);
	debug_log("audio_driver: init %d\n\r", this->_channel);			
}

word audio_channel::play_note(byte volume, word frequency, word duration) {
	if(this->_flag == 0) {
		this->_volume = volume;
		this->_frequency = frequency;
		this->_duration = duration;
		this->_flag++;
		debug_log("audio_driver: play_note %d,%d,%d,%d\n\r", this->_channel, this->_volume, this->_frequency, this->_duration);		
		return 1;	
	}
	return 0;
}

void audio_channel::loop() {
	if(this->_flag > 0) {
		debug_log("audio_driver: play %d,%d,%d,%d\n\r", this->_channel, this->_volume, this->_frequency, this->_duration);			
		this->_waveform->setVolume(this->_volume);
		this->_waveform->setFrequency(this->_frequency);
		this->_waveform->enable(true);
		vTaskDelay(this->_duration);
		this->_waveform->enable(false);
		debug_log("audio_driver: end\n\r");			
		this->_flag = 0; 
	}
}