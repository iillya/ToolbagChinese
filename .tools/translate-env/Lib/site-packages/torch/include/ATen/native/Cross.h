#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <ATen/native/DispatchStub.h>

namespace at {
class Tensor;

namespace native {

using cross_fn = void(*)(const Tensor&, const Tensor&, const Tensor&, const int64_t d);

DECLARE_DISPATCH(cross_fn, cross_stub)

}} // namespace at::native

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
