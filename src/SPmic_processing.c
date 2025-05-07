#include "sonarpen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> // For int16_t
#include <math.h>   // For sqrt
#include <alsa/asoundlib.h> // Include ALSA library

/* This page includes processes to fetch the mic feedback needed to calculate the RMS of the Sine signal */

// Constants
#define BUFFER_SIZE 4096 // Define the buffer size
#define PCM_DEVICE "default" // Define the PCM device

// Function declarations
int init_audio_capture(AudioCapture *audio_capture);
float capture_audio(AudioCapture *audio_capture);
void cleanup_audio_capture(AudioCapture *audio_capture);

/**
 * @brief Initialize audio capture.
 * 
 * This function allocates memory for the audio buffer, opens the PCM device
 * for recording, and sets the hardware parameters for audio capture.
 * 
 * @param audio_capture Pointer to the AudioCapture structure.
 * @return int 0 on success, -1 on failure.
 */
int init_audio_capture(AudioCapture *audio_capture) {
    int err;

    // Allocate memory for the audio buffer
    audio_capture->buffer = (char *)malloc(BUFFER_SIZE);
    if (audio_capture->buffer == NULL) {
        fprintf(stderr, "Failed to allocate memory for audio buffer\n");
        return -1;
    }

    // Open the PCM device for recording (input)
    if ((err = snd_pcm_open(&audio_capture->handle, PCM_DEVICE, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf(stderr, "Unable to open PCM device: %s\n", snd_strerror(err));
        free(audio_capture->buffer);
        return -1;
    }

    // Allocate hardware parameters
    snd_pcm_hw_params_alloca(&audio_capture->params);
    snd_pcm_hw_params_any(audio_capture->handle, audio_capture->params);

    // Set the desired hardware parameters
    snd_pcm_hw_params_set_access(audio_capture->handle, audio_capture->params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(audio_capture->handle, audio_capture->params, SND_PCM_FORMAT_S16_LE);
    unsigned int rate = 44100;
    snd_pcm_hw_params_set_rate_near(audio_capture->handle, audio_capture->params, &rate, 0);
    snd_pcm_hw_params_set_channels(audio_capture->handle, audio_capture->params, 1);

    // Apply the hardware parameters to the PCM device
    if ((err = snd_pcm_hw_params(audio_capture->handle, audio_capture->params)) < 0) {
        fprintf(stderr, "Unable to set HW parameters: %s\n", snd_strerror(err));
        snd_pcm_close(audio_capture->handle);
        free(audio_capture->buffer);
        return -1;
    }

    return 0;
}

// Chapter 1: Calculate RMS Volume
float calculate_rms(int16_t *samples, int num_samples) {
    if (num_samples <= 0) {
        return 0.0f; // Return 0 for invalid sample count
    }

    float sum_of_squares = 0.0f;

    // Sum of squares of samples
    for (int i = 0; i < num_samples; i++) {
        sum_of_squares += samples[i] * samples[i];
    }

    // Calculate RMS
    return sqrt(sum_of_squares / num_samples); // RMS value
}

/**
 * @brief Capture audio from the PCM device.
 * 
 * This function reads audio data from the PCM device into the buffer,
 * processes the samples, and calculates the RMS value of the captured audio.
 * 
 * @param audio_capture Pointer to the AudioCapture structure.
 * @return float RMS value of the captured audio, or -1.0 on error.
 */
float capture_audio(AudioCapture *audio_capture) {
    int err = snd_pcm_readi(audio_capture->handle, audio_capture->buffer, BUFFER_SIZE / 2);
    if (err < 0) {
        fprintf(stderr, "Error capturing audio: %s\n", snd_strerror(err));
        return -1.0f; // Return -1.0 on error
    }

    // Cast buffer to int16_t pointer
    int16_t *samples = (int16_t *)audio_capture->buffer;

    // Output the int16 array (audio samples)
    /*printf("Captured Audio Samples:\n");
    for (int i = 0; i < err; i++) {
        printf("%d ", samples[i]); // Print each sample value
    }
    printf("\n");
    fflush(stdout); // Flush output to ensure it appears immediately
*/
    // Calculate RMS using the existing function
    return calculate_rms(samples, err); // Return the RMS value as the volume

}

/**
 * @brief Cleanup audio capture resources.
 * 
 * This function closes the PCM device and frees the allocated buffer.
 * 
 * @param audio_capture Pointer to the AudioCapture structure.
 */
void cleanup_audio_capture(AudioCapture *audio_capture) {
    if (audio_capture->handle) {
        snd_pcm_close(audio_capture->handle);
    }
    free(audio_capture->buffer);
}

