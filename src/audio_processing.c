#include <math.h> // For sqrt
#include <stdint.h> // For int16_t

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

// Chapter 2: Additional Audio Processing Functions (if needed)
// Placeholder for any future audio processing functions
/*
float another_audio_processing_function(...) {
    // Implementation goes here
}
*/
