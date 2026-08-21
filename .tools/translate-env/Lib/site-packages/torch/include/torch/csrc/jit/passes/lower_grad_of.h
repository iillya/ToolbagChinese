#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/jit/ir/ir.h>

namespace torch::jit {

// This pass removes 'grad_of' nodes, replacing them with conditionals of
// the form:
// if any_defined(inputs):
//  outputs = <original_computation>
// else:
//  outputs = undefineds
TORCH_API void LowerGradOf(Graph& g);

} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
