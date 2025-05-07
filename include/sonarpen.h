#ifndef SONARPEN_H
#define SONARPEN_H

// Standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

// Audio libraries
#include <alsa/asoundlib.h>

// Touchpad interaction
#include <libevdev/libevdev.h>
#include <libudev.h>

// uinput for virtual input devices
#include <linux/uinput.h>
#include <sys/ioctl.h>
#include <sys/time.h>

// Constant values
/**
 * @brief Size of the audio buffer.
 */
#define BUFFER_SIZE 4096

/**
 * @brief PCM device name for audio capture and playback.
 * 
 * This can be changed with a function to assign it properly.
 */
#define PCM_DEVICE "default"

/**
 * @brief Maximum RMS value for audio samples (16-bit signed audio).
 */
#define MAX_RMS_VALUE 32767

// Audio Capture 

/**
 * @brief Structure to hold audio capture parameters.
 */
typedef struct {
    char *buffer;               /**< Buffer to hold audio data. */
    snd_pcm_t *handle;          /**< PCM device handle. */
    snd_pcm_hw_params_t *params;/**< Hardware parameters for PCM device. */
} AudioCapture;

// Functions for Audio Capture

int init_audio_capture(AudioCapture *audio_capture);
float capture_audio(AudioCapture *audio_capture);
void cleanup_audio_capture(AudioCapture *audio_capture);
float calculate_rms(int16_t *samples, int num_samples);

// Functions for Sound Generation

int init_audio_playback(void);
int play_tone(float frequency);
void cleanup_audio_playback(void);

// Functions for Touchpad Interaction

int init_touchpad_device(struct libevdev **dev, const char *path);
void process_touchpad_events(struct libevdev *dev);
int read_touchpad_events(const char *device_path);

// Uinput device and event handling
int setup_uinput_device(void);
void emit(int fd, int type, int code, int value);

#endif // SONARPEN_H
