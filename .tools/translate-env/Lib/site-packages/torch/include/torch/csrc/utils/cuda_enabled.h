#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

namespace torch::utils {

inline constexpr bool cuda_enabled() {
#ifdef USE_CUDA
  return true;
#else
  return false;
#endif
}

} // namespace torch::utils

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
