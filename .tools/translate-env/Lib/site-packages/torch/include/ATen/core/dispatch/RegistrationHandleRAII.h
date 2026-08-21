#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <functional>

namespace c10 {

class RegistrationHandleRAII final {
 public:
  explicit RegistrationHandleRAII(std::function<void()> onDestruction)
      : onDestruction_(std::move(onDestruction)) {}

  ~RegistrationHandleRAII() {
    if (onDestruction_) {
      onDestruction_();
    }
  }

  RegistrationHandleRAII(const RegistrationHandleRAII&) = delete;
  RegistrationHandleRAII& operator=(const RegistrationHandleRAII&) = delete;

  RegistrationHandleRAII(RegistrationHandleRAII&& rhs) noexcept
      : onDestruction_(std::move(rhs.onDestruction_)) {
    rhs.onDestruction_ = nullptr;
  }

  RegistrationHandleRAII& operator=(RegistrationHandleRAII&& rhs) noexcept {
    onDestruction_ = std::move(rhs.onDestruction_);
    rhs.onDestruction_ = nullptr;
    return *this;
  }

 private:
  std::function<void()> onDestruction_;
};

} // namespace c10

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
