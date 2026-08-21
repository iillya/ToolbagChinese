#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <torch/csrc/Export.h>

#include <cstddef>
#include <cstdint>

namespace torch::jit::fuser {

// Host-side view of TensorInfo
// Note dims[0] - we need to dynamically allocate the dims.
struct TORCH_API TensorInfo {
  uint32_t* sizes(size_t nDim) {
    return &sizes_strides[0];
  }
  uint32_t* strides(size_t nDim) {
    return &sizes_strides[nDim];
  }

  void* data;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  uint32_t sizes_strides[0];
};

} // namespace torch::jit::fuser

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
