#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <ATen/native/DispatchStub.h>

namespace at {
class Tensor;
}

namespace at::native {

using reduce_all_fn = void (*)(Tensor & result, const Tensor & self);
using reduce_min_max_fn = void (*)(Tensor & max_result, Tensor & min_result, const Tensor & self);
DECLARE_DISPATCH(reduce_all_fn, min_all_stub)
DECLARE_DISPATCH(reduce_all_fn, max_all_stub)

} // namespace at::native

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
