#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <ATen/native/DispatchStub.h>
#include <cstdint>

namespace at {
class TensorBase;
}

namespace at::native {

using weight_norm_fn = void(*)(
    TensorBase&, TensorBase&, const TensorBase&, const TensorBase&, int64_t);
using weight_norm_backward_fn = void(*)(
    TensorBase&, TensorBase&, const TensorBase&, const TensorBase&,
    const TensorBase&, const TensorBase&, int64_t);

DECLARE_DISPATCH(weight_norm_fn, weight_norm_stub)
DECLARE_DISPATCH(weight_norm_backward_fn, weight_norm_backward_stub)

} // namespace at::native

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
