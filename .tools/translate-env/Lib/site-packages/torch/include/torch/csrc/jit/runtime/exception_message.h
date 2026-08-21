#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <c10/util/Exception.h>
#include <stdexcept>

namespace torch::jit {

struct ExceptionMessage {
  ExceptionMessage(const std::exception& e) : e_(e) {}

 private:
  const std::exception& e_;
  friend std::ostream& operator<<(
      std::ostream& out,
      const ExceptionMessage& msg);
};

inline std::ostream& operator<<(
    std::ostream& out,
    const ExceptionMessage& msg) {
  auto c10_error = dynamic_cast<const c10::Error*>(&msg.e_);
  if (c10_error) {
    out << c10_error->what_without_backtrace();
  } else {
    out << msg.e_.what();
  }
  return out;
}

} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
