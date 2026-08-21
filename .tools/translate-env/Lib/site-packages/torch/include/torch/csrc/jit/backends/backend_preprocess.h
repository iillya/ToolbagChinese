#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/jit/backends/backend_detail.h>
namespace torch::jit {
class backend_preprocess_register {
  std::string backend_name_;

 public:
  backend_preprocess_register(
      const std::string& name,
      const detail::BackendPreprocessFunction& preprocess)
      : backend_name_(name) {
    detail::registerBackendPreprocessFunction(name, preprocess);
  }
};
} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
