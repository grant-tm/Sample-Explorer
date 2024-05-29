// g++ keydet.cpp -o keydet -std=c++17 ..\repos\kissfft\kiss_fft.c -I ..\repos\AudioFile\AudioFile.h

#include "../inc/KeyDet.h"

namespace fs = std::filesystem;

// Define the weights for the 12 semitones in both major and minor keys
const std::vector<int> key_templates[24] = {
    // major keys
    {0, 2, 4, 5, 7, 9, 11},  // C major
    {1, 3, 5, 6, 8, 10, 0},  // C# major
    {2, 4, 6, 7, 9, 11, 1},  // D major
    {3, 5, 7, 8, 10, 0, 2},  // D# major
    {4, 6, 8, 9, 11, 1, 3},  // E major
    {5, 7, 9, 10, 0, 2, 4},  // F major
    {6, 8, 10, 11, 1, 3, 5}, // F# major
    {7, 9, 11, 0, 2, 4, 6},  // G major
    {8, 10, 0, 1, 3, 5, 7},  // G# major
    {9, 11, 1, 2, 4, 6, 8},  // A major
    {10, 0, 2, 3, 5, 7, 9},  // A# major
    {11, 1, 3, 4, 6, 8, 10}, // B major
};

const char* int_to_key(int key_num) {
    switch (key_num) {
        case 0:  return "C Major";
        case 1:  return "C# Major";
        case 2:  return "D Major";
        case 3:  return "D# Major";
        case 4:  return "E Major";
        case 5:  return "F Major";
        case 6:  return "F# Major";
        case 7:  return "G Major";
        case 8:  return "G# Major";
        case 9:  return "A Major";
        case 10: return "A# Major";
        case 11: return "B Major";
        default: return "Unknown Key";
    }
}

std::vector<float> make_mono(std::vector<std::vector<float>> &samples) {
    // Determine the number of channels and the number of samples per channel
    size_t num_channels = samples.size();
    if (num_channels == 0) {
        return std::vector<float>(); // Return an empty vector if there are no channels
    }
    size_t num_samples = samples[0].size();

    // Initialize the mono samples vector with the same number of samples
    std::vector<float> mono_samples(num_samples, 0.0f);

    // Sum the samples from each channel
    for (size_t i = 0; i < num_samples; ++i) {
        for (size_t ch = 0; ch < num_channels; ++ch) {
            mono_samples[i] += samples[ch][i];
        }
        // Average the summed samples
        mono_samples[i] /= num_channels;
    }

    return mono_samples;
}

// copy sample input
void pack_input (std::vector<float> &samples, int offset, kiss_fft_cpx *input) {
    for (int i=0; i<FFT_WINDOW_SIZE; i++) {
        //std::cout << "\rIter: " << i;
        input[i].r = samples[i + offset];
        input[i].i = samples[i + offset];
    }
}

// run the kissfft
void perform_fft (kiss_fft_cpx *in, kiss_fft_cpx *out) {
    kiss_fft_cfg cfg = kiss_fft_alloc(FFT_WINDOW_SIZE, 0, NULL, NULL);
    kiss_fft(cfg, in, out);
    kiss_fft_free(cfg);
}

// calculate magnitude of cosine factor
float inline magnitude (kiss_fft_cpx *inum) {
    float real = pow(inum->r, 2.0);
    float imag = pow(inum->i, 2.0);
    float square = pow(real + imag, 0.5);
    return (square < 1.0) ? 0.0 : square;
}

// convert FFT index to the nearest midi number
int inline midi_note (int index, int sample_rate, int window_size) {
    // calculate the nearest MIDI note number
    float freq = index * sample_rate / window_size;
    int midi_note = std::round(69 + 12 * std::log2(freq / 440.0));
    return std::max(0, midi_note);
}

// accumualte fft results
void accumulate_midi_notes (float *midi_map, kiss_fft_cpx *output, int sample_rate, int window_size){
    
    if (!midi_map) {
        return;
    }

    // for each output index:
    // 1. calculate nearest midi note
    // 2. accumualte magnitude of fft result in midi map
    for (int i=0; i<window_size/2; i++) {
        int note_id = midi_note(i, sample_rate, window_size);
        if (note_id > 127) { note_id = 127; }
        if (note_id < 0 ) { note_id = 0; }
        
        float mag = magnitude(&output[i]);
        midi_map[note_id] += mag;
    }
}

// Function to determine the musical key based on weights
int assign_key (float *weights) {

    std::vector<float> key_weights(12, 0.0f);

    // Sum the weights for each key
    for (int key = 0; key < 12; key++) {
        for (int note : key_templates[key]) {
            for (int octave = 0; octave < 11; ++octave) {
                int midi_note = note + 12 * octave;
                if (midi_note < 128) {
                    key_weights[key] += weights[midi_note];
                }
            }
        }
    }

    // Find the key with the highest weight
    int best_key = std::distance(key_weights.begin(), std::max_element(key_weights.begin(), key_weights.end()));
    return best_key;
}

int detect_key (std::string path) {
    
    // open an audio file
    //std::string path = "D:/Samples/Instruments/Keys/Loops/JVIEWS_cassette_keys_86_Amin.wav";
    AudioFile<float> file;
    if (!file.load(path)) {
        std::cerr << "Failed to open audio file: " << path << std::endl;
        return EXIT_FAILURE;
    }

    std::vector<float> samples = make_mono(file.samples);

    // Create and initialize input output arrays
    kiss_fft_cpx input[FFT_WINDOW_SIZE];
    kiss_fft_cpx output[FFT_WINDOW_SIZE];
    for (int i=0; i<FFT_WINDOW_SIZE; i++) {
        input[i].r = 0;
        input[i].i = 0;
        output[i].r = 0;
        output[i].i = 0;
    }

    // perform accumualtion
    float midi[128] = {0.0};
    for (int i=0; i<=(file.getNumSamplesPerChannel()-FFT_WINDOW_SIZE); i += FFT_WINDOW_SIZE) {
        pack_input(samples, i, input);
        perform_fft(input, output);
        accumulate_midi_notes(midi, output, file.getSampleRate(), FFT_WINDOW_SIZE);
    }

    // assign key
    return assign_key(midi);
}