#include <stdint.h>
#include <stdio.h>

const uint32_t MANTISSA_BITS = 23;
const uint32_t NORM_MASK = (1 << MANTISSA_BITS);
const uint32_t MANTISSA_MASK = (NORM_MASK - 1);
const uint32_t SIGN_MASK = (1 << 31);
const uint32_t EXPONENT_MASK = (SIGN_MASK - 1) ^ MANTISSA_MASK;
const uint32_t EXPONENT_BIAS = 127;


int32_t to_fixed_point(float x, int scaling) {
  if (x == 0)
    return 0;

  // Decompose IEEE-754
  uint32_t conv = *((uint32_t*)&x);
  int sign = (conv & SIGN_MASK) >> 31;
  int exp = ((conv & EXPONENT_MASK) >> MANTISSA_BITS) - EXPONENT_BIAS;
  int32_t mant = (conv & MANTISSA_MASK) | NORM_MASK;

  int shift = (int)MANTISSA_BITS - exp - scaling;

  if (shift <= (int)MANTISSA_BITS - 31) {
    // MSB of mantissa overflows - saturate to max value
    if (sign != 0)
      return -SIGN_MASK;
    else
      return SIGN_MASK - 1;
  }


  int32_t ans;
  if (shift > 0)
    ans = mant >> shift;
  else if (shift < 0)
    ans = mant << -shift;
  else
    ans = mant;

  if (sign != 0)
    ans = -ans;

  return ans;
}

int count_rightmost_used_bits(uint32_t x) {
  int i = 0;

  while (i <= 31 && x > 0) {
    x = x >> 1;
    ++i;
  }

  return i;
}

float from_fixed_point(int32_t x, int scaling) {
  if (x == 0)
    return 0.0f;

  int32_t ans = 0;
  int32_t unsign = x;
  if (x <= 0) {
    ans = 1 << 31;
    unsign = -x;
  }

  int used_bits = count_rightmost_used_bits(unsign);
  int exp = used_bits - scaling - 1;
  int shift = used_bits - 1 - MANTISSA_BITS;

  ans |= EXPONENT_MASK & ((exp + 127) << MANTISSA_BITS);

  if (shift > 0)
    ans |= MANTISSA_MASK & (unsign >> shift);
  else if (shift < 0)
    ans |= MANTISSA_MASK & (unsign << -shift);
  
  return *((float*)&ans);
}

// TODO: Minimize scaling factor of both numbers
int32_t fixed_point_mul(int32_t x, int32_t y, int scaling) {
  return (x * y) >> scaling;
}

// TODO: Maximize scaling factor of x, minimize scaling factor of y
int32_t fixed_point_div(int32_t x, int32_t y, int scaling) {
  return (x / y) << scaling;
}

int main() {
  const int SCALING = 20;

  int32_t a = to_fixed_point(55.36f, SCALING);
  int32_t b = to_fixed_point(2.1f, SCALING);

  int32_t fixed = fixed_point_div(a, b, SCALING);
  float floating = from_fixed_point(fixed, SCALING);

  printf("Fixed point: %d\n", fixed);
  printf("Floating point: %f\n", floating);
}