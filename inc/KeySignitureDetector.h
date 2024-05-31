#ifndef KEY_SIGNITURE_DETECTOR_H
#define KEY_SIGNITURE_DETECTOR_H

#include <vector>
#include <string>
#include <math.h>
#include <filesystem>
#include <thread>
#include <mutex>
#include "AudioFile.h"
#include "kiss_fft.h"

#define FFT_WINDOW_SIZE 8192 * 2
#define MAX_ANALYSIS_TIME FFT_WINDOW_SIZE * 16

class MidiMap {
private:
    std::mutex mutex;
    float weights[128] = {0.0};

public:
    // Define the weights for the 12 semitones in both major and minor keys
    const std::vector<int> key_templates[24] = {
        // major keys
        {0, 2, 4, 5, 7, 9, 11},  // C  maj : A  min
        {1, 3, 5, 6, 8, 10, 0},  // C# maj : A# min
        {2, 4, 6, 7, 9, 11, 1},  // D  maj : B  min
        {3, 5, 7, 8, 10, 0, 2},  // D# maj : C  min
        {4, 6, 8, 9, 11, 1, 3},  // E  maj : C# min
        {5, 7, 9, 10, 0, 2, 4},  // F  maj : D  min
        {6, 8, 10, 11, 1, 3, 5}, // F# maj : D# min
        {7, 9, 11, 0, 2, 4, 6},  // G  maj : E  min
        {8, 10, 0, 1, 3, 5, 7},  // G# maj : F  min
        {9, 11, 1, 2, 4, 6, 8},  // A  maj : F# min
        {10, 0, 2, 3, 5, 7, 9},  // A# maj : G  min
        {11, 1, 3, 4, 6, 8, 10}, // B  maj : G# min
    };

    // Translate a key template index to the key signiture as a string
    const char* int_to_key (int);

    // set a note weight directly
    void set_weight (int note, float value);

    // increment note weight by an amount
    void inc_weight (int, float);


    // decrement a note weight by an amount
    void dec_weight (int, float);

    // read a note weight value
    float read_weight(int);

};

bool path_is_ascii(std::string path);

std::vector<float> make_mono(std::vector<std::vector<float>> &samples);

// calculate magnitude of cosine factor
float magnitude (kiss_fft_cpx *inum);

// calculate the nearest MIDI note number
int midi_note (int index, int sample_rate);

// Function to determine the musical key based on weights
int assign_key (MidiMap *midi_map);

void process_segment (const std::vector<float> *samples, int start, int sample_rate, MidiMap *midi_map);

int kdet_detect_key (std::string path);

#endif // KEY_SIGNITURE_DETECTOR_H