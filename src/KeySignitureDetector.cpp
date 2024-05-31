#include "../inc/KeySignitureDetector.h"

namespace fs = std::filesystem;

//==============================================================================
// MidiMap Definitions
//==============================================================================

// Translate a key template index to the key signiture as a string
const char* MidiMap::int_to_key(int key_num) {
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

// set a note weight directly
void MidiMap::set_weight (int note, float value) {
    std::lock_guard<std::mutex> guard(mutex);
    if (note >= 0 && note < 128) {
        weights[note] = value;
        return;
    }
}

// increment note weight by an amount
void MidiMap::inc_weight (int note, float value) {
    std::lock_guard<std::mutex> guard(mutex);
    if (note >= 0 && note < 128) {
        weights[note] += value;
        return;
    }
}


// decrement a note weight by an amount
void MidiMap::dec_weight (int note, float value) {
    std::lock_guard<std::mutex> guard(mutex);
    if (note >= 0 && note < 128) {
        weights[note] += value;
        return;
    }
}

// read a note weight value
float MidiMap::read_weight(int index) {
    std::lock_guard<std::mutex> guard(mutex);
    if (index >= 0 && index < 128) {
        return weights[index];
    } else {
        return 0;
    }
}

//==============================================================================
// Other Functions
//==============================================================================

bool path_is_ascii(std::string path) {
    for (char c : path) {
        if (static_cast<unsigned char>(c) > 127) {
            return false;
        }
    }
    return true;
}

std::vector<float> make_mono(std::vector<std::vector<float>> &samples) {
    // determine the number of channels and the number of samples per channel
    size_t num_channels = samples.size();
    
    // return an empty vector if there are no channels
    if (num_channels == 0) {
        return std::vector<float>(); 
    }
    size_t num_samples = samples[0].size();

    // initialize the mono samples vector with the same number of samples
    std::vector<float> mono_samples(num_samples, 0.0f);

    // sum the samples from each channel
    for (size_t i = 0; i < num_samples; ++i) {
        for (size_t ch = 0; ch < num_channels; ++ch) {
            mono_samples[i] += samples[ch][i];
        }
        // average the summed samples
        mono_samples[i] /= num_channels;
    }

    return mono_samples;
}

// calculate magnitude of cosine factor
float magnitude (kiss_fft_cpx *inum) {
    float real = pow(inum->r, 2.0);
    float imag = pow(inum->i, 2.0);
    float square = pow(real + imag, 0.5);
    return (square < 1.0) ? 0.0 : square;
}

// calculate the nearest MIDI note number
int midi_note (int index, int sample_rate) {
    
    float freq = index * sample_rate / FFT_WINDOW_SIZE;
    
    int midi_note = std::round(69 + 12 * std::log2(freq / 440.0));
    if (midi_note > 127) [[unlikely]] { return 127; }
    else if (midi_note <   0) [[unlikely]] { return 0; }
    else {return midi_note; }
}

// Function to determine the musical key based on weights
int assign_key (MidiMap *midi_map) {

    std::vector<float> key_weights(12, 0.0f);

    // Sum the weights for each key
    for (int key = 0; key < 12; key++) {
        for (int note : midi_map->key_templates[key]) {
            for (int octave = 0; octave < 11; ++octave) {
                int midi_note = note + 12 * octave;
                if (midi_note < 128) {
                    key_weights[key] += midi_map->read_weight(midi_note);
                }
            }
        }
    }

    // Find the key with the highest weight
    auto begin = key_weights.begin();
    auto end = key_weights.end();
    return std::distance(begin, std::max_element(begin, end));
}

void process_segment (const std::vector<float> *samples, int start, 
                                        int sample_rate, MidiMap *midi_map) {

    // accumulate the results in the midi map
    if (!midi_map) {
        return;
    }

    // pack samples in fft input arrays
    kiss_fft_cpx input[FFT_WINDOW_SIZE];
    kiss_fft_cpx output[FFT_WINDOW_SIZE];
    for (int i=0; i<FFT_WINDOW_SIZE; i++) {
        input[i].r = (*samples)[start+i];
        input[i].i = (*samples)[start+i];
    }

    // perform the fft
    kiss_fft_cfg cfg = kiss_fft_alloc(FFT_WINDOW_SIZE, 0, NULL, NULL);
    kiss_fft(cfg, input, output);;
    kiss_fft_free(cfg);

    // Accumulate fft results in midi map
    // for each output index:
    // 1. calculate nearest midi note
    // 2. accumualte magnitude of fft result in midi map
    int limit = FFT_WINDOW_SIZE / 2;
    for (int i=0; i<limit; i++) {
        int note_id = midi_note(i, sample_rate);
        float mag = magnitude(&output[i]);
        midi_map->inc_weight(note_id, mag);
    }

    return;
}

int kdet_detect_key (std::string path) {
    
    fprintf(stderr, "\r%s", path.c_str());

    // get audio file samples
    AudioFile<float> file;
    if (!path_is_ascii(path) || !file.load(path)) {
        return -1;
    } 
    int sample_rate = file.getSampleRate();
    const std::vector<float> samples = make_mono(file.samples);

    MidiMap midi_map;

    // break file samples into segments and process each
    int num_sub = file.getNumSamplesPerChannel() - FFT_WINDOW_SIZE;
    int maxx = (num_sub > MAX_ANALYSIS_TIME) ? MAX_ANALYSIS_TIME : num_sub;
    
    std::vector<std::thread> threads;
    for (int i=0; i<=num_sub; i += FFT_WINDOW_SIZE) {
        threads.emplace_back(&process_segment, &samples, i, sample_rate, &midi_map);
    }

    // Join all threads
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    // assign key based on fft results
    return assign_key(&midi_map);
}