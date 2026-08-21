#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/jit/ir/ir.h>

namespace torch::jit {

// This pass converts aten ops to a normalized form. It is
// run immediately after IR generation in both the tracer and compiler,
// so downstream consumers of the IR do not need handle ops in their
// pre-normalized form.
// Currently only handles normalization of op aliases.
TORCH_API void NormalizeOps(const std::shared_ptr<Graph>& graph);

const std::unordered_map<Symbol, Symbol>& getOperatorAliasMap();

} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
