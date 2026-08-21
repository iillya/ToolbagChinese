#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/lazy/core/ir.h>

namespace torch::lazy {

TORCH_API NodePtr operator+(const Value& node1, const Value& node2);
TORCH_API NodePtr operator-(const Value& node1, const Value& node2);
TORCH_API NodePtr operator*(const Value& node1, const Value& node2);
TORCH_API NodePtr operator/(const Value& node1, const Value& node2);

} // namespace torch::lazy

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
