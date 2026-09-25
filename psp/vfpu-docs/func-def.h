
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

// Aux functions
static inline bool fpiszero(uint32_t n) {
  // Denormals are considered zero too!
  return ((n >> 23) & 0xFF) == 0;
}
static inline bool fpisnan(uint32_t n) {
  return ((n >> 23) & 0xFF) == 0xFF && ((n & 0x7FFFFF) != 0);
}
static inline bool fpisnanorinf(uint32_t n) {
  return ((n >> 23) & 0xFF) == 0xFF;
}

// Used by vi2f instruction to convert integer to float
// Just a wrapper so we can do int to int
static inline uint32_t vi2f(uint32_t intval, uint32_t scale) {
  signed int ival = (signed)intval;
  union {
    float f32;
    uint32_t u32;
  } ret;
  ret.f32 = ldexp(ival, -scale);
  return ret.u32;
}

