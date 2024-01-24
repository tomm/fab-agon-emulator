#include "ay_3_8910.h"
#include "fabgl.h"
#include "hal.h" 
#include "audio_driver.h"
#include "audio_channel.h"

#define PLAY_SOUND_PRIORITY 3

AY_3_8910::AY_3_8910 ()
{
    register_select=0;
    toneA=0;
    toneB=0;
    toneC=0;
    noise=0;
    mixer=0;
    env_period=0;
    env_shape=0;
    amplA=0;
    amplB=0;
    amplC=0;
}

void AY_3_8910::init ()
{
    // tone
    setWaveform (0,AUDIO_WAVE_SQUARE);
    setWaveform (1,AUDIO_WAVE_SQUARE);
    setWaveform (2,AUDIO_WAVE_SQUARE);
    // noise
    setWaveform (3,AUDIO_WAVE_AY_3_8910_NOISE);
    setWaveform (4,AUDIO_WAVE_AY_3_8910_NOISE);
    setWaveform (5,AUDIO_WAVE_AY_3_8910_NOISE);

    // setWaveform (3,AUDIO_WAVE_SINE);
    // setWaveform (4,AUDIO_WAVE_SINE);
    // setWaveform (5,AUDIO_WAVE_SINE);
}

void AY_3_8910::updateSound (uint8_t channel, uint8_t mixer, uint8_t amp, uint32_t tone_freq, uint32_t noise_freq)
{
    if (amp & 0x10 > 0)
    {
        // volume envelope switched on
    }
    // tone channel
    if ((mixer & (0b00000001<<channel))==0 && tone_freq > 0)
    {
        // on
        setFrequency (channel,MASTER_FREQUENCY_DIV / tone_freq);
        setVolume (channel,(amp & 0x0f) * FABGL_AMPLITUDE_MULTIPLIER);
        // hal_hostpc_printf ("%d / %d = %d\r\n",MASTER_FREQUENCY_DIV,tone_freq,MASTER_FREQUENCY_DIV / tone_freq);
        // hal_hostpc_printf ("%c tone freq: %d, vol: %d\r\n",channel+'A',MASTER_FREQUENCY_DIV / tone_freq,(amp & 0x0f) * FABGL_AMPLITUDE_MULTIPLIER);
    } 
    else
    {
        // off
        setVolume (channel,0);
        //hal_hostpc_printf ("%c tone off\r\n",channel+'A');
    }
    // noise channel + 3
    if ((mixer & (0b00001000<<channel))==0 && noise_freq > 0)
    {
        // on
        setFrequency (channel+3,MASTER_FREQUENCY_DIV / noise_freq);
        setVolume (channel+3,(amp & 0x0f) * FABGL_AMPLITUDE_MULTIPLIER);
        // hal_hostpc_printf ("%d / %d = %d\r\n",MASTER_FREQUENCY_DIV,noise_freq,MASTER_FREQUENCY_DIV / noise_freq);
        // hal_hostpc_printf ("%c noise freq: %d, vol: %d\r\n",channel+'A',MASTER_FREQUENCY_DIV / noise_freq,(amp & 0x0f) * FABGL_AMPLITUDE_MULTIPLIER);
    } 
    else
    {
        // off
        setVolume (channel+3,0);
        //hal_hostpc_printf ("%c noise off\r\n",channel+'A');
    }
}
void AY_3_8910::write (uint8_t port, uint8_t value)
{
    bool updateA=false,updateB=false,updateC = false;
    uint8_t mixer_diff;

    if (port==0xa0)
        register_select = value;
    if (port==0xa1)
    {
        switch (register_select)
        {
            // Tone A - fine
            case 0x00:
                toneA = toneA & 0b111100000000;
                toneA = toneA + value;
                updateA = true;
                break;
            // Tone A - coarse
            case 0x01:
                toneA = toneA & 0b000011111111;
                toneA = toneA + (value << 8);
                updateA = true;
                break;
            // Tone B - fine
            case 0x02:
                toneB = toneB & 0b111100000000;
                toneB = toneB + value;
                updateB = true;
                break;
            // Tone B - coarse
            case 0x03:
                toneB = toneB & 0b000011111111;
                toneB = toneB + (value << 8);
                updateB = true;
                break;
            // Tone C - fine
            case 0x04:
                toneC = toneC & 0b111100000000;
                toneC = toneC + value;
                updateC = true;
                break;
            // Tone C - coarse
            case 0x05:
                toneC = toneC & 0b000011111111;
                toneC = toneC + (value << 8);
                updateC = true;
                break;
            // Noise generator
            case 0x06:
                noise = value & 0b00011111;
                // noise frequency change applies to all channels
                updateA = true;
                updateB = true;
                updateC = true;
                break;
            // Voice, I/O port control
            case 0x07:
                mixer_diff = mixer ^ value;
                mixer = value;
                if (mixer_diff&0b00001001!=0)
                    updateA = true;
                if (mixer_diff&0b00010010!=0)
                    updateB = true;
                if (mixer_diff&0b00100100!=0)
                    updateC = true;
                break;
            // Amplitude, volume control - A
            case 0x08:
                amplA = value & 0b00011111;
                updateA = true;
                break;
            // Amplitude, volume control - B
            case 0x09:
                amplB = value & 0b00011111;
                updateB = true; 
                break;
            // Amplitude, volume control - C
            case 0x0a:
                amplC = value & 0b00011111;
                updateC = true;
                break;
            // Envelope period - fine
            case 0x0b:
                env_period = env_period & 0b1111111100000000;
                env_period = env_period + value;
                break;
            // Envelope period - coarse
            case 0x0c:
                env_period = env_period & 0b0000000011111111;
                env_period = env_period + (value << 8);
                break;
            // Envelope shape
            case 0x0d:
                env_shape = value & 0b00001111;
                break;
            default:
                break;
        }
    }

    // update sound
    if (updateA) 
    {
        updateSound (0,mixer,amplA,toneA,noise);
        updateA = false;
    }
    if (updateB) 
    {
        updateSound (1,mixer,amplB,toneB,noise);
        updateB = false;
    }
    if (updateC) 
    {
        updateSound (2,mixer,amplC,toneC,noise);
        updateC = false;
    }
}
uint8_t AY_3_8910::read (uint8_t port)
{
    uint8_t val=0;

    if (port==0xa2)
        val = 0x3f; // no things pressed on the joystick (yet), all signals 1-High

    return val;
}