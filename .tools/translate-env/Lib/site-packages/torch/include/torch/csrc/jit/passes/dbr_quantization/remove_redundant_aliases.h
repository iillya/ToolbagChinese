#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/jit/api/module.h>

namespace torch::jit {

// This function replaces instances of
//
//   %b = aten::alias(%a)
//   %c = foo(%b)
//
// with
//
//   %c = foo(%a)
//
// on the module forward, if it's safe to do so.
TORCH_API Module DBRQuantRemoveRedundantAliases(Module& module);

} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
