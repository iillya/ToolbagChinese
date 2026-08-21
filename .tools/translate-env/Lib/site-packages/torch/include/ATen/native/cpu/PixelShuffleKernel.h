#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <ATen/native/DispatchStub.h>

namespace at {
class TensorBase;
}

namespace at::native {

using pixel_shuffle_fn = void(*)(TensorBase&, const TensorBase&, int64_t);
DECLARE_DISPATCH(pixel_shuffle_fn, pixel_shuffle_kernel)
DECLARE_DISPATCH(pixel_shuffle_fn, pixel_unshuffle_kernel)

} // at::native

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
