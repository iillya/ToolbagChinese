#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/jit/ir/ir.h>
#include <torch/csrc/onnx/onnx.h>

namespace torch::autograd {

struct SymbolicContext {
  jit::Block* block;
};

struct symbolic_unconvertible : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

} // namespace torch::autograd

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
