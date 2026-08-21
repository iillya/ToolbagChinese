#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/autograd/custom_function.h>
#include <torch/csrc/autograd/variable.h>
#include <torch/nn/options/normalization.h>
#include <torch/types.h>

namespace torch::nn::functions {

class CrossMapLRN2d : public torch::autograd::Function<CrossMapLRN2d> {
 public:
  static torch::autograd::Variable forward(
      torch::autograd::AutogradContext* ctx,
      const torch::autograd::Variable& input,
      const CrossMapLRN2dOptions& options);

  static torch::autograd::variable_list backward(
      torch::autograd::AutogradContext* ctx,
      torch::autograd::variable_list grad_output);
};

} // namespace torch::nn::functions

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
