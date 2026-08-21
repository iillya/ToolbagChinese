#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <cstdint>
#include <ATen/core/TensorBase.h>
#include <ATen/native/cuda/SortStable.h>


namespace at::native {

inline bool should_use_small_sort(const TensorBase &self, int64_t dim) {
  return self.size(dim) <= 4096;
}

void sortKeyValueInplace(
    const TensorBase &key, const TensorBase &value, int64_t dim,
    bool descending, bool stable=false);

} // namespace at::native

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
