#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <c10/metal/common.h>

template <unsigned N = 5>
struct UpsampleParams {
  ::c10::metal::array<uint64_t, N> input_strides;
  ::c10::metal::array<uint64_t, N> input_sizes;
  ::c10::metal::array<uint64_t, N> output_strides;
  ::c10::metal::array<uint64_t, N> output_sizes;
  ::c10::metal::array<float, N - 2> scales;
  bool align_corners;
};

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
