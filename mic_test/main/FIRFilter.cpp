#include "FIRFilter.h"
#include <string.h> // for memset, memcpy, memmove

FIRFilter::FIRFilter() {
    memset(insamp, 0, sizeof(insamp));

    // Bandpass filter centered at 1kHz, Fs = 8kHz
    double temp[FILTER_LEN] = {
        -0.0448093,  0.0322875,   0.0181163,   0.0087615,   0.0056797,
         0.0086685,  0.0148049,   0.0187190,   0.0151019,   0.0027594,
        -0.0132676, -0.0232561,  -0.0187804,   0.0006382,   0.0250536,
         0.0387214,  0.0299817,   0.0002609,  -0.0345546,  -0.0525282,
        -0.0395620,  0.0000246,   0.0440998,   0.0651867,   0.0479110,
         0.0000135, -0.0508558,  -0.0736313,  -0.0529380,  -0.0000709,
         0.0540186,  0.0766746,   0.0540186,  -0.0000709,  -0.0529380,
        -0.0736313, -0.0508558,   0.0000135,   0.0479110,   0.0651867,
         0.0440998,  0.0000246,  -0.0395620,  -0.0525282,  -0.0345546,
         0.0002609,  0.0299817,   0.0387214,   0.0250536,   0.0006382,
        -0.0187804, -0.0232561,  -0.0132676,   0.0027594,   0.0151019,
         0.0187190,  0.0148049,   0.0086685,   0.0056797,   0.0087615,
         0.0181163,  0.0322875,  -0.0448093
    };
    memcpy(coeffs, temp, sizeof(coeffs));
}

void FIRFilter::apply(int* input, int length) {
    double floatInput[MAX_INPUT_LEN];
    double floatOutput[MAX_INPUT_LEN];

    // Convert to float
    intToFloat(input, floatInput, length);

    // Copy new samples into insamp
    memcpy(&insamp[FILTER_LEN - 1], floatInput, length * sizeof(double));

    // FIR filtering
    for (int n = 0; n < length; n++) {
        double acc = 0.0;
        double* coeffp = coeffs;
        double* inputp = &insamp[FILTER_LEN - 1 + n];

        for (int k = 0; k < FILTER_LEN; k++) {
            acc += (*coeffp++) * (*inputp--);
        }
        floatOutput[n] = acc;
    }

    // Shift old samples
    memmove(&insamp[0], &insamp[length], (FILTER_LEN - 1) * sizeof(double));

    // Convert back to int
    floatToInt(floatOutput, input, length);  // overwrite input with filtered output
}

void FIRFilter::intToFloat(int* input, double* output, int length) {
    for (int i = 0; i < length; i++) {
        output[i] = (double)input[i];
    }
}

void FIRFilter::floatToInt(double* input, int* output, int length) {
    for (int i = 0; i < length; i++) {
        if (input[i] > 32767.0)
            input[i] = 32767.0;
        else if (input[i] < -32768.0)
            input[i] = -32768.0;

        output[i] = (int)input[i];
    }
}
