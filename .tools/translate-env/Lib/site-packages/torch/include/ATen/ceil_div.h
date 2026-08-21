#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <c10/macros/Macros.h>
#include <type_traits>

namespace at {

/**
   Computes ceil(a / b)
*/
template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
C10_ALWAYS_INLINE C10_HOST_DEVICE T ceil_div(T a, T b) {
  return (a + b - 1) / b;
}

/**
   Computes ceil(a / b) * b; i.e., rounds up `a` to the next highest
   multiple of b
*/
template <typename T>
C10_ALWAYS_INLINE C10_HOST_DEVICE T round_up(T a, T b) {
  return ceil_div(a, b) * b;
}

} // namespace at

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
