#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/jit/ir/ir.h>

namespace torch::jit {

// removes tuples where TupleConstruct and TupleUnpack are matched
// but leaves tuples in place across if statements, loops, and as inputs/outputs
TORCH_API void LowerSimpleTuples(const std::shared_ptr<Graph>& graph);

// removes _all_ tuples and raises an error if some cannot be removed
// this is used by ONNX to ensure there are not tuples before conversion,
// but will not work on graphs whose inputs contain tuples.
TORCH_API void LowerAllTuples(const std::shared_ptr<Graph>& graph);

TORCH_API void LowerSimpleTuples(Block* block);

} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
