#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include "sonarpen.h"
// Global variables
int playback_device = -1;
int capture_device = -1;
int sonarpen_detected = 0;  // 0 = not detected, 1 = detected

// Function prototypes
void autodetect_sonarpen();
void manual_device_selection();
int test_playback_capture_pair(int card, int playback_device, int capture_device);
void list_playback_devices();
void list_capture_devices();
int get_user_input_for_device(const char *prompt);
int init_audio_capture_device(AudioCapture *audio_capture, const char *device_name);
int init_audio_playback_device(const char *device_name);
int play_test_tone_left_channel(float frequency);
int capture_audio_compare_channels(AudioCapture *audio_capture, float *left_amp, float *right_amp);

// Main function
int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "--manual") == 0) {
        manual_device_selection();
    } else {
        autodetect_sonarpen();
    }

    if (sonarpen_detected) {
        // Proceed with initializing touchpad and virtual HID
        SPmouse_HID();  // Call your existing function to continue the process
    } else {
        printf("Failed to detect SonarPen. Exiting.\n");
        return 1;
    }

    return 0;
}

// Autodetect SonarPen
void autodetect_sonarpen() {
    // Scan available sound cards and devices
    printf("Autodetecting SonarPen...\n");

    // Iterate over sound cards and devices, testing each
    int card = -1;
    while (snd_card_next(&card) >= 0 && card >= 0) {
        int playback_device = -1;
        int capture_device = -1;

        printf("Testing sound card: %d\n", card);
        if (test_playback_capture_pair(card, playback_device, capture_device)) {
            // SonarPen detected
            sonarpen_detected = 1;
            break;
        }
    }

    // If no SonarPen detected, fallback to manual mode
    if (!sonarpen_detected) {
        printf("No SonarPen detected. Falling back to manual selection...\n");
        manual_device_selection();
    }
}

// Manual device selection
void manual_device_selection() {
    printf("Manual selection mode activated.\n");

    // List available playback devices
    list_playback_devices();

    // Let the user select a playback device
    printf("Select Playback Device:");
    int playback_input = get_user_input_for_device("Select Playback Device");
    int playback_card = playback_input >> 16;
    int playback_device = playback_input & 0xFFFF;

    // List available capture devices
    list_capture_devices();

    // Let the user select a capture device
    printf("Select Capture Device:");
    int capture_input = get_user_input_for_device("Select Capture Device");
    int capture_card = capture_input >> 16;
    int capture_device = capture_input & 0xFFFF;

    // Use the selected devices for playback and capture
    test_playback_capture_pair(playback_card, playback_device, capture_device);
}

// Test playback and capture device pair
int test_playback_capture_pair(int card, int playback_device, int capture_device) {
    // Initialize playback device
    char playback_device_name[32];
    snprintf(playback_device_name, sizeof(playback_device_name), "hw:%d,%d", card, playback_device);

    // Initialize capture device
    char capture_device_name[32];
    snprintf(capture_device_name, sizeof(capture_device_name), "hw:%d,%d", card, capture_device);

    // Initialize playback
    if (init_audio_playback_device(playback_device_name) < 0) {
        fprintf(stderr, "Failed to initialize playback device %s\n", playback_device_name);
        return 0;
    }

    // Initialize capture
    AudioCapture audio_capture = {0};
    if (init_audio_capture_device(&audio_capture, capture_device_name) < 0) {
        fprintf(stderr, "Failed to initialize capture device %s\n", capture_device_name);
        cleanup_audio_playback();
        return 0;
    }

    // Generate and play test tone on left channel
    if (play_test_tone_left_channel(2000.0) < 0) {
        fprintf(stderr, "Failed to play test tone on playback device %s\n", playback_device_name);
        cleanup_audio_capture(&audio_capture);
        cleanup_audio_playback();
        return 0;
    }

    // Capture audio and compare amplitudes
    float left_amp = 0.0f, right_amp = 0.0f;
    if (capture_audio_compare_channels(&audio_capture, &left_amp, &right_amp) < 0) {
        fprintf(stderr, "Failed to capture audio on capture device %s\n", capture_device_name);
        cleanup_audio_capture(&audio_capture);
        cleanup_audio_playback();
        return 0;
    }

    // Analyze amplitudes
    if (right_amp > left_amp * 2) {
        printf("SonarPen detected on card %d, playback device %d, capture device %d\n", card, playback_device, capture_device);
        sonarpen_detected = 1;
    } else {
        printf("SonarPen not detected on card %d, playback device %d, capture device %d\n", card, playback_device, capture_device);
    }

    // Cleanup
    cleanup_audio_capture(&audio_capture);
    cleanup_audio_playback();

    return sonarpen_detected;
}

// List available playback devices
void list_playback_devices() {
    int card = -1;
    snd_ctl_t *handle;
    snd_ctl_card_info_t *card_info;
    snd_pcm_info_t *pcm_info;
    snd_ctl_card_info_alloca(&card_info);
    snd_pcm_info_alloca(&pcm_info);

    printf("Available playback devices:\n");

    while (snd_card_next(&card) >= 0 && card >= 0) {
        char card_name[32];
        snprintf(card_name, sizeof(card_name), "hw:%d", card);
        if (snd_ctl_open(&handle, card_name, 0) < 0) continue;
        if (snd_ctl_card_info(handle, card_info) < 0) {
            snd_ctl_close(handle);
            continue;
        }

        int device = -1;
        while (snd_ctl_pcm_next_device(handle, &device) >= 0 && device >= 0) {
            snd_pcm_info_set_device(pcm_info, device);
            snd_pcm_info_set_subdevice(pcm_info, 0);
            snd_pcm_info_set_stream(pcm_info, SND_PCM_STREAM_PLAYBACK);
            if (snd_ctl_pcm_info(handle, pcm_info) >= 0) {
                printf("  Card %d, Device %d: %s\n", card, device, snd_pcm_info_get_name(pcm_info));
            }
        }
        snd_ctl_close(handle);
    }
}

// List available capture devices
void list_capture_devices() {
    int card = -1;
    snd_ctl_t *handle;
    snd_ctl_card_info_t *card_info;
    snd_pcm_info_t *pcm_info;
    snd_ctl_card_info_alloca(&card_info);
    snd_pcm_info_alloca(&pcm_info);

    printf("Available capture devices:\n");

    while (snd_card_next(&card) >= 0 && card >= 0) {
        char card_name[32];
        snprintf(card_name, sizeof(card_name), "hw:%d", card);
        if (snd_ctl_open(&handle, card_name, 0) < 0) continue;
        if (snd_ctl_card_info(handle, card_info) < 0) {
            snd_ctl_close(handle);
            continue;
        }

        int device = -1;
        while (snd_ctl_pcm_next_device(handle, &device) >= 0 && device >= 0) {
            snd_pcm_info_set_device(pcm_info, device);
            snd_pcm_info_set_subdevice(pcm_info, 0);
            snd_pcm_info_set_stream(pcm_info, SND_PCM_STREAM_CAPTURE);
            if (snd_ctl_pcm_info(handle, pcm_info) >= 0) {
                printf("  Card %d, Device %d: %s\n", card, device, snd_pcm_info_get_name(pcm_info));
            }
        }
        snd_ctl_close(handle);
    }
}

// Get user input for device selection
int get_user_input_for_device(const char *prompt) {
    int card, device;
    printf("%s\n", prompt);
    printf("Enter card number: ");
    scanf("%d", &card);
    printf("Enter device number: ");
    scanf("%d", &device);
    return (card << 16) | device;  // Combine card and device into a single integer
}

// Initialize audio capture device
int init_audio_capture_device(AudioCapture *audio_capture, const char *device_name) {
    int err = snd_pcm_open(&audio_capture->handle, device_name, SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0) {
        fprintf(stderr, "Error opening capture device: %s\n", snd_strerror(err));
        return -1;
    }
    // Other initialization...
    return 0;
}

// Initialize audio playback device
int init_audio_playback_device(const char *device_name) {
    int err = snd_pcm_open(&playback_handle, device_name, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        fprintf(stderr, "Error opening playback device: %s\n", snd_strerror(err));
        return -1;
    }
    // Other initialization...
    return 0;
}

// Play test tone on left channel only
int play_test_tone_left_channel(float frequency) {
    // Implement test tone playback on left channel...
    return 0;
}

// Capture and compare audio from left and right channels
int capture_audio_compare_channels(AudioCapture *audio_capture, float *left_amp, float *right_amp) {
    int err = snd_pcm_readi(audio_capture->handle, audio_capture->buffer, BUFFER_SIZE / 4);  // Divide by 4 for stereo
    if (err < 0) {
        fprintf(stderr, "Error capturing audio: %s\n", snd_strerror(err));
        return -1;
    }

    int16_t *samples = (int16_t *)audio_capture->buffer;
    int num_samples = err * 2;  // Stereo samples

    float left_sum = 0.0f, right_sum = 0.0f;
    for (int i = 0; i < num_samples; i += 2) {
        left_sum += fabs(samples[i]);      // Left channel
        right_sum += fabs(samples[i + 1]); // Right channel
    }

    *left_amp = left_sum / (num_samples / 2);
    *right_amp = right_sum / (num_samples / 2);

    return 0;
}
