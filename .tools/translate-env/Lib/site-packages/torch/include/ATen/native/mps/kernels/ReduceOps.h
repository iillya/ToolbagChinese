#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <c10/metal/common.h>

#define MAX_THREADGROUP_SIZE static_cast<uint32_t>(1024)
C10_METAL_CONSTEXPR uint32_t SUM_NCHAINS = 8;

template <unsigned N = c10::metal::max_ndim>
struct NormParams {
  float p;
  uint32_t reduction_size;
  uint32_t ndim;

  ::c10::metal::array<uint32_t, N> input_sizes;
  ::c10::metal::array<uint32_t, N> input_strides;

  ::c10::metal::array<uint32_t, N> output_sizes;
  ::c10::metal::array<uint32_t, N> output_strides;
};

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
