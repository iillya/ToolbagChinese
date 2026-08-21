#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/jit/ir/ir.h>

namespace torch::jit {

// Eliminates common inputs among `aten::cat` ops.
TORCH_API bool EliminateConcatCommonInputs(const std::shared_ptr<Graph>& graph);

// Expands `aten::cat` ops into `aten::copy` ops and eliminates redundancies
// in the buffers used for concatenation if possible.
TORCH_API void ExpandConcatAndEliminateRedundancy(
    const std::shared_ptr<Graph>& graph);

TORCH_API bool CombineConcats(const std::shared_ptr<Graph>& graph);

} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
