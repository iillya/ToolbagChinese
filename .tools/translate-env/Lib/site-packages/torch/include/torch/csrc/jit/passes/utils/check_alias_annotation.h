#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <ATen/core/ivalue.h>
#include <torch/csrc/jit/ir/ir.h>
#include <memory>
#include <string>
#include <vector>

namespace torch::jit {

// Verify that alias annotations are correct. See impl for definition of
// "correct".
//
// This function expects a graph with a single op with `unqualifiedOpName`, plus
// the inputs that you would otherwise have passed to the graph executor.
TORCH_API void checkAliasAnnotation(
    const std::shared_ptr<Graph>& graph,
    std::vector<IValue> pythonInputs,
    const std::string& unqualifiedOpName);
} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
