#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/types.h>

namespace torch::nn::functional {

inline Tensor bilinear(
    const Tensor& input1,
    const Tensor& input2,
    const Tensor& weight,
    const Tensor& bias = Tensor()) {
  return torch::bilinear(input1, input2, weight, bias);
}

// ============================================================================

inline Tensor linear(
    const Tensor& input,
    const Tensor& weight,
    const Tensor& bias = {}) {
  if (input.dim() == 2 && bias.defined()) {
    // fused op is marginally faster
    return torch::addmm(bias, input, weight.t());
  } else {
    auto output = input.matmul(weight.t());
    if (bias.defined()) {
      output += bias;
    }
    return output;
  }
}

} // namespace torch::nn::functional

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
