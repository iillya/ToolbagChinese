#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <functional>

namespace torch::jit {

class ResourceGuard {
  std::function<void()> _destructor;
  bool _released{false};

 public:
  ResourceGuard(std::function<void()> destructor)
      : _destructor(std::move(destructor)) {}

  // NOLINTNEXTLINE(bugprone-exception-escape)
  ~ResourceGuard() {
    if (!_released)
      _destructor();
  }

  void release() {
    _released = true;
  }
};

} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
