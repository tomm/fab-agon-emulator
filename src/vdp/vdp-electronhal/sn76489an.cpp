#include "sn76489an.h"
#include "fabgl.h"
#include "hal.h" 
#include "audio_driver.h"
#include "audio_channel.h"

#define PLAY_SOUND_PRIORITY 3

SN76489AN::SN76489AN ()
{
    register_select=-1;
    toneA=1;
    toneB=1;
    toneC=1;
    noise=0;
    amplA=0;
    amplB=0;
    amplC=0;
    amplNoise=0;
    first_byte = false;
}

void SN76489AN::init ()
{
    // tone
    setWaveform (0,AUDIO_WAVE_SQUARE);
    setWaveform (1,AUDIO_WAVE_SQUARE);
    setWaveform (2,AUDIO_WAVE_SQUARE);
    // noise
    setWaveform (3,AUDIO_WAVE_AY_3_8910_NOISE);
}

void SN76489AN::updateSound (uint8_t channel, uint8_t amp, uint16_t freq)
{
    // tone channel
    if (channel < 3)
    {
        // on
        if (freq>0)
        {
            // hal_printf ("%c tone freq: %d, vol: %d\r\n",channel+'A',SG1000_MASTER_FREQUENCY_DIV / freq,(15-amp) * FABGL_AMPLITUDE_MULTIPLIER);
            setFrequency (channel,SG1000_MASTER_FREQUENCY_DIV / freq);
        }
        // hal_printf ("%c frequency:%d Hz, %d attenuation\r\n",channel+'A',freq,amp);
        setVolume (channel,(15-amp) * FABGL_AMPLITUDE_MULTIPLIER);
    } 
    // noise channel
    else
    {
        if (freq&0b00000100)
            freq = FABGL_SOUNDGEN_DEFAULT_SAMPLE_RATE;
        else
        {
            switch (freq&0b00000011)
            {
                case 0b00: // N/512
                    freq = SG1000_MASTER_FREQUENCY/512;
                    break;
                case 0b01: // N/1024
                    freq = SG1000_MASTER_FREQUENCY/1024;
                    break;
                case 0b10: // N/2048
                    freq = SG1000_MASTER_FREQUENCY/2048;
                    break;
                case 0b11:
                    freq = toneC;
                    break;
            }
            
        }
        // on
        // hal_printf ("%c frequency:%d Hz, %d attenuation\r\n",channel+'A',freq,amp);
        setVolume (channel,(15-amp) * FABGL_AMPLITUDE_MULTIPLIER);
        if (freq>0)
        {
            // hal_printf ("%c tone freq: %d, vol: %d\r\n",channel+'A',SG1000_MASTER_FREQUENCY_DIV / freq,(15-amp) * FABGL_AMPLITUDE_MULTIPLIER);
            setFrequency (channel,SG1000_MASTER_FREQUENCY_DIV / freq);
        }
    } 
}
void SN76489AN::write (uint8_t value)
{
    bool updateA=false,updateB=false,updateC=false,updateNoise = false;

    if (value & 0b10000000)
    {
        register_select = value & 0b01110000;
        register_select = register_select >> 4;
        first_byte = true;
    }
    else
        first_byte = false;

    switch (register_select)
    {
        // Tone A
        case 0x00:
            if (first_byte)
            {
                toneA = toneA & 0b1111111111110000;
                toneA = toneA + (value & 0b00001111);
            }
            else
            {
                toneA = toneA & 0b1111110000001111;
                toneA = toneA + ((value & 0b00111111)<<4);
                updateA = true;
            }
            break;
        // Tone B - fine
        case 0x02:
            if (first_byte)
            {
                toneB = toneB & 0b1111111111110000;
                toneB = toneB + (value & 0b00001111);
            }
            else
            {
                toneB = toneB & 0b1111110000001111;
                toneB = toneB + ((value & 0b00111111)<<4);
                updateB = true;
            }
            break;
        // Tone C - fine
        case 0x04:
            if (first_byte)
            {
                toneC = toneC & 0b1111111111110000;
                toneC = toneC + (value & 0b00001111);
            }
            else
            {
                toneC = toneC & 0b1111110000001111;
                toneC = toneC + ((value & 0b00111111)<<4);
                updateC = true;
            }
            break;
        // Noise control
        case 0x06:
            if (first_byte)
            {
                fb = (value & 0b000000100)>>2;
                nf = value & 0b000000011;
                noise = value & 0b00000111;
            }
            break;
        // Amplitude, volume control - A
        case 0x01:
            if (first_byte)
            {
                amplA = value & 0b00001111;
                updateA = true;
            }
            break;
        // Amplitude, volume control - B
        case 0x03:
            if (first_byte)
            {
                amplB = value & 0b00001111;
                updateB = true;
            }
            break;
        // Amplitude, volume control - C
        case 0x05:
            if (first_byte)
            {
                amplC = value & 0b00001111;
                updateC = true;
            }
            break;
        // Amplitude, volume control - noise
        case 0x07:
            if (first_byte)
            {
                amplNoise = value & 0b00001111;
                updateNoise = true;
            }
            break;
        default:
            break;
    }

    // update sound
    if (updateA) 
    {
        updateSound (0,amplA,toneA);
        updateA = false;
    }
    if (updateB) 
    {
        updateSound (1,amplB,toneB);
        updateB = false;
    }
    if (updateC) 
    {
        updateSound (2,amplC,toneC);
        updateC = false;
    }
    if (updateNoise) 
    {
        updateSound (3,amplNoise,noise);
        updateC = false;
    }
}
