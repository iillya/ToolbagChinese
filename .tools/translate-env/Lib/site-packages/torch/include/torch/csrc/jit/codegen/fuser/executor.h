#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <ATen/core/stack.h>
#include <torch/csrc/Export.h>
#include <torch/csrc/jit/codegen/fuser/fused_kernel.h>
#include <torch/csrc/jit/codegen/fuser/kernel_spec.h>

#include <cstdint>

namespace torch::jit::fuser {

// Runs the fusion associated with the key (see registerFusion() in interface.h)
// on the inputs taken from the given Stack.
TORCH_API bool runFusion(
    const int64_t key,
    Stack& stack,
    std::string* code_out = nullptr);

} // namespace torch::jit::fuser

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
