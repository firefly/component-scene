#include <stdio.h>
#include <math.h>
#include <stdint.h>

// This was thrown together quickly. Do not judge me based on it alone. :)

/**
 *  To run:
 *    gcc generate-scale.c && ./a.out
 */

/*

Generated Output:

bits=2 factor=43691 shift=1 drift=0.000010 one=10000
bits=3 factor=74899 shift=3 drift=0.000028 one=10000
bits=4 factor=34953 shift=3 drift=0.000057 one=10000
bits=5 factor=67651 shift=5 drift=0.000129 one=10000
bits=6 factor=532617 shift=9 drift=0.000292 one=10000
bits=7 factor=264211 shift=9 drift=0.000562 one=10000
bits=8 factor=32897 shift=7 drift=0.000971 one=10000
bits=9 factor=4202561 shift=15 drift=0.002375 one=10000
bits=10 factor=1049613 shift=14 drift=0.004831 one=10000
bits=11 factor=262275 shift=13 drift=0.009682 one=10000
bits=12 factor=262211 shift=14 drift=0.019190 one=10000
bits=13 factor=262179 shift=15 drift=0.037710 one=10000
bits=14 factor=65541 shift=14 drift=0.074992 one=10000
bits=15 factor=65539 shift=15 drift=0.138886 one=10000

*/

// Compute the total drift (badness) of a given factor and shift
float computeDrift(uint32_t bits, uint32_t factor, uint32_t sh, uint32_t show) {
    uint32_t count = 1 << bits;
    float divisor = count - 1;

    float drift = 0;
    for (int i = 0; i < count; i++) {
        int32_t f = (i * factor) >> sh;
        float ideal = i / divisor;
        float real = ((float)f) / ((float)0x10000);
        if (show) {
            printf("i=%d ideal=%f real=%f f=%04x drift=%f\n",
              i, ideal, real, f, real - ideal);
        }
        drift += fabs(real - ideal);
    }
    return drift;
}

// Find the results for a given bit-depth
void find(uint32_t bits) {
    uint32_t count = 1 << bits;
    float divisor = count - 1;

    //printf("bits=%d count=%d divisor: %f\n", bits, count, divisor);
    float bestDrift = 999999.0f;
    int bestF = 0, bestS = 0;
    for (int s = 0; s < 32; s++) {
        uint32_t factorLo = (0x10000 << s) / (count + 2);
        uint32_t factorHi = (0x10000 << s) / (count - 2);
        for (int f = factorLo; f < factorHi; f++) {
            float drift = computeDrift(bits, f, s, 0);
            if (drift < bestDrift) {
                //printf("factor=%d shift=%d drift=%f\n", f, s, drift);
                bestS = s;
                bestF = f;
                bestDrift = drift;
            }
        }
    }

    float drift = computeDrift(bits, bestF, bestS, 0);
    printf("bits=%d factor=%d shift=%d drift=%f one=%04x\n",
      bits, bestF, bestS, drift, ((count - 1) * bestF) >> bestS);
}

int main() {
    for (int i = 2; i < 16; i++) { find(i); }
    return 0;
}
