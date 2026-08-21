#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/jit/ir/ir.h>

namespace torch::jit {

// Converts operators & their parameters to mkldnn if it is profitable
// Currently encompassing Conv2d and Conv3d, and Linear
// Op must be in float32 and mkldnn must be built
// This pass only works on frozen graph
TORCH_API void ConvertFrozenOpsToMKLDNN(std::shared_ptr<Graph>& graph);

} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
