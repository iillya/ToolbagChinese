#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/jit/frontend/resolver.h>

namespace torch::jit {
// Create a Resolver for use in generating LoweredModules for specific backends.
TORCH_API std::shared_ptr<Resolver> loweredModuleResolver();
} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
