#ifndef __FIREFLY_FIXED_H__
#define __FIREFLY_FIXED_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include <stddef.h>
#include <stdint.h>


/**
 *  A library for fixed:15.16 numeric operations.
 *
 *  This allows an int32_t to be used to store higher-precision
 *  values and for maths to be performed on, while retaining the
 *  performance of integer arithmetic.
 */

/**
 *  A fixed:15.16 value.
 */
typedef int32_t fixed_ffxt;


/**
 *  Constants
 */

extern const fixed_ffxt FM_1;      // 1
extern const fixed_ffxt FM_1_2;    // 1 / 2
extern const fixed_ffxt FM_1_4;    // 1 / 4
extern const fixed_ffxt FM_1_8;    // 1 / 8
extern const fixed_ffxt FM_1_16;   // 1 / 16

extern const fixed_ffxt FM_PI_2;   // pi / 2
extern const fixed_ffxt FM_PI;     // pi
extern const fixed_ffxt FM_3PI_2;  // 3 * pi / 2
extern const fixed_ffxt FM_2PI;    // 2 * pi

extern const fixed_ffxt FM_E;      // e

extern const fixed_ffxt FM_MAX;    // Largest possible positive value
extern const fixed_ffxt FM_MIN;    // Lowest possible negative value


/**
 *  Returns %%value%% as a fixed:15.16 with 0 as its decimals.
 *
 *  For example: tofx(1) == FM_1
 */
fixed_ffxt tofx(int32_t value);


// [ sign: 1 byte ][ 32767: 5 bytes ][ decimal: 1 byte ][ : 7 bytes][ NULL ]
#define FIXED_STRING_LENGTH     (16)

/**
 *  Creates a string representation of %%value%% into %%output%%.
 *  The %%output%% buffer must be at least [[FIXED_STRING_LENGTH]] bytes.
 *
 *  @example:
 *
 *    char output[COLOR_NAME_LENGTH];
 *    printf("Value: %s\n", ffx_sprintfx(FM_PI, output));
 *    // Value: 3.141601
 */
char* ffx_sprintfx(fixed_ffxt value, char *output);

/**
 *  Returns a fixed:15.16 for top / bottom, preserving any possible
 *  decimals.
 *
 *  For example: ratiofx(1, 2) == FM_1_2
 */
fixed_ffxt ratiofx(int32_t top, int32_t bottom);


/**
 *  Returns the value of %%x%% / %%y%%.
 */
fixed_ffxt divfx(fixed_ffxt x, fixed_ffxt y);

/**
 *  Returns the value of %%x%% * %%y%%.
 */
fixed_ffxt mulfx(fixed_ffxt x, fixed_ffxt y);

/**
 *  Returns %%scalar%% scaled by %%t%%.
 */
int32_t scalarfx(int32_t scalar, fixed_ffxt t);

/**
 *  Returns the log (base-2) of %%x%%.
 */
fixed_ffxt log2fx(fixed_ffxt x);

/**
 *  Returns the value of e raised to the power of  %%x%%.
 */
fixed_ffxt exp2fx(fixed_ffxt x);

/**
 *  Returns the value of %%x%% raised to the power of  %%y%%.
 */
fixed_ffxt powfx(fixed_ffxt x, fixed_ffxt y);

/**
 *  Returns the sine of %%x%%.
 *
 *  A third-order approximation is used and the values of 0
 */
fixed_ffxt sinfx(fixed_ffxt x);

/**
 *  Returns the cosine of %%x%%.
 */
fixed_ffxt cosfx(fixed_ffxt x);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_FIXED_H__ */
