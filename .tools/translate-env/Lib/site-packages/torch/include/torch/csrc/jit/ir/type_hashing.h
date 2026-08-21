#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <ATen/core/jit_type.h>
#include <torch/csrc/jit/ir/ir.h>

namespace torch::jit {

struct TORCH_API HashType {
  size_t operator()(const TypePtr& type) const;
  size_t operator()(const c10::ConstTypePtr& type) const;
};

struct EqualType {
  bool operator()(const TypePtr& a, const TypePtr& b) const;
  bool operator()(const c10::ConstTypePtr& a, const c10::ConstTypePtr& b) const;
};

} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
