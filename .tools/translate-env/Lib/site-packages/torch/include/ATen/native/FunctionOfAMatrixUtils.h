#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <ATen/native/DispatchStub.h>
#include <cstdint>

namespace at {
struct TensorIterator;

namespace native {

using _compute_linear_combination_fn = void(*)(
  TensorIterator& iter,
  int64_t in_stride,
  int64_t coeff_stride,
  int64_t num_summations
);

DECLARE_DISPATCH(_compute_linear_combination_fn, _compute_linear_combination_stub)

}} // namespace at::native

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
