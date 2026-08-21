#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/jit/ir/ir.h>

#include <memory>

namespace torch::jit {
// see .cpp for docs
TORCH_API void RemoveInplaceOps(const std::shared_ptr<Graph>& graph);

TORCH_API void ImplicitCastForBinaryInplaceOps(Block* block);
} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
