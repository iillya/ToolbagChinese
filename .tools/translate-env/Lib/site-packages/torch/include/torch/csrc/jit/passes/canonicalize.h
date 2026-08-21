#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/jit/ir/ir.h>

namespace torch::jit {

TORCH_API std::shared_ptr<Graph> Canonicalize(
    const std::shared_ptr<Graph>& graph,
    bool keep_unique_names = true);

TORCH_API void CanonicalizeOutputs(std::shared_ptr<Graph>& graph);

TORCH_API std::optional<const Use> firstOrLastUse(Value* v, bool find_first);

TORCH_API bool isBeforeOrAfter(
    const Use& a,
    const Use& b,
    bool checking_before);

} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
