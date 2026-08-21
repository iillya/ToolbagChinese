#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <ATen/core/operator_name.h>
#include <string>
#include <unordered_set>

namespace c10 {

struct TORCH_API ObservedOperators {
  ObservedOperators() = delete;

  static bool isObserved(const OperatorName& name);

  static std::unordered_set<std::string>& getUnobservedOperatorList();
};

} // namespace c10

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
