#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/Export.h>
#include <torch/csrc/jit/ir/ir.h>

#include <cstddef>

namespace torch::jit {

// insert GraphExecutor nodes that group together
// subgraphs that are differentiable by the jit's autodiff passes
// threshold - minimum number of nodes that will appear in a block
// returns all differentiable blocks that have been found
TORCH_API std::vector<Node*> CreateAutodiffSubgraphs(
    const std::shared_ptr<Graph>& graph,
    size_t threshold = 2);
} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
