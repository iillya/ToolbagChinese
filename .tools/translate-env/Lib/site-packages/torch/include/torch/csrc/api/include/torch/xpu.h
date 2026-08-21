#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/Export.h>

#include <cstddef>
#include <cstdint>

namespace torch::xpu {

/// Returns the number of XPU devices available.
size_t TORCH_API device_count();

/// Returns true if at least one XPU device is available.
bool TORCH_API is_available();

/// Sets the seed for the current GPU.
void TORCH_API manual_seed(uint64_t seed);

/// Sets the seed for all available GPUs.
void TORCH_API manual_seed_all(uint64_t seed);

/// Waits for all kernels in all streams on a XPU device to complete.
void TORCH_API synchronize(int64_t device_index);

} // namespace torch::xpu

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
