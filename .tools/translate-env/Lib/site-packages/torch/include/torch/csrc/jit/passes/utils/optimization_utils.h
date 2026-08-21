#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)

#pragma once

#include <torch/csrc/jit/ir/ir.h>

namespace torch::jit {

// Checks if the parameters, not including the
// first param are all constants.
bool nonConstantParameters(Node* n);

} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
