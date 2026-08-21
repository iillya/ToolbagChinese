#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/jit/ir/ir.h>

namespace torch::jit::fuser::onednn {

// Prepare binary ops for LLGA
//
// The pass does the following:
//
// - Convert scalar input of aten::add and aten::mul into Float tensor with
//   dimension [1]
//
// - Decompose fused add into aten::mul + aten::add when alpha != 1.0
//
// - Eliminate identity add/mul, i.e., tensor + 0, tensor * 1
//
void PrepareBinaryForLLGA(const std::shared_ptr<Graph>& graph);

} // namespace torch::jit::fuser::onednn

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
