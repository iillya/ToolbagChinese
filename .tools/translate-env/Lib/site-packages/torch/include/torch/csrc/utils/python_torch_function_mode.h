#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <ATen/PythonTorchFunctionTLS.h>

namespace torch::overrides {

struct StashTorchFunctionModeGuard {
  StashTorchFunctionModeGuard() {
    cur_mode_ = at::impl::PythonTorchFunctionTLS::pop_stack();
  }
  ~StashTorchFunctionModeGuard() {
    at::impl::PythonTorchFunctionTLS::push_onto_stack(cur_mode_);
  }
  StashTorchFunctionModeGuard(const StashTorchFunctionModeGuard&) = delete;
  StashTorchFunctionModeGuard(StashTorchFunctionModeGuard&&) = delete;
  StashTorchFunctionModeGuard& operator=(const StashTorchFunctionModeGuard&) =
      delete;
  StashTorchFunctionModeGuard& operator=(StashTorchFunctionModeGuard&&) =
      delete;

  const std::shared_ptr<c10::SafePyObject>& get_cur_mode() {
    return cur_mode_;
  }

 private:
  std::shared_ptr<c10::SafePyObject> cur_mode_;
};

} // namespace torch::overrides

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
