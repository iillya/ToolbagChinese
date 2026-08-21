#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <ATen/core/ivalue.h>

namespace torch::jit::mobile {
/**
 * Recursively scan the IValue object, traversing lists, tuples, dicts, and stop
 * and call the user provided callback function 'func' when a Tensor is found.
 */
void for_each_tensor_in_ivalue(
    const ::c10::IValue& iv,
    std::function<void(const ::at::Tensor&)> const& func);
} // namespace torch::jit::mobile

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
