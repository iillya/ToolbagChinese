#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

namespace at::Reduction {

// NB: Keep this in sync with Reduction class in torch/nn/_reduction.py
// These constants control the reduction behavior of loss functions.
// Ideally, this would be a scoped enum, but jit doesn't support that
enum Reduction {
  None, // Do not reduce
  Mean, // (Possibly weighted) mean of losses
  Sum, // Sum losses
  END
};
} // namespace at::Reduction

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
