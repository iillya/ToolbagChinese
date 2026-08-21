#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <sstream>

namespace torch::autograd::utils {

inline std::string requires_grad_leaf_error(bool requires_grad) {
  std::ostringstream oss;
  oss << "you can only change requires_grad flags of leaf variables.";
  if (requires_grad == false) {
    oss << " If you want to use a computed variable in a subgraph "
           "that doesn't require differentiation use "
           "var_no_grad = var.detach().";
  }
  return oss.str();
}

} // namespace torch::autograd::utils

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
