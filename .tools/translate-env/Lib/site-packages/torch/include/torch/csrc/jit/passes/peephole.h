#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/jit/ir/ir.h>

namespace torch::jit {

// return true if graph is modified
TORCH_API bool PeepholeOptimize(
    const std::shared_ptr<Graph>& graph,
    bool disable_shape_peepholes = false);
// return true if graph is modified
TORCH_API bool PeepholeOptimize(
    Block* block,
    bool disable_shape_peepholes = false);
// return true if graph is modified
TORCH_API bool FuseAddMM(const std::shared_ptr<Graph>& graph);

} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
