#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <memory>

#include <torch/csrc/jit/ir/ir.h>

namespace torch::jit {

void EvalPeepholeONNX(
    std::shared_ptr<Graph>& g,
    std::map<std::string, IValue>& paramDict);

} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
