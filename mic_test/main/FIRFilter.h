#ifndef FIR_FILTER_H
#define FIR_FILTER_H

#include <stdint.h>
#include <stddef.h>

class FIRFilter {
public:
    FIRFilter();
    void apply(int* input, int length);

private:
    static const int FILTER_LEN = 63;
    static const int MAX_INPUT_LEN = 1024;
    static const int BUFFER_LEN = FILTER_LEN - 1 + MAX_INPUT_LEN;

    double insamp[BUFFER_LEN];
    double coeffs[FILTER_LEN];

    void intToFloat(int* input, double* output, int length);
    void floatToInt(double* input, int* output, int length);
};

#endif // FIR_FILTER_H
