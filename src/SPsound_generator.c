#include "sonarpen.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alsa/asoundlib.h>

#define BUFFER_LEN 1024  // Use a smaller buffer size suitable for real-time playback
#define SAMPLE_RATE 48000
#define PCM_DEVICE "default"

static snd_pcm_t *playback_handle = NULL;
static unsigned int sample_rate = SAMPLE_RATE;
static int channels = 2;
static double phase = 0.0;  // Keep phase between calls

int init_audio_playback() {
    int err;
    // Open PCM device for playback
    if ((err = snd_pcm_open(&playback_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        fprintf(stderr, "Playback open error: %s\n", snd_strerror(err));
        return -1;
    }

    // Set hardware parameters
    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(playback_handle, params);

    // Set access type
    snd_pcm_hw_params_set_access(playback_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

    // Set sample format
    snd_pcm_hw_params_set_format(playback_handle, params, SND_PCM_FORMAT_S16_LE);

    // Set channels
    snd_pcm_hw_params_set_channels(playback_handle, params, channels);

    // Set sample rate
    snd_pcm_hw_params_set_rate_near(playback_handle, params, &sample_rate, 0);

    // Apply hardware parameters
    if ((err = snd_pcm_hw_params(playback_handle, params)) < 0) {
        fprintf(stderr, "Unable to set HW parameters: %s\n", snd_strerror(err));
        snd_pcm_close(playback_handle);
        return -1;
    }

    return 0;
}

int play_tone(float frequency) {
    short buffer[BUFFER_LEN * channels];
    int err;

    // Generate sine wave
    double step = 2 * M_PI * frequency / sample_rate;

    for (int i = 0; i < BUFFER_LEN; i++) {
        short value = (short)(32767 * sin(phase));
        buffer[i * 2] = 0;         // Left channel silent
        buffer[i * 2 + 1] = value; // Right channel with tone
        phase += step;
        if (phase >= 2 * M_PI) phase -= 2 * M_PI;
    }

    // Write audio data
    err = snd_pcm_writei(playback_handle, buffer, BUFFER_LEN);
    if (err == -EPIPE) {
        // Buffer underrun
        fprintf(stderr, "Buffer underrun\n");
        snd_pcm_prepare(playback_handle);
    } else if (err < 0) {
        fprintf(stderr, "Error writing to PCM device: %s\n", snd_strerror(err));
        return -1;
    }

    return 0;
}

void cleanup_audio_playback() {
    if (playback_handle) {
        snd_pcm_drain(playback_handle);
        snd_pcm_close(playback_handle);
        playback_handle = NULL;
    }
}
