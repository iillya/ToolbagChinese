#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <c10/macros/Macros.h>

namespace c10 {

// RAII thread local guard that tracks whether code is being executed in
// `at::parallel_for` or `at::parallel_reduce` loop function.
class C10_API ParallelGuard {
 public:
  static bool is_enabled();

  ParallelGuard(bool state);
  ~ParallelGuard();

 private:
  bool previous_state_;
};

} // namespace c10

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
