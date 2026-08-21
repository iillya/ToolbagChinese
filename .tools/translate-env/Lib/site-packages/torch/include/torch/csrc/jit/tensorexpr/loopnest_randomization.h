#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

namespace torch::jit::tensorexpr {

// Applies a series of loop optimizations chosen randomly. This is only for
// testing purposes. This allows automatic stress testing of NNC loop
// transformations.
void loopnestRandomization(int64_t seed, LoopNest& l);
} // namespace torch::jit::tensorexpr

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
