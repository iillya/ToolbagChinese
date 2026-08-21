#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/jit/ir/ir.h>

/** \brief Runs a set of Optimizations that Optimize Frozen Graphs
 *
 * Currently this set of optimizations is:
 * - FoldFrozenConvBatchnorm
 * - FoldFrozenConvAddOrSub
 * - FoldFrozenConvMulOrDiv
 * - FoldFrozenLinearBatchnorm
 */

namespace torch::jit {

TORCH_API void OptimizeFrozenGraph(
    std::shared_ptr<Graph>& graph,
    bool optimize_numerics = true);

} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
