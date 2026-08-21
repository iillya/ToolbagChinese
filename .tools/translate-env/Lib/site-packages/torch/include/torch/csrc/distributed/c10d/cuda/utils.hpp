#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

// This file contains utility functions common for CUDA, which can be used by
// ProcessGroupNCCL or SymmetricMemory.

namespace c10d::cuda {

bool deviceSupportsMulticast(int device_idx);

} // namespace c10d::cuda

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
