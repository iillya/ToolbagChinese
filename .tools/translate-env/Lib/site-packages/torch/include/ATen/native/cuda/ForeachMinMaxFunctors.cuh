#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <ATen/NumericUtils.h>

namespace at::native {

// std:: does not have clamp functors
template <typename T>
struct minimum {
  __device__ T operator()(const T& a, const T& b) const {
    return (_isnan(a) || a < b) ? a : b;
  }
};

template <typename T>
struct maximum {
  __device__ T operator()(const T& a, const T& b) const {
    return (_isnan(a) || a > b) ? a : b;
  }
};

} // namespace at::native

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
