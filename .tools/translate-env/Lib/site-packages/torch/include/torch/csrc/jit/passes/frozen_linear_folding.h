#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/jit/ir/ir.h>

namespace torch::jit {

// Fuses Linear -> BatchNormNd into a single Linear by
// folding batchnorm weights into linear weights.
// This pass only works on Frozen Graphs; otherwise it is a No-Op.
TORCH_API bool FoldFrozenLinearBatchnorm(std::shared_ptr<Graph>& graph);

} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
