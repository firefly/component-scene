#include <stdio.h>
#include <math.h>
#include <stdint.h>

// This was thrown together quickly. Do not judge me based on it alone. :)

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
    for (int i = 2; i < 24; i++) { find(i); }
    return 0;
}
