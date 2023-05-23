#include <stdint.h>
#include <stdio.h>

const uint32_t MANTISSA_BITS = 23;
const uint32_t NORM_MASK = (1 << MANTISSA_BITS);
const uint32_t MANTISSA_MASK = (NORM_MASK - 1);
const uint32_t SIGN_MASK = (1 << 31);
const uint32_t EXPONENT_MASK = (SIGN_MASK - 1) ^ MANTISSA_MASK;
const uint32_t EXPONENT_BIAS = 127;

#define MIN(a, b) (((a) < (b))? (a) : (b))
#define MAX(a, b) (((a) > (b))? (a) : (b))
#define CLAMP(x, min, max) MAX(min, MIN(max, x))

typedef struct {
  int32_t value;
  int scaling;
} FixedPointNumber;

#define FIXED_POINT_ZERO (FixedPointNumber) { .value = 0, .scaling = 0 }
#define FIXED_POINT_MAX (FixedPointNumber) { .value = SIGN_MASK - 1, .scaling = 0 }
#define FIXED_POINT_MIN (FixedPointNumber) { .value = -SIGN_MASK, .scaling = 0 }

int count_leftmost_used_bits(uint32_t x) {
  int i = 0;

  while (i <= 31 && x > 0) {
    x = x << 1;
    ++i;
  }

  return i;
}

int count_rightmost_used_bits(uint32_t x) {
  int i = 0;

  while (i <= 31 && x > 0) {
    x = x >> 1;
    ++i;
  }

  return i;
}

FixedPointNumber to_fixed_point(float x) {
  if (x == 0)
    return FIXED_POINT_ZERO;

  // Decompose IEEE-754
  uint32_t conv = *((uint32_t*)&x);
  int sign = (conv & SIGN_MASK) >> 31;
  int exp = ((conv & EXPONENT_MASK) >> MANTISSA_BITS) - EXPONENT_BIAS;
  int32_t mant = (conv & MANTISSA_MASK) | NORM_MASK;

  // Use the highest scaling factor possible (simplest solution between those that have maximum accuracy)
  int scaling = MIN(30 - exp, 30);

  if (scaling < 0) { 
    // The value doesn't fit. Saturate to max value (signed)
    return sign != 0? FIXED_POINT_MIN : FIXED_POINT_MAX;
  }

  int shift = (int)MANTISSA_BITS - exp - scaling;

  int32_t ans;
  if (shift > 0)
    ans = mant >> shift;
  else if (shift < 0)
    ans = mant << -shift;
  else
    ans = mant;
 
  // Apply sign
  if (sign != 0)
    ans = -ans;

  return (FixedPointNumber) { .value = ans, .scaling = scaling };
}

float from_fixed_point(FixedPointNumber x) {
  if (x.value == 0)
    return 0.0f;

  int32_t ans = 0;
  int32_t unsign = x.value;
  if (x.value <= 0) {
    ans = 1 << 31;
    unsign = -x.value;
  }

  int used_bits = count_rightmost_used_bits(unsign);
  int exp = used_bits - x.scaling - 1;
  int shift = used_bits - 1 - MANTISSA_BITS;

  ans |= EXPONENT_MASK & ((exp + 127) << MANTISSA_BITS);

  if (shift > 0)
    ans |= MANTISSA_MASK & (unsign >> shift);
  else if (shift < 0)
    ans |= MANTISSA_MASK & (unsign << -shift);
  
  return *((float*)&ans);
}

// Minimize scaling factor of both numbers
FixedPointNumber fixed_point_mul(FixedPointNumber x, FixedPointNumber y) {
  int32_t acc = x.value * y.value;
  printf("X (internal): %d\n", x.value);
  printf("Y (internal): %d\n", y.value);
  printf("Acc: %d\n", acc);

  return (FixedPointNumber) {
    .value = (x.value * y.value) >> (x.scaling + y.scaling), 
    .scaling = x.scaling
  };
}

// Maximize scaling factor of x, minimize scaling factor of y
// FixedPointNumber fixed_point_div(FixedPointNumber a, FixedPointNumber b) {
//   return (x / y) << scaling;
// }

int main() {
  const float X = 15.5f;
  const float Y = 2.0f;

  FixedPointNumber x = to_fixed_point(X);
  FixedPointNumber y = to_fixed_point(Y);

  FixedPointNumber ans = fixed_point_mul(x, y);

  printf("X: %.24f\n", from_fixed_point(x));
  printf("Y: %.24f\n", from_fixed_point(y));
  printf("Ans: %.24f\n", from_fixed_point(ans));
}