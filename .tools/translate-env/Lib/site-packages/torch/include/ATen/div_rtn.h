#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

// Integer division rounding to -Infinity
template <typename T>
static inline T div_rtn(T x, T y) {
  int q = x / y;
  int r = x % y;
  if ((r != 0) && ((r < 0) != (y < 0)))
    --q;
  return q;
}

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
