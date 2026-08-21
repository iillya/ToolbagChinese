#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/jit/ir/ir.h>

namespace torch::jit {

TORCH_API void fuseStaticSubgraphs(
    std::shared_ptr<Graph> graph,
    size_t min_size);

TORCH_API void performTensorExprFusion(
    std::shared_ptr<Graph> graph,
    std::vector<IValue> sample_inputs);

} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
