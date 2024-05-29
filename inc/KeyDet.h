#ifndef KEY_DET_H
#define KEY_DET_H

#include <vector>
#include <string>
#include <math.h>
#include <filesystem>
#include "AudioFile.h"
#include "kiss_fft.h"

#define FFT_WINDOW_SIZE 8192 * 2

const char* int_to_key (int key_num);

std::vector<float> make_mono (std::vector<std::vector<float>> &);

void pack_input (std::vector<float> &, int , kiss_fft_cpx *);

void perform_fft (kiss_fft_cpx *, kiss_fft_cpx *);

float inline magnitude (kiss_fft_cpx *);

int inline midi_note (int, int, int);

void accumulate_midi_notes (float *, kiss_fft_cpx *, int, int);

int assign_key (float *);

int detect_key (std::string);

#endif // KEY_DET_H